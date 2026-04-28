#include <stddef.h>
#include <stdint.h>

#include "board.h"
#include "dap_main.h"
#include "apm32e10x_dma.h"
#include "apm32e10x_gpio.h"
#include "apm32e10x_rcm.h"
#include "apm32e10x_usart.h"

#define UART_DMA_RX_BUF_SIZE 1024U
#define TARGET_MIN_VOLTAGE_MV     1800U
/* Divider 4.7K/10K: Vadc = Vtarget * 10 / 14.7 = Vtarget * 100 / 147 */
#define VREF_ADC_THRESHOLD_MV ((TARGET_MIN_VOLTAGE_MV * 100U + 73U) / 147U)
#define VREF_PRESENT_THRESHOLD_MV VREF_ADC_THRESHOLD_MV
#define VREF_STABLE_THRESHOLD_MV  VREF_ADC_THRESHOLD_MV
#define VREF_STABLE_CONFIRM_COUNT 4U

static uint8_t g_uart_rx_dma_buf[UART_DMA_RX_BUF_SIZE];
static volatile uint8_t g_uart_ready = 0;
static volatile uint8_t g_uart_tx_done_pending = 0;
static volatile uint32_t g_uart_tx_done_size = 0;
static volatile uint16_t g_uart_rx_dma_last_pos = 0;
static volatile uint8_t g_cdc_port_opened = 0;
static volatile uint8_t g_cdc_dtr = 0;
static volatile uint8_t g_cdc_rts = 0;
static volatile uint8_t g_probe_power_enabled = 0;
static volatile uint8_t g_uart_rx_wait_vref = 0;
static volatile uint8_t g_vref_stable_count = 0;
static volatile uint8_t g_uart_rx_path_enabled = 0;

static void apm32_uart_rx_path_disable(void)
{
    GPIO_Config_T gpioConfig;

    if (!g_uart_ready || !g_uart_rx_path_enabled) {
        return;
    }

    DMA_Disable(DMA1_Channel5);
    DMA_ClearIntFlag(DMA1_INT_FLAG_GINT5);
    USART_DisableInterrupt(USART1, USART_INT_IDLE);
    USART_DisableDMA(USART1, USART_DMA_RX);

    gpioConfig.mode = GPIO_MODE_ANALOG;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = UART_RX_PIN;
    GPIO_Config(UART_RX_PORT, &gpioConfig);

    g_uart_rx_path_enabled = 0;
}

static void apm32_uart_rx_path_enable(void)
{
    GPIO_Config_T gpioConfig;

    if (!g_uart_ready || g_uart_rx_path_enabled) {
        return;
    }

    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = UART_RX_PIN;
    GPIO_Config(UART_RX_PORT, &gpioConfig);

    DMA_Disable(DMA1_Channel5);
    DMA_ClearIntFlag(DMA1_INT_FLAG_GINT5);
    DMA_ConfigDataNumber(DMA1_Channel5, UART_DMA_RX_BUF_SIZE);
    g_uart_rx_dma_last_pos = 0U;
    DMA_Enable(DMA1_Channel5);

    USART_EnableDMA(USART1, USART_DMA_RX);
    USART_EnableInterrupt(USART1, USART_INT_IDLE);

    g_uart_rx_path_enabled = 1;
}

static void apm32_usb2uart_target_power_update(bool opened)
{
    if (opened) {
        if (!g_cdc_port_opened) {
            g_cdc_port_opened = 1;
            g_vref_stable_count = 0;

            if (board_target_voltage_adc_mv() >= VREF_PRESENT_THRESHOLD_MV) {
                g_uart_rx_wait_vref = 0;
                if (g_probe_power_enabled) {
                    board_target_power_set(false);
                    g_probe_power_enabled = 0;
                }
                apm32_uart_rx_path_enable();
            } else {
                if (!g_probe_power_enabled) {
                    board_target_power_set(true);
                    g_probe_power_enabled = 1;
                }
                g_uart_rx_wait_vref = 1;
                apm32_uart_rx_path_disable();
            }
        }
    } else {
        if (g_cdc_port_opened) {
            if (g_probe_power_enabled) {
                board_target_power_set(false);
                g_probe_power_enabled = 0;
            }
            g_cdc_port_opened = 0;
            g_uart_rx_wait_vref = 0;
            g_vref_stable_count = 0;
            apm32_uart_rx_path_enable();
        }
    }
}

static USART_STOP_BIT_T apm32_uart_stop_bits(uint8_t fmt)
{
    switch (fmt) {
        case 1:
            return USART_STOP_BIT_1_5;
        case 2:
            return USART_STOP_BIT_2;
        default:
            return USART_STOP_BIT_1;
    }
}

static USART_PARITY_T apm32_uart_parity(uint8_t parity)
{
    switch (parity) {
        case 1:
            return USART_PARITY_ODD;
        case 2:
            return USART_PARITY_EVEN;
        default:
            return USART_PARITY_NONE;
    }
}

static void apm32_uart_dma_rx_drain(void)
{
    uint16_t current_pos;
    uint16_t chunk_len;

    if (g_uart_rx_wait_vref) {
        return;
    }

    current_pos = (uint16_t)(UART_DMA_RX_BUF_SIZE - DMA_ReadDataNumber(DMA1_Channel5));
    if (current_pos == g_uart_rx_dma_last_pos) {
        return;
    }

    if (current_pos > g_uart_rx_dma_last_pos) {
        chunk_len = (uint16_t)(current_pos - g_uart_rx_dma_last_pos);
        chry_ringbuffer_write(&g_uartrx, &g_uart_rx_dma_buf[g_uart_rx_dma_last_pos], chunk_len);
    } else {
        chunk_len = (uint16_t)(UART_DMA_RX_BUF_SIZE - g_uart_rx_dma_last_pos);
        chry_ringbuffer_write(&g_uartrx, &g_uart_rx_dma_buf[g_uart_rx_dma_last_pos], chunk_len);
        if (current_pos > 0U) {
            chry_ringbuffer_write(&g_uartrx, &g_uart_rx_dma_buf[0], current_pos);
        }
    }

    g_uart_rx_dma_last_pos = current_pos;
}

void apm32_usb2uart_dma_tx_isr(void)
{
    if (DMA_ReadIntFlag(DMA1_INT_FLAG_TC4) == SET) {
        DMA_ClearIntFlag(DMA1_INT_FLAG_GINT4);
        DMA_Disable(DMA1_Channel4);
        g_uart_tx_done_pending = 1;
    }
}

void apm32_usb2uart_dma_rx_isr(void)
{
    if ((DMA_ReadIntFlag(DMA1_INT_FLAG_HT5) == SET) || (DMA_ReadIntFlag(DMA1_INT_FLAG_TC5) == SET)) {
        DMA_ClearIntFlag(DMA1_INT_FLAG_GINT5);
        if (g_uart_rx_path_enabled) {
            apm32_uart_dma_rx_drain();
        }
    }
}

void apm32_usb2uart_usart_isr(void)
{
    if (USART_ReadIntFlag(USART1, USART_INT_IDLE) == SET) {
        volatile uint32_t temp;
        temp = USART1->STS;
        temp = USART1->DATA;
        (void)temp;
        if (g_uart_rx_path_enabled) {
            apm32_uart_dma_rx_drain();
        }
    }
}

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

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    DMA_Config_T dmaConfig;
    GPIO_Config_T gpioConfig;
    USART_Config_T usartConfig;

    if (line_coding == NULL) {
        return;
    }

    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA | RCM_APB2_PERIPH_USART1 | RCM_APB2_PERIPH_AFIO);
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

    DMA_Disable(DMA1_Channel4);
    DMA_Disable(DMA1_Channel5);
    DMA_ClearIntFlag(DMA1_INT_FLAG_GINT4);
    DMA_ClearIntFlag(DMA1_INT_FLAG_GINT5);

    USART_Disable(USART1);
    USART_DisableInterrupt(USART1, USART_INT_IDLE);
    USART_DisableDMA(USART1, USART_DMA_TX_RX);

    gpioConfig.mode = GPIO_MODE_AF_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pin = UART_TX_PIN;
    GPIO_Config(UART_TX_PORT, &gpioConfig);

    gpioConfig.mode = GPIO_MODE_IN_FLOATING;
    gpioConfig.pin = UART_RX_PIN;
    GPIO_Config(UART_RX_PORT, &gpioConfig);

    USART_ConfigStructInit(&usartConfig);
    usartConfig.baudRate = line_coding->dwDTERate;
    usartConfig.wordLength = (line_coding->bDataBits == 9) ? USART_WORD_LEN_9B : USART_WORD_LEN_8B;
    usartConfig.stopBits = apm32_uart_stop_bits(line_coding->bCharFormat);
    usartConfig.parity = apm32_uart_parity(line_coding->bParityType);
    usartConfig.mode = USART_MODE_TX_RX;
    usartConfig.hardwareFlow = USART_HARDWARE_FLOW_NONE;

    USART_Config(USART1, &usartConfig);
    USART_EnableInterrupt(USART1, USART_INT_IDLE);
    USART_EnableDMA(USART1, USART_DMA_TX_RX);
    USART_Enable(USART1);

    DMA_ConfigStructInit(&dmaConfig);

    dmaConfig.peripheralBaseAddr = (uint32_t)&USART1->DATA;
    dmaConfig.memoryBaseAddr = 0U;
    dmaConfig.dir = DMA_DIR_PERIPHERAL_DST;
    dmaConfig.bufferSize = 0U;
    dmaConfig.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
    dmaConfig.memoryInc = DMA_MEMORY_INC_ENABLE;
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    dmaConfig.memoryDataSize = DMA_MEMORY_DATA_SIZE_BYTE;
    dmaConfig.loopMode = DMA_MODE_NORMAL;
    dmaConfig.priority = DMA_PRIORITY_HIGH;
    dmaConfig.M2M = DMA_M2MEN_DISABLE;
    DMA_Config(DMA1_Channel4, &dmaConfig);
    DMA_EnableInterrupt(DMA1_Channel4, DMA_INT_TC | DMA_INT_TERR);

    dmaConfig.peripheralBaseAddr = (uint32_t)&USART1->DATA;
    dmaConfig.memoryBaseAddr = (uint32_t)g_uart_rx_dma_buf;
    dmaConfig.dir = DMA_DIR_PERIPHERAL_SRC;
    dmaConfig.bufferSize = UART_DMA_RX_BUF_SIZE;
    dmaConfig.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
    dmaConfig.memoryInc = DMA_MEMORY_INC_ENABLE;
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    dmaConfig.memoryDataSize = DMA_MEMORY_DATA_SIZE_BYTE;
    dmaConfig.loopMode = DMA_MODE_CIRCULAR;
    dmaConfig.priority = DMA_PRIORITY_HIGH;
    dmaConfig.M2M = DMA_M2MEN_DISABLE;
    DMA_Config(DMA1_Channel5, &dmaConfig);
    DMA_EnableInterrupt(DMA1_Channel5, DMA_INT_TC | DMA_INT_HT | DMA_INT_TERR);
    DMA_Enable(DMA1_Channel5);

    g_uart_rx_dma_last_pos = 0U;
    g_uart_tx_done_pending = 0U;
    g_uart_rx_path_enabled = 1;

    NVIC_EnableIRQRequest(DMA1_Channel4_IRQn, 1, 2);
    NVIC_EnableIRQRequest(DMA1_Channel5_IRQn, 1, 1);
    NVIC_EnableIRQRequest(USART1_IRQn, 1, 0);

    g_uart_ready = 1;
}

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
    if (!g_uart_ready || data == NULL || len == 0) {
        return;
    }

    DMA_Disable(DMA1_Channel4);
    DMA_ClearIntFlag(DMA1_INT_FLAG_GINT4);
    DMA1_Channel4->CHMADDR = (uint32_t)data;
    DMA_ConfigDataNumber(DMA1_Channel4, len);

    g_uart_tx_done_size = len;
    g_uart_tx_done_pending = 0;

    DMA_Enable(DMA1_Channel4);
}

void apm32_usb2uart_poll(void)
{
    uint32_t vref_mv;

    if (!g_uart_ready) {
        return;
    }

    if (g_uart_rx_wait_vref) {
        vref_mv = board_target_voltage_adc_mv();
        if (vref_mv >= VREF_STABLE_THRESHOLD_MV) {
            if (g_vref_stable_count < VREF_STABLE_CONFIRM_COUNT) {
                g_vref_stable_count++;
            }
            if (g_vref_stable_count >= VREF_STABLE_CONFIRM_COUNT) {
                g_uart_rx_wait_vref = 0;
                chry_ringbuffer_reset(&g_uartrx);
                apm32_uart_rx_path_enable();
            }
        } else {
            g_vref_stable_count = 0;
        }
    }

    if (g_uart_tx_done_pending) {
        g_uart_tx_done_pending = 0;
        chry_dap_usb2uart_uart_send_complete(g_uart_tx_done_size);
    }

    apm32_uart_dma_rx_drain();
}

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;

    if (intf != 1U && intf != 2U) {
        return;
    }

    g_cdc_dtr = dtr ? 1U : 0U;
    apm32_usb2uart_target_power_update((g_cdc_dtr | g_cdc_rts) != 0U);
}

void usbd_cdc_acm_set_rts(uint8_t busid, uint8_t intf, bool rts)
{
    (void)busid;

    if (intf != 1U && intf != 2U) {
        return;
    }

    g_cdc_rts = rts ? 1U : 0U;
    apm32_usb2uart_target_power_update((g_cdc_dtr | g_cdc_rts) != 0U);
}
