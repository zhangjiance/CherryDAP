/*!
 * @file       usb_osbdm.h
 *
 * @brief      OSBDM USB Device interface for APM32E103
 *
 * @note       USB configuration for OSBDM protocol support
 */

#ifndef __USB_OSBDM_H
#define __USB_OSBDM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_core.h"
#include <stdint.h>

/* USB Device VID/PID - Freescale OSBDM compatible IDs */
#define USBD_VID                    0x15A2
#define USBD_PID                    0x005E
#define USBD_MAX_POWER              100

/* OSBDM USB Endpoint addresses */
#define OSBDM_IN_EP                 0x82    /* EP2 IN */
#define OSBDM_OUT_EP                0x01    /* EP1 OUT */

/* OSBDM USB Endpoint packet sizes */
#define OSBDM_EP_MPS_FS             64      /* Full Speed: 64 bytes */
#define OSBDM_EP_MPS_HS             512     /* High Speed: 512 bytes (not used on APM32E103) */

/* USB buffer size - use full speed max packet size */
#define USB_OSBDM_BUFSIZE           OSBDM_EP_MPS_FS

/**
 * @brief OSBDM receive buffer (command from host)
 */
extern uint8_t g_usb_osbdm_rx_buf[USB_OSBDM_BUFSIZE];

/**
 * @brief OSBDM transmit buffer (response to host)
 */
extern uint8_t g_usb_osbdm_tx_buf[USB_OSBDM_BUFSIZE];

/**
 * @brief Initialize OSBDM USB device
 */
void usb_osbdm_init(void);

/**
 * @brief Send data through OSBDM IN endpoint
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return 0 on success, negative on error
 */
int32_t usb_osbdm_send(uint8_t *data, uint32_t len);

/**
 * @brief Send data through OSBDM IN endpoint (compatibility alias)
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return 0 on success, negative on error
 */
int32_t usb_osbdm_ep_in_send(uint8_t *data, uint32_t len);

/**
 * @brief Re-arm OSBDM OUT endpoint to receive next host command
 */
void usb_osbdm_rearm_out(void);

/**
 * @brief Poll USB runtime tasks (DFU runtime detach handling)
 */
void usb_osbdm_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_OSBDM_H */
