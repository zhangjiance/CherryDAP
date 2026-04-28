/*!
 * @file       apm32e10x_int.h
 *
 * @brief      Interrupt handlers header file
 *
 * @note       CherryDAP for APM32E103
 */

#ifndef __APM32E10X_INT_H
#define __APM32E10X_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apm32e10x.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USBD1_LP_CAN1_RX0_IRQHandler(void);
void USBDWakeUp_IRQHandler(void);
void USART1_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __APM32E10X_INT_H */
