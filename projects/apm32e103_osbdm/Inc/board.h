/*!
 * @file       board.h
 *
 * @brief      Board configuration for CherryDAP on APM32E103
 *
 * @note       Pin definitions based on BlackMagic Native Hardware Version 3
 *             (Mini V2.1a - compatible with HW3/4/5, before HW6 redesign)
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apm32e10x.h"
#include "apm32e10x_gpio.h"
#include "apm32e10x_rcm.h"
#include "apm32e10x_misc.h"
#include <stdbool.h>

/* LED Pin Definitions - BlackMagic Native HW3 */
#define LED_PORT            GPIOB
#define LED_RCC             RCM_APB2_PERIPH_GPIOB

#define LED_ERROR_PIN       GPIO_PIN_2   /* PB2 - LED0 Red (Error) */
#define LED_RUNNING_PIN     GPIO_PIN_10  /* PB10 - LED1 GREEN (Running) */
#define LED_IDLE_PIN        GPIO_PIN_11  /* PB11 - LED2 BLUE (Idle) */

#define LED_ERROR_NUM        0
#define LED_RUNNING_NUM      1
#define LED_IDLE_NUM         2

/* JTAG/SWD Pin Definitions - BlackMagic Native HW3 */
#define JTAG_PORT           GPIOA
#define JTAG_RCC            RCM_APB2_PERIPH_GPIOA

#define TDI_PIN             GPIO_PIN_3   /* PA3 (HW3/4/5), PA7 on HW6+ */
#define TMS_SWDIO_PIN       GPIO_PIN_4   /* PA4 (all versions) */
#define TCK_SWCLK_PIN       GPIO_PIN_5   /* PA5 (all versions) */
#define TDO_PIN             GPIO_PIN_6   /* PA6 (all versions) */
#define NRST_PIN            GPIO_PIN_2   /* PA2 (HW3/4/5), PA9 on HW6+ */
#define TMS_DIR_PORT        GPIOA
#define TMS_DIR_PIN         GPIO_PIN_1   /* PA1 - level shifter direction control */

#define TRST_PORT           GPIOC
#define TRST_PIN            GPIO_PIN_13  /* PC13 (actual hardware) */

/* UART Pin Definitions - BlackMagic Native HW3 */
#define UART_TX_PORT        GPIOA
#define UART_TX_PIN         GPIO_PIN_9   /* PA9 - USART1_TX (HW3/4/5), PA2/USART2 on HW6+ */
#define UART_RX_PORT        GPIOA  
#define UART_RX_PIN         GPIO_PIN_10  /* PA10 - USART1_RX (HW3/4/5), PA3/USART2 on HW6+ */

/* USB Pin Definitions - All HW versions */
#define USB_PORT            GPIOA
#define USB_DP_PIN          GPIO_PIN_12  /* PA12 - USB D+ */
#define USB_DM_PIN          GPIO_PIN_11  /* PA11 - USB D- */
#define USB_PU_PORT         GPIOA
#define USB_PU_PIN          GPIO_PIN_8   /* PA8 - USB Pull-up control */

/* Bootloader request pin (BlackMagic native_plus style) */
#define BOOT_REQ_PORT        GPIOB
#define BOOT_REQ_PIN         GPIO_PIN_12
#define BOOT_REQ_RCC         RCM_APB2_PERIPH_GPIOB

/* Target power switch (compatible with BlackMagic native HW3 tpwr control) */
#define TPWR_CTRL_PORT      GPIOB
#define TPWR_CTRL_PIN       GPIO_PIN_1   /* PB1 - tpwr bridge control */
#define TPWR_CTRL_RCC       RCM_APB2_PERIPH_GPIOB

/* Target voltage sense (BlackMagic native style): PB0 -> ADC1 CH8, divider 4.7K + 10K */
#define TPWR_SENSE_PORT     GPIOB
#define TPWR_SENSE_PIN      GPIO_PIN_0
#define TPWR_SENSE_RCC      RCM_APB2_PERIPH_GPIOB

/* JTAG buffer direction control */
#define TMS_DIR_OUT()       do { TMS_DIR_PORT->BSC = TMS_DIR_PIN; __DSB(); } while (0)
#define TMS_DIR_IN()        do { TMS_DIR_PORT->BC = TMS_DIR_PIN; __DSB(); } while (0)

/* OSBDM/JTAG compatibility macros expected by legacy driver sources */
#define TCK_SET()           do { JTAG_PORT->BSC = TCK_SWCLK_PIN; __DSB(); } while (0)
#define TCK_RESET()         do { JTAG_PORT->BC = TCK_SWCLK_PIN; __DSB(); } while (0)
#define TMS_SET()           do { TMS_DIR_OUT(); JTAG_PORT->BSC = TMS_SWDIO_PIN; __DSB(); } while (0)
#define TMS_RESET()         do { TMS_DIR_OUT(); JTAG_PORT->BC = TMS_SWDIO_PIN; __DSB(); } while (0)
#define TDI_SET()           do { JTAG_PORT->BSC = TDI_PIN; __DSB(); } while (0)
#define TDI_RESET()         do { JTAG_PORT->BC = TDI_PIN; __DSB(); } while (0)
#define TDO_READ()          ((JTAG_PORT->IDATA & TDO_PIN) ? 1U : 0U)
#define TDO_GET()           TDO_READ()

#define TRST_SET()          do { TRST_PORT->BSC = TRST_PIN; __DSB(); } while (0)
#define TRST_RESET()        do { TRST_PORT->BC = TRST_PIN; __DSB(); } while (0)
#define TRST_ASSERT()       TRST_RESET()
#define TRST_DEASSERT()     TRST_SET()

#define SRST_SET()          do { JTAG_PORT->BSC = NRST_PIN; __DSB(); } while (0)
#define SRST_RESET()        do { JTAG_PORT->BC = NRST_PIN; __DSB(); } while (0)
/* Match OSBDM eppc expectation from 5301 port: SRST control is inverted by hardware path. */
#define SRST_ASSERT()       SRST_SET()
#define SRST_DEASSERT()     SRST_RESET()

#define TCK_HIGH()          TCK_SET()
#define TCK_LOW()           TCK_RESET()
#define TMS_HIGH()          TMS_SET()
#define TMS_LOW()           TMS_RESET()
#define TDI_HIGH()          TDI_SET()
#define TDI_LOW()           TDI_RESET()

/* Function prototypes */
void board_init(void);
void board_led_init(void);
void board_led_on(uint8_t led_num);
void board_led_off(uint8_t led_num);
void board_led_toggle(uint8_t led_num);
void board_target_power_init(void);
void board_target_power_set(bool enable);
bool board_target_power_get(void);
uint32_t board_target_voltage_adc_mv(void);
uint32_t board_target_voltage_sense_mv(void);
bool board_target_voltage_is_present(uint32_t threshold_mv);
void board_request_bootloader(void);
void board_boot_timer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
