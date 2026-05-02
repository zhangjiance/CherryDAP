/*!
 * @file       usb_config.h
 *
 * @brief      CherryUSB configuration for OSBDM on APM32E103
 */

#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include "apm32e10x.h"
#include <stdio.h>
#include <stdlib.h>

#define CHERRYUSB_VERSION_DEF 0x001100

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) 

#define usb_malloc(size) malloc(size)
#define usb_free(ptr)    free(ptr)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_ERROR
#endif

/* Data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* ================ USB Device Configuration ================ */

/* Number of USB buses */
#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif

/* Ep0 max transfer buffer */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 256

/* Use struct usb_descriptor registration API */
#define CONFIG_USBDEV_ADVANCE_DESC

/* Max number of interfaces */
#ifndef CONFIG_USBDEV_MAX_INTF
#define CONFIG_USBDEV_MAX_INTF 2
#endif

/* ================ USB Device Port Configuration ================ */

/* APM32E103 Full-Speed USB Device controller */
#define CONFIG_USB_FS

/* USB endpoint count (EP0 + 7 bidirectional endpoints) */
#ifndef CONFIG_USBDEV_EP_NUM
#define CONFIG_USBDEV_EP_NUM 8
#endif

/* USB endpoint buffer size */
#ifndef CONFIG_USBDEV_FSDEV_PMA_ACCESS
#define CONFIG_USBDEV_FSDEV_PMA_ACCESS 2
#endif

/* Rename USBD_BASE macro to avoid conflict and match CherryDAP setup */
#undef USBD_BASE
#define USB_BASE_ADDR (0x40005C00UL)

/* ================ Memory Configuration ================ */

/* Descriptor buffers - non-cached RAM sections */
#define USB_NOCACHE_RAM_SECTION 
#undef USB_MEM_ALIGNX
#define USB_MEM_ALIGNX __attribute__((aligned(CONFIG_USB_ALIGN_SIZE)))

/* ================ Bulk Endpoint Sizes ================ */

#ifndef USB_BULK_EP_MPS_FS
#define USB_BULK_EP_MPS_FS  64
#endif

#ifndef USB_BULK_EP_MPS_HS
#define USB_BULK_EP_MPS_HS  512
#endif

#endif /* CHERRYUSB_CONFIG_H */
