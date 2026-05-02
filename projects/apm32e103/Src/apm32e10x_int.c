/*!
 * @file       apm32e10x_int.c
 *
 * @brief      Interrupt handlers
 *
 * @note       CherryDAP for APM32E103
 */

#include "apm32e10x_int.h"

static volatile uint32_t g_systick_counter = 0;

extern void USBD_IRQHandler(uint8_t busid);
extern void apm32_usb2uart_usart_isr(void);
extern void apm32_usb2uart_dma_tx_isr(void);
extern void apm32_usb2uart_dma_rx_isr(void);

/*!
 * @brief   This function handles NMI exception
 */
void NMI_Handler(void)
{
}

/*!
 * @brief   This function handles Hard Fault exception
 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}

/*!
 * @brief   This function handles Memory Manage exception
 */
void MemManage_Handler(void)
{
    while (1)
    {
    }
}

/*!
 * @brief   This function handles Bus Fault exception
 */
void BusFault_Handler(void)
{
    while (1)
    {
    }
}

/*!
 * @brief   This function handles Usage Fault exception
 */
void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

/*!
 * @brief   This function handles SVCall exception
 */
void SVC_Handler(void)
{
}

/*!
 * @brief   This function handles Debug Monitor exception
 */
void DebugMon_Handler(void)
{
}

/*!
 * @brief   This function handles PendSV_Handler exception
 */
void PendSV_Handler(void)
{
}

/*!
 * @brief   This function handles SysTick Handler
 */
void SysTick_Handler(void)
{
    g_systick_counter++;
}

uint32_t systick_get(void)
{
    return g_systick_counter;
}

void USBD1_LP_CAN1_RX0_IRQHandler(void)
{
    USBD_IRQHandler(0);
}

void USBDWakeUp_IRQHandler(void)
{
    USBD_IRQHandler(0);
}

void USART1_IRQHandler(void)
{
    apm32_usb2uart_usart_isr();
}

void DMA1_Channel4_IRQHandler(void)
{
    apm32_usb2uart_dma_tx_isr();
}

void DMA1_Channel5_IRQHandler(void)
{
    apm32_usb2uart_dma_rx_isr();
}
