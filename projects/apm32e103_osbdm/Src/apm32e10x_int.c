/*
 * USB interrupt bridge for CherryUSB FSDEV on APM32E103.
 * Keep this file minimal to avoid overriding app-specific fault handlers.
 */

#include "apm32e10x.h"
#include "apm32e10x_tmr.h"
#include "board.h"

static volatile uint8_t  g_boot_req_samples     = 0;
static volatile uint8_t  g_bootloader_triggered = 0;

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

void TMR4_IRQHandler(void)
{
    if (TMR_ReadIntFlag(TMR4, TMR_INT_UPDATE) != RESET) {
        TMR_ClearIntFlag(TMR4, TMR_INT_UPDATE);

        /* Prevent reentry if bootloader request already in progress. */
        if (g_bootloader_triggered) {
            return;
        }

        if (GPIO_ReadInputBit(BOOT_REQ_PORT, BOOT_REQ_PIN) == BIT_RESET) {
            /* Visual feedback: toggle LED while pressing */
            board_led_toggle(LED_IDLE_NUM);

            if (g_boot_req_samples < 10U) {
                g_boot_req_samples++;
            }
            if (g_boot_req_samples >= 10U) {
                g_bootloader_triggered = 1;
                board_request_bootloader();
            }
        } else {
            g_boot_req_samples = 0U;
        }
    }
}
