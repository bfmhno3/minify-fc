#include "app/app_hooks.h"

#include "bsp_watchdog.h"
#include "cmsis_os2.h"
#include "main.h"

#define APP_IDLE_WATCHDOG_KICK_INTERVAL_TICKS 100U

void app_idle_hook(void)
{
    static uint32_t last_watchdog_kick_tick;
    const uint32_t current_tick = osKernelGetTickCount();

    if ((current_tick - last_watchdog_kick_tick) >= APP_IDLE_WATCHDOG_KICK_INTERVAL_TICKS) {
        last_watchdog_kick_tick = current_tick;
        bsp_watchdog_kick();
    }

    __WFI();
}
