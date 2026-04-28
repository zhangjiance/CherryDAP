/*
 * Copyright (c) 2024, CherryDAP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include "apm32e10x.h"
#include <stdio.h>
#include <stdlib.h>

#define CHERRYUSB_VERSION_DEF 0x001100

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

#define usb_malloc(size) malloc(size)
#define usb_free(ptr)    free(ptr)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* Data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* ================ USB Device Configuration ================ */

/* Number of USB buses */
#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif

/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512

/* Setup packet log for debug */
/* #define CONFIG_USBDEV_SETUP_LOG_PRINT */

/* Check if the input descriptor is correct */
/* #define CONFIG_USBDEV_DESC_CHECK */

/* Use struct usb_descriptor registration API (same as HSLink-Pro) */
#define CONFIG_USBDEV_ADVANCE_DESC

/* Enable test mode */
/* #define CONFIG_USBDEV_TEST_MODE */

/* Thread stack size */
#define CONFIG_USBDEV_THREAD_STACK_SIZE 2048

/* ================ USB Device Port Configuration ================ */
#define CONFIG_USBDEV_EP_NUM 8

/* FSDEV Configuration for APM32/STM32 Full Speed Device */
#define CONFIG_USBDEV_FSDEV_PMA_ACCESS 2

#ifndef CONFIG_USB_FSDEV_RAM_SIZE
#define CONFIG_USB_FSDEV_RAM_SIZE 512
#endif

/* ================ USB CDC ACM Configuration ================ */
#define CONFIG_USBDEV_CDC_ACM_EP_NUM 8

/* CDC ACM buffer size */
#ifndef CONFIG_USBDEV_CDC_ACM_TX_BUFSIZE
#define CONFIG_USBDEV_CDC_ACM_TX_BUFSIZE 2048
#endif

#ifndef CONFIG_USBDEV_CDC_ACM_RX_BUFSIZE
#define CONFIG_USBDEV_CDC_ACM_RX_BUFSIZE 2048
#endif

/* ================ USB HID Configuration ================ */
#ifndef CONFIG_USBDEV_HID_REPORT_DESC_MAX_LENGTH
#define CONFIG_USBDEV_HID_REPORT_DESC_MAX_LENGTH 256
#endif

/* ================ USB MSC Configuration ================ */
#define CONFIG_USBDEV_MSC_THREAD_STACK_SIZE 2048

#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#endif

/* MSC Manufacturer */
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING "CherryUSB"

/* MSC Product */
#define CONFIG_USBDEV_MSC_PRODUCT_STRING "CherryUSB MSC"

/* MSC Serial */
#define CONFIG_USBDEV_MSC_SERIAL_STRING "0123456789AB"

/* MSC Version */
#define CONFIG_USBDEV_MSC_VERSION_STRING "0.01"

/* ================ USB Memory Configuration ================ */
/* Attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))

/* ================ USB Device Register Base ================ */
extern uint32_t SystemCoreClock;

/* Rename USBD_BASE macro to avoid conflict */
#undef USBD_BASE
#define USB_BASE_ADDR (0x40005C00UL)

#endif /* CHERRYUSB_CONFIG_H */
