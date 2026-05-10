/*!
 * @file       board.c
 *
 * @brief      Board support functions for CherryDAP on APM32E103
 *
 * @note       Pin definitions based on BlackMagic Native Plus
 */

#include "board.h"
#include "apm32e10x_adc.h"
#include "apm32e10x_fmc.h"
#include "apm32e10x_tmr.h"

static volatile uint8_t g_target_power_enabled = 0;
static volatile uint8_t g_target_voltage_adc_ready = 0;

static void board_jtag_gpio_init(void)
{
    GPIO_Config_T gpioConfig;

    RCM_EnableAPB2PeriphClock(JTAG_RCC | RCM_APB2_PERIPH_GPIOC | RCM_APB2_PERIPH_AFIO);

    /* JTAG outputs: TDI/TMS/TCK push-pull. */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = TDI_PIN | TMS_SWDIO_PIN | TCK_SWCLK_PIN;
    GPIO_Config(JTAG_PORT, &gpioConfig);
    GPIO_ResetBit(JTAG_PORT, TDI_PIN | TMS_SWDIO_PIN | TCK_SWCLK_PIN);

    /* TMS direction control for external level-shifter buffer. */
    gpioConfig.pin = TMS_DIR_PIN;
    GPIO_Config(TMS_DIR_PORT, &gpioConfig);
    GPIO_SetBit(TMS_DIR_PORT, TMS_DIR_PIN);

    /* TDO input floating. */
    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.pin = TDO_PIN;
    GPIO_Config(JTAG_PORT, &gpioConfig);

    /* TRST/SRST as open-drain outputs, deasserted high. */
    gpioConfig.mode = GPIO_MODE_OUT_OD;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = TRST_PIN;
    GPIO_Config(TRST_PORT, &gpioConfig);
    GPIO_SetBit(TRST_PORT, TRST_PIN);

    gpioConfig.pin = NRST_PIN;
    GPIO_Config(JTAG_PORT, &gpioConfig);
    GPIO_ResetBit(JTAG_PORT, NRST_PIN);
}

static void board_target_voltage_init(void)
{
    GPIO_Config_T gpioConfig;
    ADC_Config_T adcConfig;

    RCM_EnableAPB2PeriphClock(TPWR_SENSE_RCC | RCM_APB2_PERIPH_ADC1);
    RCM_ConfigADCCLK(RCM_PCLK2_DIV_6);

    gpioConfig.mode = GPIO_MODE_ANALOG;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = TPWR_SENSE_PIN;
    GPIO_Config(TPWR_SENSE_PORT, &gpioConfig);

    ADC_ConfigStructInit(&adcConfig);
    adcConfig.mode = ADC_MODE_INDEPENDENT;
    adcConfig.scanConvMode = DISABLE;
    adcConfig.continuosConvMode = DISABLE;
    adcConfig.externalTrigConv = ADC_EXT_TRIG_CONV_None;
    adcConfig.dataAlign = ADC_DATA_ALIGN_RIGHT;
    adcConfig.nbrOfChannel = 1;
    ADC_Config(ADC1, &adcConfig);

    ADC_ConfigRegularChannel(ADC1, ADC_CHANNEL_8, 1, ADC_SAMPLETIME_239CYCLES5);
    ADC_Enable(ADC1);

    ADC_ResetCalibration(ADC1);
    while (ADC_ReadResetCalibrationStatus(ADC1)) {
    }
    ADC_StartCalibration(ADC1);
    while (ADC_ReadCalibrationStartFlag(ADC1)) {
    }

    g_target_voltage_adc_ready = 1;
}

static void board_system_clock_init(void)
{
    RCM_Reset();
    RCM_ConfigHSE(RCM_HSE_OPEN);

    if (RCM_WaitHSEReady() == SUCCESS) {
        FMC_EnablePrefetchBuffer();
        FMC_ConfigLatency(FMC_LATENCY_2);

        RCM_ConfigAHB(RCM_AHB_DIV_1);
        RCM_ConfigAPB2(RCM_APB_DIV_1);
        RCM_ConfigAPB1(RCM_APB_DIV_2);

    #if defined(CHERRYDAP_SYSCLK_72M)
        /* HSE = 8MHz, PLL x9 => SYSCLK = 72MHz */
        RCM_ConfigPLL(RCM_PLLSEL_HSE, RCM_PLLMF_9);
    #elif defined(CHERRYDAP_SYSCLK_96M)
        /* HSE = 8MHz, PLL x12 => SYSCLK = 96MHz */
        RCM_ConfigPLL(RCM_PLLSEL_HSE, RCM_PLLMF_12);
    #else
        /* Default: HSE = 8MHz, PLL x15 => SYSCLK = 120MHz */
        RCM_ConfigPLL(RCM_PLLSEL_HSE, RCM_PLLMF_15);
    #endif
        RCM_EnablePLL();
        while (RCM_ReadStatusFlag(RCM_FLAG_PLLRDY) == RESET) {
        }

        RCM_ConfigSYSCLK(RCM_SYSCLK_SEL_PLL);
        while (RCM_ReadSYSCLKSource() != RCM_SYSCLK_SEL_PLL) {
        }

    #if defined(CHERRYDAP_SYSCLK_72M)
        /* USB clock = PLL / 1.5 = 72MHz / 1.5 = 48MHz */
        RCM_ConfigUSBCLK(RCM_USB_DIV_1_5);
    #elif defined(CHERRYDAP_SYSCLK_96M)
        /* USB clock = PLL / 2 = 96MHz / 2 = 48MHz */
        RCM_ConfigUSBCLK(RCM_USB_DIV_2);
    #else
        /* USB clock = PLL / 2.5 = 120MHz / 2.5 = 48MHz */
        RCM_ConfigUSBCLK(RCM_USB_DIV_2_5);
    #endif

        SystemCoreClockUpdate();
        RCM_EnableCSS();
    }
}

/*!
 * @brief       Initialize board - system clock configuration
 */
void board_init(void)
{
    GPIO_Config_T gpioConfig;

    /*
     * Hardware fallback: if BOOT_REQ is held low at power-on,
     * jump to bootloader directly without requiring DFU runtime detach.
     */
    RCM_EnableAPB2PeriphClock(BOOT_REQ_RCC);
    gpioConfig.mode = GPIO_MODE_IN_PU;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = BOOT_REQ_PIN;
    GPIO_Config(BOOT_REQ_PORT, &gpioConfig);
    for (volatile uint32_t i = 0; i < 5000U; i++) {
    }
    if (GPIO_ReadInputBit(BOOT_REQ_PORT, BOOT_REQ_PIN) == BIT_RESET) {
        board_request_bootloader();
    }

    /* Ensure vector table points to application region when bootloader is present. */
    NVIC_ConfigVectorTable(NVIC_VECT_TAB_FLASH, VECT_TAB_OFFSET);

    board_system_clock_init();

    /* Enable clocks used by DAP/USB/UART paths. */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_AFIO |
                              RCM_APB2_PERIPH_GPIOA |
                              RCM_APB2_PERIPH_GPIOB |
                              RCM_APB2_PERIPH_GPIOC |
                              RCM_APB2_PERIPH_USART1);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USB);
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

    board_jtag_gpio_init();
    board_target_power_init();
    board_target_voltage_init();
}

void board_target_power_init(void)
{
    GPIO_Config_T gpioConfig;

    RCM_EnableAPB2PeriphClock(TPWR_CTRL_RCC);

    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = TPWR_CTRL_PIN;
    GPIO_Config(TPWR_CTRL_PORT, &gpioConfig);

    /* BlackMagic native tpwr bridge pin is active-low, keep power off by default. */
    GPIO_SetBit(TPWR_CTRL_PORT, TPWR_CTRL_PIN);
    g_target_power_enabled = 0;
}

void board_target_power_set(bool enable)
{
    GPIO_Config_T gpioConfig;

    /* Re-assert pin mode in case DAP port setup switched PB1 to input. */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = TPWR_CTRL_PIN;
    GPIO_Config(TPWR_CTRL_PORT, &gpioConfig);

    if (enable) {
        GPIO_ResetBit(TPWR_CTRL_PORT, TPWR_CTRL_PIN);
        g_target_power_enabled = 1;
    } else {
        GPIO_SetBit(TPWR_CTRL_PORT, TPWR_CTRL_PIN);
        g_target_power_enabled = 0;
    }
}

bool board_target_power_get(void)
{
    return g_target_power_enabled != 0;
}

uint32_t board_target_voltage_adc_mv(void)
{
    uint32_t raw;

    if (!g_target_voltage_adc_ready) {
        board_target_voltage_init();
    }

    ADC_ConfigRegularChannel(ADC1, ADC_CHANNEL_8, 1, ADC_SAMPLETIME_239CYCLES5);
    ADC_EnableSoftwareStartConv(ADC1);
    while (ADC_ReadStatusFlag(ADC1, ADC_FLAG_EOC) == RESET) {
    }

    raw = ADC_ReadConversionValue(ADC1);
    ADC_ClearStatusFlag(ADC1, ADC_FLAG_EOC);

    return (raw * 3300U) / 4095U;
}

uint32_t board_target_voltage_sense_mv(void)
{
    uint32_t adc_mv;

    adc_mv = board_target_voltage_adc_mv();

    /* Divider: 4.7K(top) + 10K(bottom) => Vin = Vadc * 14.7/10 */
    return (adc_mv * 147U) / 100U;
}

bool board_target_voltage_is_present(uint32_t threshold_mv)
{
    return board_target_voltage_sense_mv() >= threshold_mv;
}

void board_request_bootloader(void)
{
    GPIO_Config_T gpioConfig;

    /* Match BlackMagic runtime behavior: force USB detach before reset. */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | BOOT_REQ_RCC);

    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = USB_PU_PIN;
    GPIO_Config(USB_PU_PORT, &gpioConfig);

    gpioConfig.mode = GPIO_MODE_IN_PD;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = USB_DP_PIN | USB_DM_PIN;
    GPIO_Config(USB_PORT, &gpioConfig);

    for (volatile uint32_t i = 0; i < (SystemCoreClock / 400U); i++) {
    }

    /* Drive boot request line so bootloader stays in DFU after reset. */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = BOOT_REQ_PIN;
    GPIO_Config(BOOT_REQ_PORT, &gpioConfig);
    GPIO_ResetBit(BOOT_REQ_PORT, BOOT_REQ_PIN);

    __DSB();
    SCB->AIRCR = ((0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
                  (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
                  SCB_AIRCR_VECTRESET_Msk);
    __DSB();
    while (1) {
    }
}

void board_boot_timer_init(void)
{
    TMR_BaseConfig_T tmr4cfg;

    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_TMR4);

    TMR_ConfigTimeBaseStructInit(&tmr4cfg);
    tmr4cfg.division   = (uint16_t)(SystemCoreClock / 10000U) - 1U;
    tmr4cfg.period     = 1000U - 1U;
    tmr4cfg.countMode  = TMR_COUNTER_MODE_UP;
    TMR_ConfigTimeBase(TMR4, &tmr4cfg);

    TMR_EnableInterrupt(TMR4, TMR_INT_UPDATE);
    NVIC_EnableIRQ(TMR4_IRQn);

    TMR_Enable(TMR4);
}


/*!
 * @brief       Initialize LED pins
 */
void board_led_init(void)
{
    GPIO_Config_T gpioConfig;
    
    /* Enable GPIO clock */
    RCM_EnableAPB2PeriphClock(LED_RCC);
    
    /* Configure LED pins as output push-pull */
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.pin = LED_RUNNING_PIN | LED_IDLE_PIN | LED_ERROR_PIN;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    GPIO_Config(LED_PORT, &gpioConfig);
    
    /* Turn off all LEDs initially */
    GPIO_SetBit(LED_PORT, LED_RUNNING_PIN | LED_IDLE_PIN | LED_ERROR_PIN);
}

/*!
 * @brief       Turn on LED
 * @param       led_num: 0=Running, 1=Idle, 2=Error
 */
void board_led_on(uint8_t led_num)
{
    switch(led_num) {
        case LED_RUNNING_NUM:
            GPIO_ResetBit(LED_PORT, LED_RUNNING_PIN);
            break;
        case LED_IDLE_NUM:
            GPIO_ResetBit(LED_PORT, LED_IDLE_PIN);
            break;
        case LED_ERROR_NUM:
            GPIO_ResetBit(LED_PORT, LED_ERROR_PIN);
            break;
    }
}

/*!
 * @brief       Turn off LED
 * @param       led_num: 0=Running, 1=Idle, 2=Error
 */
void board_led_off(uint8_t led_num)
{
    switch(led_num) {
        case LED_RUNNING_NUM:
            GPIO_SetBit(LED_PORT, LED_RUNNING_PIN);
            break;
        case LED_IDLE_NUM:
            GPIO_SetBit(LED_PORT, LED_IDLE_PIN);
            break;
        case LED_ERROR_NUM:
            GPIO_SetBit(LED_PORT, LED_ERROR_PIN);
            break;
    }
}

/*!
 * @brief       Toggle LED
 * @param       led_num: 0=Running, 1=Idle, 2=Error
 */
void board_led_toggle(uint8_t led_num)
{
    uint16_t pin;
    
    switch(led_num) {
        case LED_RUNNING_NUM:
            pin = LED_RUNNING_PIN;
            break;
        case LED_IDLE_NUM:
            pin = LED_IDLE_PIN;
            break;
        case LED_ERROR_NUM:
            pin = LED_ERROR_PIN;
            break;
        default:
            return;
    }
    
    if (GPIO_ReadOutputBit(LED_PORT, pin)) {
        GPIO_ResetBit(LED_PORT, pin);
    } else {
        GPIO_SetBit(LED_PORT, pin);
    }
}
