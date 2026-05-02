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
    uint32_t last_boot_req_sample_ms = 0U;
    uint8_t boot_req_low_samples = 0U;

    board_init();
    board_led_init();
    SysTick_Config(SystemCoreClock / 1000U);

    /* Initialize CMSIS-DAP + CDC ACM */
    chry_dap_init(0, USB_BASE_ADDR);

    /* Show firmware is alive */
    board_led_on(0);

    while (1)
    {
        uint32_t now_ms = systick_get();

        chry_dap_handle();
        chry_dap_usb2uart_handle();
        apm32_usb2uart_poll();

        if ((now_ms - last_boot_req_sample_ms) >= 100U) {
            last_boot_req_sample_ms = now_ms;

            if (GPIO_ReadInputBit(BOOT_REQ_PORT, BOOT_REQ_PIN) == BIT_RESET) {
                if (boot_req_low_samples < 10U) {
                    boot_req_low_samples++;
                }
                if (boot_req_low_samples >= 10U) {
                    board_request_bootloader();
                }
            } else {
                boot_req_low_samples = 0U;
            }
        }
    }
}
