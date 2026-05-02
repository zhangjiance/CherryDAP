/*!
 * @file       board_osbdm.c
 *
 * @brief      Board GPIO implementation for OSBDM on APM32E103
 *
 * @note       Hardware initialization and control functions
 */

#include "board_osbdm.h"
#include "apm32e10x_adc.h"
#include "apm32e10x_tmr.h"

/* Dummy legacy GPIO register used by JM60 compatibility macros. */
volatile uint8_t osbdm_dummy_gpio_reg = 0;

/**
 * @brief Initialize board GPIOs for OSBDM operation
 */
void board_osbdm_gpio_init(void)
{
    GPIO_Config_T gpioConfig;
    
    /* Enable GPIO clocks */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | 
                              RCM_APB2_PERIPH_GPIOB | 
                              RCM_APB2_PERIPH_GPIOC);
    
    /* Configure JTAG output pins (TDI, TMS, TCK) - Push-pull, 50MHz */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    
    /* PA3 - TDI */
    gpioConfig.pin = TDI_PIN;
    GPIO_Config(TDI_PORT, &gpioConfig);
    GPIO_ResetBit(TDI_PORT, TDI_PIN);
    
    /* PA4 - TMS */
    gpioConfig.pin = TMS_PIN;
    GPIO_Config(TMS_PORT, &gpioConfig);
    GPIO_ResetBit(TMS_PORT, TMS_PIN);
    
    /* PA5 - TCK */
    gpioConfig.pin = TCK_PIN;
    GPIO_Config(TCK_PORT, &gpioConfig);
    GPIO_ResetBit(TCK_PORT, TCK_PIN);
    
    /* PA6 - TDO - Input floating */
    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.pin = TDO_PIN;
    GPIO_Config(TDO_PORT, &gpioConfig);
    
    /* PC13 - TRST - Open-drain output */
    gpioConfig.mode = GPIO_MODE_OUT_OD;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = TRST_PIN;
    GPIO_Config(TRST_PORT, &gpioConfig);
    GPIO_SetBit(TRST_PORT, TRST_PIN);  /* Deassert TRST (active low) */
    
    /* PA2 - SRST - Open-drain output */
    gpioConfig.mode = GPIO_MODE_OUT_OD;
    gpioConfig.pin = SRST_OUT_PIN;
    GPIO_Config(SRST_OUT_PORT, &gpioConfig);
    GPIO_SetBit(SRST_OUT_PORT, SRST_OUT_PIN);  /* Deassert SRST (active low) */
    
    /* Configure LED pins - Active low (sink current) */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_2MHz;
    
    gpioConfig.pin = LED_GREEN_PIN;
    GPIO_Config(LED_PORT, &gpioConfig);
    GPIO_SetBit(LED_PORT, LED_GREEN_PIN);  /* Off */
    
    gpioConfig.pin = LED_RED_PIN;
    GPIO_Config(LED_PORT, &gpioConfig);
    GPIO_SetBit(LED_PORT, LED_RED_PIN);  /* Off */
    
    gpioConfig.pin = LED_ORANGE_PIN;
    GPIO_Config(LED_PORT, &gpioConfig);
    GPIO_SetBit(LED_PORT, LED_ORANGE_PIN);  /* Off */
    
    /* Configure target power control - PB1 */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_2MHz;
    gpioConfig.pin = PWR_PIN;
    GPIO_Config(PWR_PORT, &gpioConfig);
    GPIO_ResetBit(PWR_PORT, PWR_PIN);  /* Power off initially */
    
    /* Configure target voltage sense - PB0 - Analog input */
    gpioConfig.mode = GPIO_MODE_ANALOG;
    gpioConfig.pin = VREF_PIN;
    GPIO_Config(VREF_PORT, &gpioConfig);
}

/**
 * @brief Initialize JTAG/BDM pins to default state
 */
void board_jtag_pins_init(void)
{
    /* Set all control signals to safe defaults */
    TCK_LOW();
    TMS_HIGH();      /* Idle state */
    TDI_LOW();
    TRST_DEASSERT(); /* Not resetting */
    SRST_DEASSERT(); /* Not resetting */
}

/**
 * @brief Initialize target power control
 */
void board_target_power_init(void)
{
    ADC_Config_T adcConfig;
    
    /* Enable ADC1 clock */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_ADC1);
    
    /* ADC configuration */
    ADC_Reset(ADC1);
    adcConfig.mode = ADC_MODE_INDEPENDENT;
    adcConfig.scanConvMode = DISABLE;
    adcConfig.continuosConvMode = DISABLE;
    adcConfig.externalTrigConv = ADC_EXT_TRIG_CONV_None;
    adcConfig.dataAlign = ADC_DATA_ALIGN_RIGHT;
    adcConfig.nbrOfChannel = 1;
    ADC_Config(ADC1, &adcConfig);
    
    /* Enable ADC1 */
    ADC_Enable(ADC1);
    
    /* ADC calibration */
    ADC_ResetCalibration(ADC1);
    while (ADC_ReadResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_ReadCalibrationStartFlag(ADC1));
}

/**
 * @brief Set target power state
 */
void board_target_power_set(bool enable)
{
    if (enable) {
        PWR_ENABLE();
    } else {
        PWR_DISABLE();
    }
}

/**
 * @brief Read target voltage (via ADC)
 * @return Voltage in millivolts
 */
uint16_t board_target_voltage_read(void)
{
    uint16_t adc_value;
    uint32_t voltage_mv;
    
    /* Configure ADC channel 8 (PB0) */
    ADC_ConfigRegularChannel(ADC1, ADC_CHANNEL_8, 1, ADC_SAMPLETIME_55CYCLES5);
    
    /* Start conversion */
    ADC_EnableSoftwareStartConv(ADC1);
    
    /* Wait for conversion complete */
    while (!ADC_ReadStatusFlag(ADC1, ADC_FLAG_EOC));
    
    /* Read ADC value (12-bit, 0-4095) */
    adc_value = ADC_ReadConversionValue(ADC1);
    
    /* Convert to millivolts
     * Voltage divider: 4.7K + 10K = 14.7K total
     * Ratio = (10K / 14.7K) = 0.68
     * VREF = ADC_value * (3.3V / 4096) / 0.68
     * VREF_mV = (adc_value * 3300 * 147) / (4096 * 100)
     */
    voltage_mv = (adc_value * 3300UL * 147UL) / (4096UL * 100UL);
    
    return (uint16_t)voltage_mv;
}

/**
 * @brief Check if target power is good
 */
bool board_target_power_good(void)
{
    uint16_t voltage = board_target_voltage_read();
    return (voltage > 1000);  /* > 1.0V */
}

/**
 * @brief Microsecond delay using DWT cycle counter (Cortex-M3)
 */
void delay_us(uint32_t us)
{
    uint32_t start, current, elapsed;
    uint32_t cycles = us * (SystemCoreClock / 1000000);
    
    /* Enable DWT if not already enabled */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    
    start = DWT->CYCCNT;
    
    do {
        current = DWT->CYCCNT;
        elapsed = current - start;
    } while (elapsed < cycles);
}

/**
 * @brief Millisecond delay
 */
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

/* Compatibility aliases for OSBDM driver */
void wait_ms(uint16_t ms)
{
    delay_ms(ms);
}

void dly(uint16_t cycles)
{
    /* Simple cycle delay - approximate */
    volatile uint32_t i;
    for (i = 0; i < cycles; i++) {
        __NOP();
    }
}
