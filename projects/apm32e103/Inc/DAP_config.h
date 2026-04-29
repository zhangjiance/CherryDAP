/*
 * Copyright (c) 2024, CherryDAP for APM32E103
 * Pin definitions based on BlackMagic Native Hardware Version 3
 * (Mini V2.1a - compatible with HW3/4/5)
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include "stdint.h"
#include "board.h"
#include "apm32e10x_gpio.h"
#include "apm32e10x_rcm.h"

#ifndef   __STATIC_INLINE
#define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE                 
#define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline
#endif
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

/// Processor Clock
#define CPU_CLOCK               120000000U
#define IO_PORT_WRITE_CYCLES    2U
#define DELAY_SLOW_CYCLES       3U
#define DELAY_FAST_CYCLES       1U

/// SWD/JTAG Configuration
#define DAP_SWD                 1
#define DAP_JTAG                1
#define DAP_JTAG_DEV_CNT        8U
#define DAP_DEFAULT_PORT        1U
#define DAP_DEFAULT_SWJ_CLOCK   10000000U

/// USB Configuration
#define DAP_PACKET_SIZE         64U
#define DAP_PACKET_COUNT        8U

/// SWO Configuration
#define SWO_UART                0
#define SWO_BUFFER_SIZE         4096U
#define SWO_STREAM              0

/// Timestamp
#define TIMESTAMP_CLOCK         120000000U

/// Target Device
#define TARGET_DEVICE_FIXED     0
#define TARGET_DEVICE_VENDOR    ""
#define TARGET_DEVICE_NAME      ""

/// UART Configuration
#define DAP_UART                0
#define DAP_UART_USB_COM_PORT   0

/// SWO Manchester mode
#define SWO_MANCHESTER          0

/// Additional functions
__STATIC_INLINE uint8_t DAP_GetTargetBoardNameString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetTargetDeviceNameString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetTargetBoardVendorString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetVendorString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetProductString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetSerNumString (char *str) { (void)str; return (0U); }
__STATIC_INLINE uint8_t DAP_GetTargetDeviceVendorString (char *str) { (void)str; return (0U); }

// Pin Definitions (BlackMagic Native HW3 - Mini V2.1a compatible)
#define JTAG_TCK_PORT       GPIOA
#define JTAG_TCK_PIN        GPIO_PIN_5  /* PA5 - SWCLK/TCK */
#define JTAG_TMS_PORT       GPIOA
#define JTAG_TMS_PIN        GPIO_PIN_4  /* PA4 - SWDIO/TMS */
#define JTAG_TMS_DIR_PORT   GPIOA
#define JTAG_TMS_DIR_PIN    GPIO_PIN_1  /* PA1 - SWDIO direction control (HW3 buffer) */
#define JTAG_TDI_PORT       GPIOA
#define JTAG_TDI_PIN        GPIO_PIN_3  /* PA3 - TDI (HW3/4/5) */
#define JTAG_TDO_PORT       GPIOA
#define JTAG_TDO_PIN        GPIO_PIN_6  /* PA6 - TDO */
#define JTAG_TRST_PORT      GPIOC
#define JTAG_TRST_PIN       GPIO_PIN_13 /* PC13 - nTRST */
#define JTAG_NRST_PORT      GPIOA
#define JTAG_NRST_PIN       GPIO_PIN_2  /* PA2 - nRST (HW3/4/5) */

// Fast GPIO register access helpers
__STATIC_FORCEINLINE void GPIO_SET_FAST(GPIO_T *port, uint32_t pin_mask) {
    port->BSC = pin_mask;
}

__STATIC_FORCEINLINE void GPIO_CLR_FAST(GPIO_T *port, uint32_t pin_mask) {
    port->BSC = (pin_mask << 16U);
}

__STATIC_FORCEINLINE uint32_t GPIO_READ_FAST(GPIO_T *port, uint32_t pin_mask) {
    return (port->IDATA & pin_mask) ? 1U : 0U;
}

/* Native_plus style fixed SWDIO mode switching on PA4 (CFGLOW nibble #4). */
#define SWD_CR          (JTAG_TMS_PORT->CFGLOW)
#define SWD_CR_SHIFT    (4U << 2U)
#define SWD_CR_MASK     (0xFU << SWD_CR_SHIFT)
#define SWD_CR_FLOAT    (0x4U << SWD_CR_SHIFT) /* input floating */
#define SWD_CR_DRIVE    (0x3U << SWD_CR_SHIFT) /* output push-pull 50MHz */

// GPIO Macros
#define PIN_SET(port, pin)      GPIO_SET_FAST((port), (pin))
#define PIN_CLR(port, pin)      GPIO_CLR_FAST((port), (pin))
#define PIN_READ(port, pin)     GPIO_READ_FAST((port), (pin))

__STATIC_INLINE void PORT_JTAG_SETUP (void) {
    GPIO_Config_T gpioConfig;
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_GPIOC);
    
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = JTAG_TCK_PIN | JTAG_TMS_PIN | JTAG_TMS_DIR_PIN | JTAG_TDI_PIN | JTAG_NRST_PIN;
    GPIO_Config(GPIOA, &gpioConfig);
    gpioConfig.pin = JTAG_TRST_PIN;
    GPIO_Config(JTAG_TRST_PORT, &gpioConfig);
    
    gpioConfig.mode = GPIO_MODE_IN_PU;
    gpioConfig.pin = JTAG_TDO_PIN;
    GPIO_Config(JTAG_TDO_PORT, &gpioConfig);
    
    PIN_SET(JTAG_TCK_PORT, JTAG_TCK_PIN);
    PIN_SET(JTAG_TMS_PORT, JTAG_TMS_PIN);
    PIN_SET(JTAG_TMS_DIR_PORT, JTAG_TMS_DIR_PIN);
    PIN_SET(JTAG_TDI_PORT, JTAG_TDI_PIN);
    PIN_SET(JTAG_NRST_PORT, JTAG_NRST_PIN);
    PIN_SET(JTAG_TRST_PORT, JTAG_TRST_PIN);
}

__STATIC_INLINE void PORT_SWD_SETUP (void) {
    GPIO_Config_T gpioConfig;
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_GPIOC);
    
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = JTAG_TCK_PIN | JTAG_TMS_PIN | JTAG_TMS_DIR_PIN;
    GPIO_Config(JTAG_TCK_PORT, &gpioConfig);
    gpioConfig.pin = JTAG_NRST_PIN;
    GPIO_Config(JTAG_NRST_PORT, &gpioConfig);
    
    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.pin = JTAG_TDI_PIN | JTAG_TDO_PIN;
    GPIO_Config(GPIOA, &gpioConfig);
    gpioConfig.pin = JTAG_TRST_PIN;
    GPIO_Config(JTAG_TRST_PORT, &gpioConfig);
    
    PIN_SET(JTAG_TCK_PORT, JTAG_TCK_PIN);
    PIN_SET(JTAG_TMS_PORT, JTAG_TMS_PIN);
    PIN_SET(JTAG_TMS_DIR_PORT, JTAG_TMS_DIR_PIN);
    PIN_SET(JTAG_NRST_PORT, JTAG_NRST_PIN);
}

__STATIC_INLINE void PORT_OFF (void) {
    GPIO_Config_T gpioConfig;
    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.pin = JTAG_TCK_PIN | JTAG_TMS_PIN | JTAG_TMS_DIR_PIN | JTAG_TDI_PIN | JTAG_TDO_PIN | JTAG_NRST_PIN;
    GPIO_Config(GPIOA, &gpioConfig);
    gpioConfig.pin = JTAG_TRST_PIN;
    GPIO_Config(JTAG_TRST_PORT, &gpioConfig);

    /* SWDIO buffer direction: low=input/high-Z, high=output-drive */
    PIN_CLR(JTAG_TMS_DIR_PORT, JTAG_TMS_DIR_PIN);
}

__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN  (void) { return PIN_READ(JTAG_TCK_PORT, JTAG_TCK_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE void     PIN_SWCLK_TCK_SET (void) { PIN_SET(JTAG_TCK_PORT, JTAG_TCK_PIN); }
__STATIC_FORCEINLINE void     PIN_SWCLK_TCK_CLR (void) { PIN_CLR(JTAG_TCK_PORT, JTAG_TCK_PIN); }
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN  (void) { return PIN_READ(JTAG_TMS_PORT, JTAG_TMS_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE void     PIN_SWDIO_TMS_SET (void) { PIN_SET(JTAG_TMS_PORT, JTAG_TMS_PIN); }
__STATIC_FORCEINLINE void     PIN_SWDIO_TMS_CLR (void) { PIN_CLR(JTAG_TMS_PORT, JTAG_TMS_PIN); }
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN      (void) { return PIN_READ(JTAG_TMS_PORT, JTAG_TMS_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT     (uint32_t bit) { JTAG_TMS_PORT->BSC = (bit & 1U) ? JTAG_TMS_PIN : (JTAG_TMS_PIN << 16U); }

__STATIC_FORCEINLINE void     PIN_SWDIO_OUT_ENABLE  (void) {
    uint32_t cr = SWD_CR;
    cr &= ~SWD_CR_MASK;
    cr |= SWD_CR_DRIVE;
    /* native_plus ordering: set direction first, then switch pad to output. */
    PIN_SET(JTAG_TMS_DIR_PORT, JTAG_TMS_DIR_PIN);
    SWD_CR = cr;
    __NOP();
}

__STATIC_FORCEINLINE void     PIN_SWDIO_OUT_DISABLE (void) {
    uint32_t cr = SWD_CR;
    cr &= ~SWD_CR_MASK;
    cr |= SWD_CR_FLOAT;
    /* native_plus ordering: switch pad to input first, then release buffer direction. */
    SWD_CR = cr;
    PIN_CLR(JTAG_TMS_DIR_PORT, JTAG_TMS_DIR_PIN);
    __NOP();
}

__STATIC_FORCEINLINE uint32_t PIN_TDI_IN  (void) { return PIN_READ(JTAG_TDI_PORT, JTAG_TDI_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE void     PIN_TDI_OUT (uint32_t bit) { JTAG_TDI_PORT->BSC = (bit & 1U) ? JTAG_TDI_PIN : (JTAG_TDI_PIN << 16U); }
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN  (void) { return PIN_READ(JTAG_TDO_PORT, JTAG_TDO_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN   (void) { return PIN_READ(JTAG_TRST_PORT, JTAG_TRST_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE void     PIN_nTRST_OUT  (uint32_t bit) { JTAG_TRST_PORT->BSC = (bit & 1U) ? JTAG_TRST_PIN : (JTAG_TRST_PIN << 16U); }
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN  (void) { return PIN_READ(JTAG_NRST_PORT, JTAG_NRST_PIN) ? 1U : 0U; }
__STATIC_FORCEINLINE void     PIN_nRESET_OUT (uint32_t bit) { JTAG_NRST_PORT->BSC = (bit & 1U) ? JTAG_NRST_PIN : (JTAG_NRST_PIN << 16U); }

static uint8_t g_dap_led_connected = 0U;
static uint8_t g_dap_led_running = 0U;
static uint8_t g_dap_led_error = 0U;

__STATIC_INLINE void DAP_LED_UPDATE (void) {
    if (g_dap_led_connected) {
        if (g_dap_led_running) {
            board_led_on(0);  /* running */
            board_led_off(1); /* idle */
        } else {
            board_led_off(0);
            board_led_on(1);
        }

        if (g_dap_led_error) {
            board_led_on(2);
        } else {
            board_led_off(2);
        }
    } else {
        board_led_off(0);
        board_led_off(1);
        board_led_off(2);
    }
}

__STATIC_INLINE void LED_CONNECTED_OUT (uint32_t bit) {
    g_dap_led_connected = (uint8_t)(bit & 1U);

    if (g_dap_led_connected == 0U) {
        g_dap_led_running = 0U;
        g_dap_led_error = 0U;
    }

    DAP_LED_UPDATE();
}

__STATIC_INLINE void LED_RUNNING_OUT   (uint32_t bit) {
    g_dap_led_running = (uint8_t)(bit & 1U);
    DAP_LED_UPDATE();
}
__STATIC_INLINE uint32_t TIMESTAMP_GET (void) { return (0U); }

__STATIC_INLINE void DAP_SETUP (void) {
    board_init();
    board_led_init();
    g_dap_led_connected = 0U;
    g_dap_led_running = 0U;
    g_dap_led_error = 0U;
    DAP_LED_UPDATE();
    PORT_OFF();
}

__STATIC_INLINE uint8_t RESET_TARGET (void) { return (0U); }

#endif /* __DAP_CONFIG_H__ */
