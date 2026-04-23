#include "bsp_watchdog.h"

#include "iwdg.h"

#include <stdbool.h>

static bool watchdog_initialized = false;

void bsp_watchdog_init(uint32_t timeout_ms)
{
    (void)timeout_ms;

    MX_IWDG_Init();
    watchdog_initialized = true;
}

void bsp_watchdog_kick(void)
{
    if (!watchdog_initialized)
    {
        return;
    }

    (void)HAL_IWDG_Refresh(&hiwdg);
}
