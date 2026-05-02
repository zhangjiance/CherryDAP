/*!
 * @file       main.c
 *
 * @brief      Main application for OSBDM on APM32E103
 *
 * @note       OSBDM debug interface for multiple target architectures
 */

#include "apm32e10x.h"
#include "apm32e10x_fmc.h"
#include "apm32e10x_misc.h"
#include "board_osbdm.h"
#include "usb_osbdm.h"
#include "board_id.h"
#include "cmd_processing.h"
#include "targetAPI.h"
#include <stdio.h>

/* Target driver init is implemented by the selected OSBDM driver. */
void t_debug_init(void);

/* System tick counter */
static volatile uint32_t systick_counter = 0;

/* Target voltage monitoring */
static volatile uint16_t target_voltage_mv = 0;

/* Activity LED blink state */
static volatile uint32_t led_blink_timer = 0;
static volatile uint8_t led_blink_state = 0;

/**
 * @brief System clock initialization
 */
static void system_clock_init(void)
{
    RCM_Reset();
    RCM_ConfigHSE(RCM_HSE_OPEN);

    if (RCM_WaitHSEReady() == SUCCESS) {
        FMC_EnablePrefetchBuffer();
        FMC_ConfigLatency(FMC_LATENCY_2);

        RCM_ConfigAHB(RCM_AHB_DIV_1);
        RCM_ConfigAPB2(RCM_APB_DIV_1);
        RCM_ConfigAPB1(RCM_APB_DIV_2);

        /* HSE = 8MHz, PLL x15 => SYSCLK = 120MHz */
        RCM_ConfigPLL(RCM_PLLSEL_HSE, RCM_PLLMF_15);
        RCM_EnablePLL();
        while (RCM_ReadStatusFlag(RCM_FLAG_PLLRDY) == RESET);

        RCM_ConfigSYSCLK(RCM_SYSCLK_SEL_PLL);
        while (RCM_ReadSYSCLKSource() != RCM_SYSCLK_SEL_PLL);

        /* USB clock = PLL / 2.5 = 120MHz / 2.5 = 48MHz */
        RCM_ConfigUSBCLK(RCM_USB_DIV_2_5);

        SystemCoreClockUpdate();
        RCM_EnableCSS();
    }
}

/**
 * @brief Board initialization
 */
static void board_init(void)
{
    /* Set vector table offset for bootloader compatibility */
    NVIC_ConfigVectorTable(NVIC_VECT_TAB_FLASH, 0x2000);  /* 8KB bootloader offset */
    
    /* Initialize system clock */
    system_clock_init();
    
    /* Enable peripheral clocks */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_AFIO |
                              RCM_APB2_PERIPH_GPIOA |
                              RCM_APB2_PERIPH_GPIOB |
                              RCM_APB2_PERIPH_GPIOC);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USB);
    
    /* Configure SysTick for 1ms interrupts */
    SysTick_Config(SystemCoreClock / 1000);
}

/**
 * @brief SysTick interrupt handler
 */
void SysTick_Handler(void)
{
    systick_counter++;
    
    /* Update LED blink timer */
    if (led_blink_timer > 0) {
        led_blink_timer--;
    }
}

/**
 * @brief Get system tick count in milliseconds
 */
uint32_t systick_get(void)
{
    return systick_counter;
}

/**
 * @brief Process LED error indication
 * 
 * Red LED indicates error conditions:
 * - Target voltage too low (< 1.0V)
 * - Reset line mismatch (output asserted but input not responding)
 */
static void led_error_process(void)
{
    /* Read target voltage periodically */
    static uint32_t last_voltage_check = 0;
    if ((systick_counter - last_voltage_check) >= 100) {
        target_voltage_mv = board_target_voltage_read();
        last_voltage_check = systick_counter;
    }
    
    /* Check for error conditions */
    if (target_voltage_mv < 1000) {
        /* Target voltage too low */
        LED_RED_ON();
    } else {
        LED_RED_OFF();
    }
}

/**
 * @brief Process LED status indication
 * 
 * Green LED blinks briefly when commands are executed
 */
static void led_status_process(void)
{
    switch (led_blink_state) {
        case 0:  /* Idle - LED on */
            if (debug_cmd_pending) {
                LED_GREEN_OFF();
                led_blink_timer = 50;  /* 50ms off */
                led_blink_state = 1;
            }
            break;
            
        case 1:  /* Command active - LED off */
            if (led_blink_timer == 0) {
                LED_GREEN_ON();
                led_blink_timer = 10;  /* 10ms on */
                led_blink_state = 2;
            }
            break;
            
        case 2:  /* Brief on pulse */
            if (led_blink_timer == 0) {
                led_blink_state = 0;
            }
            break;
    }
}

/**

 * @brief Main application entry point
 */
int main(void)
{
    /* Initialize board hardware */
    board_init();
    
    /* Initialize OSBDM GPIO pins */
    board_osbdm_gpio_init();
    
    /* Initialize target power control and ADC */
    board_target_power_init();
    
    /* Initialize USB OSBDM device */
    usb_osbdm_init();
    
    /* Initialize OSBDM protocol layer */
    read_board_id();    /* Returns board ID (can be customized) */
    read_osbdm_id();    /* Returns OSBDM firmware version */
    t_debug_init();     /* Initialize debug interface to safe state */
    
    /* Turn on green LED to indicate ready */
    LED_GREEN_ON();
    LED_RED_OFF();
    LED_ORANGE_OFF();
    
    /* Enable target power by default */
    board_target_power_set(true);
    
    /* Main loop */
    while (1) {
        /* Monitor error conditions and update red LED */
        led_error_process();
        
        /* Update status LED (green) */
        led_status_process();
        
        /* Process OSBDM commands when received */
        if (debug_cmd_pending) {
            /* Execute the command */
            debug_command_exec();
            
            /* Clear pending flag */
            debug_cmd_pending = 0;
        }
    }
    
    return 0;
}

/**
 * @brief Minimal printf support for debugging (optional)
 * 
 * Redirect printf to nowhere if not needed, or implement UART output
 */
int fputc(int ch, FILE *f)
{
    (void)f;
    /* Could redirect to UART for debugging */
    return ch;
}

int fgetc(FILE *f)
{
    (void)f;
    return 0;
}
