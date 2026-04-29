#include "DAP_config.h"
#include "DAP.h"
#include "apm32e10x.h"

/*
 * Platform override for CMSIS-DAP timing.
 *
 * The generic implementation only enters fast_clock for very high requested
 * rates (close to half CPU clock). For this board we want to force the fast
 * path for practical SWD/JTAG rates so per-bit GPIO overhead is minimized.
 */
void Set_Clock_Delay(uint32_t clock)
{
    uint32_t core_hz;
    uint32_t delay;

    if (clock == 0U) {
        DAP_Data.fast_clock = 0U;
        DAP_Data.clock_delay = 1U;
        return;
    }

    core_hz = SystemCoreClock;
    if (core_hz == 0U) {
        core_hz = CPU_CLOCK;
    }

    /* Force fast path for high-frequency requests (blackmagic-native-plus style). */
    if (clock >= 8000000U) {
        DAP_Data.fast_clock = 1U;
        DAP_Data.clock_delay = 1U;
        return;
    }

    DAP_Data.fast_clock = 0U;

    delay = ((core_hz / 2U) + (clock - 1U)) / clock;
    if (delay > IO_PORT_WRITE_CYCLES) {
        delay -= IO_PORT_WRITE_CYCLES;
        delay = (delay + (DELAY_SLOW_CYCLES - 1U)) / DELAY_SLOW_CYCLES;
    } else {
        delay = 1U;
    }

    if (delay == 0U) {
        delay = 1U;
    }

    DAP_Data.clock_delay = delay;
}
