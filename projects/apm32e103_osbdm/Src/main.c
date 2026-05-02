/*!
 * @file       main.c
 *
 * @brief      Main application for OSBDM on APM32E103
 *
 * @note       OSBDM debug interface for multiple target architectures
 */

#include "apm32e10x.h"
#include "board.h"
#include "cmd_processing.h"
#include "usb_osbdm.h"
#include <stdio.h>

typedef struct {
    uint32_t magic;
    uint32_t handler_id;
    uint32_t stacked_r0;
    uint32_t stacked_r1;
    uint32_t stacked_r2;
    uint32_t stacked_r3;
    uint32_t stacked_r12;
    uint32_t stacked_lr;
    uint32_t stacked_pc;
    uint32_t stacked_xpsr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t mmfar;
    uint32_t bfar;
} fault_snapshot_t;

volatile fault_snapshot_t g_fault_snapshot;

__attribute__((used, noinline)) void fault_capture_and_halt(uint32_t *stacked_regs, uint32_t handler_id)
{
    g_fault_snapshot.magic = 0x46414C54U; /* "FALT" */
    g_fault_snapshot.handler_id = handler_id;
    g_fault_snapshot.stacked_r0 = stacked_regs[0];
    g_fault_snapshot.stacked_r1 = stacked_regs[1];
    g_fault_snapshot.stacked_r2 = stacked_regs[2];
    g_fault_snapshot.stacked_r3 = stacked_regs[3];
    g_fault_snapshot.stacked_r12 = stacked_regs[4];
    g_fault_snapshot.stacked_lr = stacked_regs[5];
    g_fault_snapshot.stacked_pc = stacked_regs[6];
    g_fault_snapshot.stacked_xpsr = stacked_regs[7];
    g_fault_snapshot.cfsr = SCB->CFSR;
    g_fault_snapshot.hfsr = SCB->HFSR;
    g_fault_snapshot.dfsr = SCB->DFSR;
    g_fault_snapshot.afsr = SCB->AFSR;
    g_fault_snapshot.mmfar = SCB->MMFAR;
    g_fault_snapshot.bfar = SCB->BFAR;

    __DSB();
    __BKPT(0);
    while (1) {
        board_led_on(2);
    }
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, #1\n"
        "b fault_capture_and_halt\n");
}

__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, #2\n"
        "b fault_capture_and_halt\n");
}

__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, #3\n"
        "b fault_capture_and_halt\n");
}

__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, #4\n"
        "b fault_capture_and_halt\n");
}

/* System tick counter */
static volatile uint32_t systick_counter = 0;

/* Target voltage monitoring */
static volatile uint16_t target_voltage_mv = 0;

/* Activity LED blink state */
static volatile uint32_t led_blink_timer = 0;
static volatile uint8_t led_blink_state = 0;

/**
 * @brief SysTick interrupt handler
 */
void SysTick_Handler(void)
{
    systick_counter++;
    
    /* Update LED blink timer */
    if (led_blink_timer > 0) {
        led_blink_timer--;
    }
}

void WWDT_IRQHandler(void)
{
    /* Defensive: clear WWDT early wakeup flag and keep IRQ masked in app mode. */
    WWDT->STS_B.EWIFLG = RESET;
    NVIC_DisableIRQ(WWDT_IRQn);
    NVIC_ClearPendingIRQ(WWDT_IRQn);
}

/**
 * @brief Get system tick count in milliseconds
 */
uint32_t systick_get(void)
{
    return systick_counter;
}
/**
 * @brief Main application entry point
 */
int main(void)
{
    uint32_t last_boot_req_sample_ms = 0U;
    uint8_t boot_req_low_samples = 0U;

    /* Initialize board hardware */
    board_init();
    board_led_init();
    SysTick_Config(SystemCoreClock / 1000U);

    /* Keep target power disabled by default for safety. */
    board_target_power_set(false);

    /* Initialize OSBDM USB stack and protocol endpoint state. */
    debug_cmd_pending = 0;
    usb_osbdm_init();

    /* Initial LED state: running on, others off. */
    board_led_on(0);
    board_led_off(1);
    board_led_off(2);

    /* Main loop: process USB requests and execute OSBDM commands. */
    while (1) {
        uint32_t now_ms = systick_get();

        usb_osbdm_poll();

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

        if (debug_cmd_pending != 0U) {
            board_led_toggle(1);
            debug_command_exec();
            debug_cmd_pending = 0;
            board_led_toggle(1);
        }
    }
    
    return 0;
}

/**
 * @brief Minimal printf support for debugging (optional)
 * 
 * Redirect printf to nowhere if not needed, or implement UART output
 */
int fputc(int ch, FILE *f)
{
    (void)f;
    return ch;
}

int fgetc(FILE *f)
{
    (void)f;
    return 0;
}
