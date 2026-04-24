#ifndef BSP_SENSORS_H
#define BSP_SENSORS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} bsp_sensors_axis3i16_t;

typedef struct
{
    int32_t pressure;
    int32_t temperature;
} bsp_sensors_baro_raw_t;

typedef enum
{
    BSP_SENSORS_BAROMETER_NONE = 0,
    BSP_SENSORS_BAROMETER_BMP280 = 1,
    BSP_SENSORS_BAROMETER_SPL06 = 2,
} bsp_sensors_barometer_type_t;

typedef struct
{
    bool imu_present;
    bool magnetometer_present;
    bool barometer_present;
    bsp_sensors_barometer_type_t barometer_type;
    uint8_t imu_address;
} bsp_sensors_status_t;

void bsp_sensors_init(void);
bool bsp_sensors_is_initialized(void);
void bsp_sensors_get_status(bsp_sensors_status_t *status);
bool bsp_sensors_is_data_ready(void);
bool bsp_sensors_read_imu_raw(
    bsp_sensors_axis3i16_t *accelerometer,
    bsp_sensors_axis3i16_t *gyroscope,
    int16_t *temperature_raw);
bool bsp_sensors_read_magnetometer_raw(bsp_sensors_axis3i16_t *magnetometer);
bool bsp_sensors_read_barometer_raw(bsp_sensors_baro_raw_t *barometer);

#ifdef __cplusplus
}
#endif

#endif
