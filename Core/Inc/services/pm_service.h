#ifndef PM_SERVICE_H
#define PM_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PM_BAT_LOW_VOLTAGE_FLY    3.00f
#define PM_BAT_LOW_VOLTAGE_STATIC 3.60f
#define PM_BAT_LOW_TIMEOUT_MS     5000
#define PM_VOLTAGE_INVALID        0.0f

typedef struct {
    uint8_t flags;
    float vBat;
} __attribute__((packed)) pm_syslink_info_t;

typedef enum {
    PM_STATE_BATTERY = 0,
    PM_STATE_CHARGING,
    PM_STATE_CHARGED,
    PM_STATE_LOW_POWER,
    PM_STATE_SHUT_DOWN,
} pm_state_t;

void pm_service_init(void);
bool pm_service_test(void);
void pm_service_task(void *param);
void pm_service_update_voltage(void *atkp);

float pm_service_get_voltage(void);
float pm_service_get_voltage_max(void);
float pm_service_get_voltage_min(void);
pm_state_t pm_service_get_state(void);
bool pm_service_is_low_power(void);
bool pm_service_is_charging(void);

#ifdef __cplusplus
}
#endif

#endif