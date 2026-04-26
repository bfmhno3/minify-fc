#include "services/pm_service.h"

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "services/config_service.h"
#include "comm/atkp.h"

static float battery_voltage = PM_VOLTAGE_INVALID;
static float battery_voltage_max = 0.0f;
static float battery_voltage_min = 0.0f;
static uint32_t low_power_timestamp = 0;
static pm_state_t pm_state = PM_STATE_BATTERY;
static pm_syslink_info_t pm_syslink_info;
static bool is_init = false;

void pm_service_init(void)
{
    if (is_init)
    {
        return;
    }

    battery_voltage = 3.7f;
    battery_voltage_max = 0.0f;
    battery_voltage_min = 0.0f;
    low_power_timestamp = 0;
    pm_state = PM_STATE_BATTERY;
    memset(&pm_syslink_info, 0, sizeof(pm_syslink_info));

    is_init = true;
}

bool pm_service_test(void)
{
    return is_init && (battery_voltage > 0);
}

void pm_service_update_voltage(void *atkp)
{
    if (!is_init || atkp == NULL)
    {
        return;
    }

    atkp_t *packet = (atkp_t *)atkp;

    if (packet->dataLen < sizeof(pm_syslink_info_t))
    {
        return;
    }

    memcpy(&pm_syslink_info, &packet->data[0], sizeof(pm_syslink_info_t));

    if (pm_syslink_info.vBat <= 0.0f)
    {
        return;
    }

    battery_voltage = pm_syslink_info.vBat;

    if (battery_voltage > battery_voltage_max)
    {
        battery_voltage_max = battery_voltage;
    }

    if ((battery_voltage_min == 0.0f) || (battery_voltage < battery_voltage_min))
    {
        battery_voltage_min = battery_voltage;
    }
}

float pm_service_get_voltage(void)
{
    return battery_voltage;
}

float pm_service_get_voltage_max(void)
{
    return battery_voltage_max;
}

float pm_service_get_voltage_min(void)
{
    return battery_voltage_min;
}

pm_state_t pm_service_get_state(void)
{
    return pm_state;
}

bool pm_service_is_low_power(void)
{
    return (pm_state == PM_STATE_LOW_POWER);
}

bool pm_service_is_charging(void)
{
    return (pm_state == PM_STATE_CHARGING);
}

void pm_service_task(void *param)
{
    (void)param;

    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    float voltage_threshold;

    if (config_service_is_flying())
    {
        voltage_threshold = PM_BAT_LOW_VOLTAGE_FLY;
    }
    else
    {
        voltage_threshold = PM_BAT_LOW_VOLTAGE_STATIC;
    }

    if (battery_voltage < voltage_threshold)
    {
        if (low_power_timestamp == 0)
        {
            low_power_timestamp = current_time;
        }
        else if ((current_time - low_power_timestamp) > PM_BAT_LOW_TIMEOUT_MS)
        {
            pm_state = PM_STATE_LOW_POWER;
        }
    }
    else
    {
        low_power_timestamp = 0;

        if (pm_state == PM_STATE_LOW_POWER)
        {
            pm_state = PM_STATE_BATTERY;
        }
    }

    uint8_t flags = pm_syslink_info.flags;
    bool pgood = (flags & 0x01) != 0;
    bool chg = (flags & 0x02) != 0;

    if (pgood && !chg)
    {
        pm_state = PM_STATE_CHARGED;
    }
    else if (pgood && chg)
    {
        pm_state = PM_STATE_CHARGING;
    }
    else if (!pgood && !chg && (pm_state == PM_STATE_LOW_POWER))
    {
    }
    else if (pm_state != PM_STATE_LOW_POWER)
    {
        pm_state = PM_STATE_BATTERY;
    }
}