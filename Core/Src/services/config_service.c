#include "services/config_service.h"

#include <string.h>
#include "bsp_flash.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#define CONFIG_PARAM_ADDR (0x08004000UL)

static config_param_t configParam;
static config_param_t configParamDefault;

static bool isInit = false;
static bool isDirty = false;
static volatile bool isFlying = false;

static SemaphoreHandle_t mutex = NULL;
static QueueHandle_t saveQueue = NULL;

static uint8_t config_checksum(const config_param_t *data)
{
    const uint8_t *c = (const uint8_t *)data;
    size_t len = sizeof(config_param_t);
    uint8_t sum = 0;

    for (size_t i = 0; i < len; i++)
    {
        sum += *(c++);
    }
    sum -= data->checksum;

    return sum;
}

static void config_set_defaults(void)
{
    configParam.version = CONFIG_VERSION;

    configParam.pidAngle.roll.kp = 8.0f;
    configParam.pidAngle.roll.ki = 0.0f;
    configParam.pidAngle.roll.kd = 0.0f;

    configParam.pidAngle.pitch.kp = 8.0f;
    configParam.pidAngle.pitch.ki = 0.0f;
    configParam.pidAngle.pitch.kd = 0.0f;

    configParam.pidAngle.yaw.kp = 20.0f;
    configParam.pidAngle.yaw.ki = 0.0f;
    configParam.pidAngle.yaw.kd = 1.5f;

    configParam.pidRate.roll.kp = 300.0f;
    configParam.pidRate.roll.ki = 0.0f;
    configParam.pidRate.roll.kd = 6.5f;

    configParam.pidRate.pitch.kp = 300.0f;
    configParam.pidRate.pitch.ki = 0.0f;
    configParam.pidRate.pitch.kd = 6.5f;

    configParam.pidRate.yaw.kp = 200.0f;
    configParam.pidRate.yaw.ki = 18.5f;
    configParam.pidRate.yaw.kd = 0.0f;

    configParam.pidPos.vx.kp = 4.5f;
    configParam.pidPos.vx.ki = 0.0f;
    configParam.pidPos.vx.kd = 0.0f;

    configParam.pidPos.vy.kp = 4.5f;
    configParam.pidPos.vy.ki = 0.0f;
    configParam.pidPos.vy.kd = 0.0f;

    configParam.pidPos.vz.kp = 100.0f;
    configParam.pidPos.vz.ki = 150.0f;
    configParam.pidPos.vz.kd = 10.0f;

    configParam.pidPos.x.kp = 4.0f;
    configParam.pidPos.x.ki = 0.0f;
    configParam.pidPos.x.kd = 0.6f;

    configParam.pidPos.y.kp = 4.0f;
    configParam.pidPos.y.ki = 0.0f;
    configParam.pidPos.y.kd = 0.6f;

    configParam.pidPos.z.kp = 6.0f;
    configParam.pidPos.z.ki = 0.0f;
    configParam.pidPos.z.kd = 4.5f;

    configParam.trimP = 0.0f;
    configParam.trimR = 0.0f;
    configParam.thrustBase = 34000;

    configParam.checksum = config_checksum(&configParam);

    memcpy(&configParamDefault, &configParam, sizeof(config_param_t));
}

static bool config_load_from_flash(void)
{
    if (!bsp_flash_read(CONFIG_PARAM_ADDR, &configParam, sizeof(config_param_t)))
    {
        return false;
    }

    if (configParam.version == CONFIG_VERSION)
    {
        if (config_checksum(&configParam) == configParam.checksum)
        {
            return true;
        }
    }

    return false;
}

static bool config_save_to_flash(void)
{
    configParam.checksum = config_checksum(&configParam);

    if (!bsp_flash_erase(CONFIG_PARAM_ADDR, sizeof(config_param_t)))
    {
        return false;
    }

    return bsp_flash_write(CONFIG_PARAM_ADDR, &configParam, sizeof(config_param_t));
}

void config_service_init(void)
{
    if (isInit)
    {
        return;
    }

    config_set_defaults();

    if (!config_load_from_flash())
    {
        config_save_to_flash();
    }

    memcpy(&configParamDefault, &configParam, sizeof(config_param_t));

    mutex = xSemaphoreCreateMutex();
    saveQueue = xQueueCreate(4, sizeof(bool));

    isInit = true;
}

bool config_service_test(void)
{
    return isInit;
}

bool config_service_is_flying(void)
{
    return isFlying;
}

void config_service_set_flying(bool flying)
{
    isFlying = flying;
}

const config_param_t* config_service_get(void)
{
    if (!isInit)
    {
        return NULL;
    }

    const config_param_t *result;
    xSemaphoreTake(mutex, portMAX_DELAY);
    result = &configParam;
    xSemaphoreGive(mutex);

    return result;
}

config_param_t* config_service_mut(void)
{
    if (!isInit)
    {
        return NULL;
    }

    return &configParam;
}

void config_service_mark_dirty(void)
{
    if (!isInit)
    {
        return;
    }

    isDirty = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(saveQueue, &isDirty, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void config_service_reset_pid(void)
{
    if (!isInit)
    {
        return;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    configParam.pidAngle = configParamDefault.pidAngle;
    configParam.pidRate = configParamDefault.pidRate;
    configParam.pidPos = configParamDefault.pidPos;
    xSemaphoreGive(mutex);

    config_service_mark_dirty();
}

void config_service_task(void *arg)
{
    (void)arg;

    bool dirty = false;

    while (1)
    {
        if (xQueueReceive(saveQueue, &dirty, portMAX_DELAY) == pdTRUE)
        {
            if (dirty)
            {
                xSemaphoreTake(mutex, portMAX_DELAY);
                bool success = config_save_to_flash();
                xSemaphoreGive(mutex);

                if (success)
                {
                    isDirty = false;
                }
            }
        }
    }
}