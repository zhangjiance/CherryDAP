/*!
 * @file       main.c
 *
 * @brief      Main program for CherryDAP on APM32E103
 *
 * @note       Compatible with BlackMagic DFU bootloader
 *             Application starts at 0x08002000
 */

#include "board.h"
#include "apm32e10x_int.h"
#include "usb_config.h"
#include "dap_main.h"

void apm32_usb2uart_poll(void);

/*!
 * @brief       Main program
 */
int main(void)
{
    board_init();
    board_led_init();

    /* Initialize CMSIS-DAP + CDC ACM */
    chry_dap_init(0, USB_BASE_ADDR);

    /* Show firmware is alive */
    board_led_on(0);

    while (1)
    {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        apm32_usb2uart_poll();
    }
}
