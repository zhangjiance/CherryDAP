/*
 * USB interrupt bridge for CherryUSB FSDEV on APM32E103.
 * Keep this file minimal to avoid overriding app-specific fault handlers.
 */

#include "apm32e10x.h"

extern void USBD_IRQHandler(uint8_t busid);

void USBD1_HP_CAN1_TX_IRQHandler(void)
{
    USBD_IRQHandler(0);
}

void USBD1_LP_CAN1_RX0_IRQHandler(void)
{
    USBD_IRQHandler(0);
}

void USBDWakeUp_IRQHandler(void)
{
    USBD_IRQHandler(0);
}
