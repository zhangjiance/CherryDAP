/*!
 * @file       board_osbdm.h
 *
 * @brief      Board GPIO configuration for OSBDM on APM32E103
 *
 * @note       Pin mappings compatible with BlackMagic Native HW3
 *             JTAG/BDM signals mapped to existing debug pins
 */

#ifndef __BOARD_OSBDM_H
#define __BOARD_OSBDM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apm32e10x.h"
#include "apm32e10x_gpio.h"
#include "apm32e10x_rcm.h"
#include <stdbool.h>
#include <stdint.h>

/* JTAG/BDM Pin Definitions - Reuse existing SWD pins */
#define JTAG_PORT           GPIOA
#define JTAG_RCC            RCM_APB2_PERIPH_GPIOA

/* JTAG signal pins */
#define TDI_PORT            GPIOA
#define TDI_PIN             GPIO_PIN_3   /* PA3 */
#define TDI_PIN_NUM         3

#define TMS_PORT            GPIOA
#define TMS_PIN             GPIO_PIN_4   /* PA4 - SWDIO/TMS */
#define TMS_PIN_NUM         4

#define TCK_PORT            GPIOA
#define TCK_PIN             GPIO_PIN_5   /* PA5 - SWCLK/TCK */
#define TCK_PIN_NUM         5

#define TDO_PORT            GPIOA
#define TDO_PIN             GPIO_PIN_6   /* PA6 */
#define TDO_PIN_NUM         6

/* Reset signals */
#define TRST_PORT           GPIOC
#define TRST_PIN            GPIO_PIN_13  /* PC13 */
#define TRST_PIN_NUM        13
#define TRST_RCC            RCM_APB2_PERIPH_GPIOC

#define SRST_OUT_PORT       GPIOA
#define SRST_OUT_PIN        GPIO_PIN_2   /* PA2 - nRST */
#define SRST_OUT_PIN_NUM    2

#define SRST_IN_PORT        GPIOA
#define SRST_IN_PIN         GPIO_PIN_2   /* PA2 - same pin for input/output */
#define SRST_IN_PIN_NUM     2

/* Target power control */
#define PWR_PORT            GPIOB
#define PWR_PIN             GPIO_PIN_1   /* PB1 - Target power control */
#define PWR_PIN_NUM         1
#define PWR_RCC             RCM_APB2_PERIPH_GPIOB

/* Target voltage sense */
#define VREF_PORT           GPIOB
#define VREF_PIN            GPIO_PIN_0   /* PB0 - ADC input */
#define VREF_PIN_NUM        0
#define VREF_RCC            RCM_APB2_PERIPH_GPIOB

/* LED indicators */
#define LED_PORT            GPIOB
#define LED_RCC             RCM_APB2_PERIPH_GPIOB
#define LED_GREEN_PIN       GPIO_PIN_2   /* PB2 - Yellow/Running */
#define LED_RED_PIN         GPIO_PIN_11  /* PB11 - Error */
#define LED_ORANGE_PIN      GPIO_PIN_10  /* PB10 - Idle/Activity */

/*==============================================================================
 * GPIO Control Macros - Direct register access for performance
 *============================================================================*/

/* TCK (PA5) control */
#define TCK_SET()           do { GPIOA->BSC = GPIO_PIN_5; __DSB(); } while(0)
#define TCK_RESET()         do { GPIOA->BC = GPIO_PIN_5; __DSB(); } while(0)
#define TCK_HIGH()          TCK_SET()
#define TCK_LOW()           TCK_RESET()

/* TMS (PA4) control */
#define TMS_SET()           do { GPIOA->BSC = GPIO_PIN_4; __DSB(); } while(0)
#define TMS_RESET()         do { GPIOA->BC = GPIO_PIN_4; __DSB(); } while(0)
#define TMS_HIGH()          TMS_SET()
#define TMS_LOW()           TMS_RESET()

/* TDI (PA3) control */
#define TDI_SET()           do { GPIOA->BSC = GPIO_PIN_3; __DSB(); } while(0)
#define TDI_RESET()         do { GPIOA->BC = GPIO_PIN_3; __DSB(); } while(0)
#define TDI_HIGH()          TDI_SET()
#define TDI_LOW()           TDI_RESET()

/* TDO (PA6) read */
#define TDO_READ()          ((GPIOA->IDATA & GPIO_PIN_6) ? 1 : 0)
#define TDO_GET()           TDO_READ()

/* TRST (PC13) control */
#define TRST_SET()          do { GPIOC->BSC = GPIO_PIN_13; __DSB(); } while(0)
#define TRST_RESET()        do { GPIOC->BC = GPIO_PIN_13; __DSB(); } while(0)
#define TRST_HIGH()         TRST_SET()
#define TRST_LOW()          TRST_RESET()
#define TRST_ASSERT()       TRST_LOW()    /* Active low */
#define TRST_DEASSERT()     TRST_HIGH()

/* SRST (PA2) control - bidirectional */
#define SRST_OUT_SET()      do { GPIOA->BSC = GPIO_PIN_2; __DSB(); } while(0)
#define SRST_OUT_RESET()    do { GPIOA->BC = GPIO_PIN_2; __DSB(); } while(0)
#define SRST_OUT_HIGH()     SRST_OUT_SET()
#define SRST_OUT_LOW()      SRST_OUT_RESET()
#define SRST_IN_READ()      ((GPIOA->IDATA & GPIO_PIN_2) ? 1 : 0)
#define SRST_ASSERT()       SRST_OUT_LOW()    /* Active low */
#define SRST_DEASSERT()     SRST_OUT_HIGH()

/* Target power control (PB1) */
#define PWR_ENABLE()        do { GPIOB->BSC = GPIO_PIN_1; } while(0)
#define PWR_DISABLE()       do { GPIOB->BC = GPIO_PIN_1; } while(0)

/* LED control */
#define LED_GREEN_ON()      do { GPIOB->BC = GPIO_PIN_2; } while(0)
#define LED_GREEN_OFF()     do { GPIOB->BSC = GPIO_PIN_2; } while(0)
#define LED_GREEN_TOGGLE()  do { GPIOB->ODATA ^= GPIO_PIN_2; } while(0)

#define LED_RED_ON()        do { GPIOB->BC = GPIO_PIN_11; } while(0)
#define LED_RED_OFF()       do { GPIOB->BSC = GPIO_PIN_11; } while(0)
#define LED_RED_TOGGLE()    do { GPIOB->ODATA ^= GPIO_PIN_11; } while(0)

#define LED_ORANGE_ON()     do { GPIOB->BC = GPIO_PIN_10; } while(0)
#define LED_ORANGE_OFF()    do { GPIOB->BSC = GPIO_PIN_10; } while(0)
#define LED_ORANGE_TOGGLE() do { GPIOB->ODATA ^= GPIO_PIN_10; } while(0)

/* Direction control for bidirectional pins (if needed for open-drain) */
#define TMS_DIR_OUT()       do { \
    GPIO_ConfigPinMode(TMS_PORT, TMS_PIN, GPIO_MODE_OUT_PP); \
    GPIO_ConfigPinSpeed(TMS_PORT, TMS_PIN, GPIO_SPEED_50MHz); \
} while(0)

#define TMS_DIR_IN()        do { \
    GPIO_ConfigPinMode(TMS_PORT, TMS_PIN, GPIO_MODE_IN_FLOATING); \
} while(0)

#define TDI_DIR_OUT()       do { \
    GPIO_ConfigPinMode(TDI_PORT, TDI_PIN, GPIO_MODE_OUT_PP); \
    GPIO_ConfigPinSpeed(TDI_PORT, TDI_PIN, GPIO_SPEED_50MHz); \
} while(0)

#define TDI_DIR_IN()        do { \
    GPIO_ConfigPinMode(TDI_PORT, TDI_PIN, GPIO_MODE_IN_FLOATING); \
} while(0)

#define TCK_DIR_OUT()       do { \
    GPIO_ConfigPinMode(TCK_PORT, TCK_PIN, GPIO_MODE_OUT_PP); \
    GPIO_ConfigPinSpeed(TCK_PORT, TCK_PIN, GPIO_SPEED_50MHz); \
} while(0)

#define SRST_DIR_OUT()      do { \
    GPIO_ConfigPinMode(SRST_OUT_PORT, SRST_OUT_PIN, GPIO_MODE_OUT_OD); \
    GPIO_ConfigPinSpeed(SRST_OUT_PORT, SRST_OUT_PIN, GPIO_SPEED_50MHz); \
} while(0)

#define SRST_DIR_IN()       do { \
    GPIO_ConfigPinMode(SRST_IN_PORT, SRST_IN_PIN, GPIO_MODE_IN_FLOATING); \
} while(0)

/*==============================================================================
 * Function Prototypes
 *============================================================================*/

/**
 * @brief Initialize board GPIOs for OSBDM operation
 */
void board_osbdm_gpio_init(void);

/**
 * @brief Initialize JTAG/BDM pins to default state
 */
void board_jtag_pins_init(void);

/**
 * @brief Initialize target power control
 */
void board_target_power_init(void);

/**
 * @brief Set target power state
 * @param enable true to enable power, false to disable
 */
void board_target_power_set(bool enable);

/**
 * @brief Read target voltage (via ADC)
 * @return Voltage in millivolts (0-3300mV range)
 */
uint16_t board_target_voltage_read(void);

/**
 * @brief Check if target power is good
 * @return true if voltage > 1.0V
 */
bool board_target_power_good(void);

/**
 * @brief Microsecond delay
 * @param us Delay in microseconds
 */
void delay_us(uint32_t us);

/**
 * @brief Millisecond delay
 * @param ms Delay in milliseconds
 */
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_OSBDM_H */
