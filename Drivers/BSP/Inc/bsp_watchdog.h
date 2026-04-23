#ifndef BSP_WATCHDOG_H
#define BSP_WATCHDOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_watchdog_init(uint32_t timeout_ms);
void bsp_watchdog_kick(void);

#ifdef __cplusplus
}
#endif

#endif
