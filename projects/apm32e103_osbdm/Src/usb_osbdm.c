/*!
 * @file       usb_osbdm.c
 *
 * @brief      OSBDM USB Device implementation for APM32E103
 *
 * @note       USB device configuration and callbacks
 */

#include "usb_osbdm.h"
#include "usbd_core.h"
#include "cmd_processing.h"
#include <string.h>

/* USB Bus ID */
#define BUSID                       0

/* Configuration descriptor size */
#define CONFIG_DESC_SIZE            (9 + 9 + 7 + 7)  /* Config + Interface + 2 Endpoints */
#define INTERFACE_NUM               1

/*==============================================================================
 * USB Buffers
 *============================================================================*/

/* OSBDM receive buffer - aligned for USB DMA */
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_usb_osbdm_rx_buf[USB_OSBDM_BUFSIZE];

/* OSBDM transmit buffer - aligned for USB DMA */
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_usb_osbdm_tx_buf[USB_OSBDM_BUFSIZE];

/*==============================================================================
 * USB Descriptors
 *============================================================================*/

/* Device Descriptor */
static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(
        USB_2_0,        /* bcdUSB */
        0x00,           /* bDeviceClass - Defined at interface level */
        0x00,           /* bDeviceSubClass */
        0x00,           /* bDeviceProtocol */
        USBD_VID,       /* idVendor */
        USBD_PID,       /* idProduct */
        0x0100,         /* bcdDevice */
        0x01            /* bNumConfigurations */
    ),
};

/* Configuration Descriptor (Full Speed) */
static const uint8_t config_descriptor_fs[] = {
    /* Configuration Descriptor */
    USB_CONFIG_DESCRIPTOR_INIT(
        CONFIG_DESC_SIZE,
        INTERFACE_NUM,
        0x01,                           /* bConfigurationValue */
        USB_CONFIG_BUS_POWERED,
        USBD_MAX_POWER
    ),
    
    /* Interface Descriptor 0 - OSBDM Bulk Interface */
    USB_INTERFACE_DESCRIPTOR_INIT(
        0x00,           /* bInterfaceNumber */
        0x00,           /* bAlternateSetting */
        0x02,           /* bNumEndpoints */
        0x00,           /* bInterfaceClass - Generic */
        0x00,           /* bInterfaceSubClass */
        0x00,           /* bInterfaceProtocol */
        0x02            /* iInterface */
    ),
    
    /* Endpoint Descriptor - Bulk IN (EP2) */
    USB_ENDPOINT_DESCRIPTOR_INIT(
        OSBDM_IN_EP,
        USB_ENDPOINT_TYPE_BULK,
        OSBDM_EP_MPS_FS,
        0x00
    ),
    
    /* Endpoint Descriptor - Bulk OUT (EP1) */
    USB_ENDPOINT_DESCRIPTOR_INIT(
        OSBDM_OUT_EP,
        USB_ENDPOINT_TYPE_BULK,
        OSBDM_EP_MPS_FS,
        0x00
    ),
};

/* Device Qualifier Descriptor (for High Speed capable devices) */
static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(
        USB_2_0,
        0x00,
        0x00,
        0x00,
        0x01
    ),
};

/* String Descriptors */
static const char *string_descriptors[] = {
    "\x09\x04",                     /* LangID: 0x0409 (US English) */
    "ARM",                          /* Manufacturer */
    "OSBDM Debug Port",             /* Product */
    "OSBDM001",                     /* Serial Number */
};

/*==============================================================================
 * USB Descriptor Callbacks
 *============================================================================*/

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor_fs;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor_fs;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    
    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

/* USB Descriptor structure */
static const struct usb_descriptor osbdm_descriptor = {
    .device_descriptor_callback         = device_descriptor_callback,
    .config_descriptor_callback         = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback    = other_speed_config_descriptor_callback,
    .string_descriptor_callback         = string_descriptor_callback,
};

/*==============================================================================
 * USB Endpoint Callbacks
 *============================================================================*/

/**
 * @brief OSBDM IN endpoint callback (after data sent to host)
 */
static void osbdm_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    /* Send zero-length packet if needed (when nbytes is multiple of MPS) */
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        usbd_ep_start_write(busid, ep, NULL, 0);
    }
}

/**
 * @brief OSBDM OUT endpoint callback (after receiving data from host)
 */
static void osbdm_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)nbytes;
    
    /* Set command pending flag - first byte is command code */
    debug_cmd_pending = g_usb_osbdm_rx_buf[0];
    
    /* Re-arm OUT endpoint for next command */
    usbd_ep_start_read(busid, ep, g_usb_osbdm_rx_buf, sizeof(g_usb_osbdm_rx_buf));
}

/*==============================================================================
 * USB Event Handler
 *============================================================================*/

/**
 * @brief USB device event handler
 */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
            
        case USBD_EVENT_CONNECTED:
            break;
            
        case USBD_EVENT_DISCONNECTED:
            break;
            
        case USBD_EVENT_RESUME:
            break;
            
        case USBD_EVENT_SUSPEND:
            break;
            
        case USBD_EVENT_CONFIGURED:
            /* USB configured - start receiving commands */
            usbd_ep_start_read(busid, OSBDM_OUT_EP, g_usb_osbdm_rx_buf, sizeof(g_usb_osbdm_rx_buf));
            break;
            
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
            
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;
            
        default:
            break;
    }
}

/*==============================================================================
 * USB Interface and Endpoint Structures
 *============================================================================*/

static struct usbd_interface osbdm_interface;

static struct usbd_endpoint osbdm_in_ep = {
    .ep_addr = OSBDM_IN_EP,
    .ep_cb   = osbdm_in_callback
};

static struct usbd_endpoint osbdm_out_ep = {
    .ep_addr = OSBDM_OUT_EP,
    .ep_cb   = osbdm_out_callback
};

/*==============================================================================
 * Public Functions
 *============================================================================*/

/**
 * @brief Initialize OSBDM USB device
 */
void usb_osbdm_init(void)
{
    /* Register USB descriptor */
    usbd_desc_register(BUSID, &osbdm_descriptor);
    
    /* Add interface */
    usbd_add_interface(BUSID, &osbdm_interface);
    
    /* Add endpoints */
    usbd_add_endpoint(BUSID, &osbdm_in_ep);
    usbd_add_endpoint(BUSID, &osbdm_out_ep);
    
    /* Initialize USB device controller
     * For APM32E103: USB base address is USBD_BASE
     * CherryUSB will handle the USB peripheral initialization
     */
    usbd_initialize(BUSID, (uint32_t)USBD_BASE, usbd_event_handler);
}

/**
 * @brief Send data through OSBDM IN endpoint
 */
int32_t usb_osbdm_send(uint8_t *data, uint32_t len)
{
    return usbd_ep_start_write(BUSID, OSBDM_IN_EP, data, len);
}

/**
 * @brief Send data through OSBDM IN endpoint (compatibility alias)
 */
int32_t usb_osbdm_ep_in_send(uint8_t *data, uint32_t len)
{
    return usb_osbdm_send(data, len);
}
