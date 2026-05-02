/*!
 * @file       usb_osbdm.c
 *
 * @brief      OSBDM USB Device implementation for APM32E103
 *
 * @note       USB device configuration and callbacks
 */

#include "usb_osbdm.h"
#include "board.h"
#include "usbd_core.h"
#include "usb_dfu.h"
#include "cmd_processing.h"
#include <string.h>

/* USB Bus ID */
#define BUSID                       0

/* DFU runtime descriptor size: interface + functional descriptor */
#define DFU_RUNTIME_DESC_SIZE       18
#define USBD_DFU_RUNTIME_ENABLE     0

/* Configuration descriptor size */
#define CONFIG_DESC_SIZE            (9 + 9 + 7 + 7 + USBD_DFU_RUNTIME_ENABLE * DFU_RUNTIME_DESC_SIZE)
#define INTERFACE_NUM               (1 + USBD_DFU_RUNTIME_ENABLE)
#define DFU_RUNTIME_INTF_NUM        1

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
        0x0000,         /* bcdDevice */
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

#if USBD_DFU_RUNTIME_ENABLE
    /* DFU Runtime Interface + Functional Descriptor */
    USB_INTERFACE_DESCRIPTOR_INIT(DFU_RUNTIME_INTF_NUM, 0x00, 0x00, USB_DEVICE_CLASS_APP_SPECIFIC, DFU_SUBCLASS_DFU, DFU_PROTOCOL_RUNTIME, 0x04),
    0x09,
    DFU_FUNC_DESC,
    DFU_ATTR_WILL_DETACH | DFU_ATTR_CAN_DNLOAD,
    WBVAL(0xff),
    WBVAL(0x400),
    WBVAL(0x011a),
#endif
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
    "OSBDM DFU Runtime",            /* DFU Runtime */
};

#if USBD_DFU_RUNTIME_ENABLE
static uint8_t g_dfu_runtime_state = DFU_STATE_APP_IDLE;
static uint8_t g_dfu_runtime_status[6] = { DFU_STATUS_OK, 0, 0, 0, DFU_STATE_APP_IDLE, 0 };
static volatile uint8_t g_dfu_runtime_detach_pending = 0;
static volatile uint32_t g_dfu_runtime_detach_delay = 0;

#define DFU_RUNTIME_DETACH_DELAY_LOOPS 200000U

static void dfu_runtime_poll(void)
{
    if (!g_dfu_runtime_detach_pending) {
        return;
    }

    if (g_dfu_runtime_detach_delay > 0) {
        g_dfu_runtime_detach_delay--;
        return;
    }

    board_request_bootloader();
}

static int dfu_runtime_class_interface_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    (void)busid;

    switch (setup->bRequest) {
        case DFU_REQUEST_DETACH:
            g_dfu_runtime_state = DFU_STATE_APP_DETACH;
            g_dfu_runtime_status[4] = g_dfu_runtime_state;
            g_dfu_runtime_detach_pending = 1;
            g_dfu_runtime_detach_delay = DFU_RUNTIME_DETACH_DELAY_LOOPS;
            *len = 0;
            break;

        case DFU_REQUEST_GETSTATUS:
            if (g_dfu_runtime_state == DFU_STATE_APP_DETACH) {
                g_dfu_runtime_status[1] = 10;
                g_dfu_runtime_status[2] = 0;
                g_dfu_runtime_status[3] = 0;
            }
            memcpy(*data, g_dfu_runtime_status, sizeof(g_dfu_runtime_status));
            *len = sizeof(g_dfu_runtime_status);
            break;

        case DFU_REQUEST_GETSTATE:
            (*data)[0] = g_dfu_runtime_state;
            *len = 1;
            break;

        case DFU_REQUEST_CLRSTATUS:
        case DFU_REQUEST_ABORT:
            g_dfu_runtime_state = DFU_STATE_APP_IDLE;
            g_dfu_runtime_status[0] = DFU_STATUS_OK;
            g_dfu_runtime_status[4] = g_dfu_runtime_state;
            g_dfu_runtime_detach_pending = 0;
            g_dfu_runtime_detach_delay = 0;
            *len = 0;
            break;

        default:
            return -1;
    }

    return 0;
}

static void dfu_runtime_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    (void)busid;
    (void)arg;

    if (event == USBD_EVENT_RESET) {
        g_dfu_runtime_state = DFU_STATE_APP_IDLE;
        g_dfu_runtime_detach_pending = 0;
        g_dfu_runtime_detach_delay = 0;
        g_dfu_runtime_status[0] = DFU_STATUS_OK;
        g_dfu_runtime_status[1] = 0;
        g_dfu_runtime_status[2] = 0;
        g_dfu_runtime_status[3] = 0;
        g_dfu_runtime_status[4] = DFU_STATE_APP_IDLE;
        g_dfu_runtime_status[5] = 0;
    }
}
#endif

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

#if USBD_DFU_RUNTIME_ENABLE
static struct usbd_interface dfu_runtime_intf;
#endif

static struct usbd_endpoint osbdm_in_ep = {
    .ep_addr = OSBDM_IN_EP,
    .ep_cb   = osbdm_in_callback
};

static struct usbd_endpoint osbdm_out_ep = {
    .ep_addr = OSBDM_OUT_EP,
    .ep_cb   = osbdm_out_callback
};

void usb_dc_low_level_init(void)
{
    GPIO_Config_T gpioConfig;

    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | RCM_APB2_PERIPH_AFIO);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USB);

    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = USB_DP_PIN | USB_DM_PIN;
    GPIO_Config(USB_PORT, &gpioConfig);

    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.pin = USB_PU_PIN;
    GPIO_Config(USB_PU_PORT, &gpioConfig);
    GPIO_SetBit(USB_PU_PORT, USB_PU_PIN);

    NVIC_ConfigPriorityGroup(NVIC_PRIORITY_GROUP_2);
    NVIC_EnableIRQRequest(USBD1_LP_CAN1_RX0_IRQn, 1, 0);
    NVIC_EnableIRQRequest(USBDWakeUp_IRQn, 1, 1);
}

void usb_dc_low_level_deinit(void)
{
    NVIC_DisableIRQ(USBD1_LP_CAN1_RX0_IRQn);
    NVIC_DisableIRQ(USBDWakeUp_IRQn);
    GPIO_ResetBit(USB_PU_PORT, USB_PU_PIN);
}

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

#if USBD_DFU_RUNTIME_ENABLE
    dfu_runtime_intf.class_interface_handler = dfu_runtime_class_interface_request_handler;
    dfu_runtime_intf.class_endpoint_handler = NULL;
    dfu_runtime_intf.vendor_handler = NULL;
    dfu_runtime_intf.notify_handler = dfu_runtime_notify_handler;
    usbd_add_interface(BUSID, &dfu_runtime_intf);
#endif
    
    /* Add endpoints */
    usbd_add_endpoint(BUSID, &osbdm_in_ep);
    usbd_add_endpoint(BUSID, &osbdm_out_ep);
    
    /* Initialize USB device controller
     * For APM32E103: USB base address is USBD_BASE
     * CherryUSB will handle the USB peripheral initialization
     */
    usbd_initialize(BUSID, USB_BASE_ADDR, usbd_event_handler);
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

void usb_osbdm_poll(void)
{
#if USBD_DFU_RUNTIME_ENABLE
    dfu_runtime_poll();
#endif
}
