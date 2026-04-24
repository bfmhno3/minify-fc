#include "bsp_sensors.h"

#include "i2c.h"
#include "main.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define BSP_SENSORS_I2C_TIMEOUT_MS 20u
#define BSP_SENSORS_I2C_RETRIES 2u

#define BSP_SENSORS_DRDY_PORT GPIOA
#define BSP_SENSORS_DRDY_PIN GPIO_PIN_4

#define BSP_SENSORS_MPU_ADDRESS_LOW 0x68u
#define BSP_SENSORS_MPU_ADDRESS_HIGH 0x69u
#define BSP_SENSORS_AK8963_ADDRESS 0x0Cu
#define BSP_SENSORS_BMP280_ADDRESS 0x76u
#define BSP_SENSORS_SPL06_ADDRESS 0x76u

#define BSP_SENSORS_MPU_REG_SMPLRT_DIV 0x19u
#define BSP_SENSORS_MPU_REG_CONFIG 0x1Au
#define BSP_SENSORS_MPU_REG_GYRO_CONFIG 0x1Bu
#define BSP_SENSORS_MPU_REG_ACCEL_CONFIG 0x1Cu
#define BSP_SENSORS_MPU_REG_ACCEL_CONFIG_2 0x1Du
#define BSP_SENSORS_MPU_REG_INT_PIN_CFG 0x37u
#define BSP_SENSORS_MPU_REG_INT_ENABLE 0x38u
#define BSP_SENSORS_MPU_REG_INT_STATUS 0x3Au
#define BSP_SENSORS_MPU_REG_ACCEL_XOUT_H 0x3Bu
#define BSP_SENSORS_MPU_REG_USER_CTRL 0x6Au
#define BSP_SENSORS_MPU_REG_PWR_MGMT_1 0x6Bu
#define BSP_SENSORS_MPU_REG_WHO_AM_I 0x75u

#define BSP_SENSORS_MPU_WHO_AM_I_MASK 0x7Eu
#define BSP_SENSORS_MPU_WHO_AM_I_VALUE 0x70u
#define BSP_SENSORS_MPU_INT_STATUS_DATA_READY 0x01u
#define BSP_SENSORS_MPU_CLOCK_PLL_XGYRO 0x01u
#define BSP_SENSORS_MPU_GYRO_FS_2000DPS 0x18u
#define BSP_SENSORS_MPU_ACCEL_FS_16G 0x18u
#define BSP_SENSORS_MPU_ACCEL_DLPF_41HZ 0x03u
#define BSP_SENSORS_MPU_GYRO_DLPF_98HZ 0x02u
#define BSP_SENSORS_MPU_INT_CFG_BYPASS_EN 0x02u
#define BSP_SENSORS_MPU_INT_CFG_ANYRD_2CLEAR 0x10u

#define BSP_SENSORS_AK8963_REG_WIA 0x00u
#define BSP_SENSORS_AK8963_REG_ST1 0x02u
#define BSP_SENSORS_AK8963_REG_HXL 0x03u
#define BSP_SENSORS_AK8963_REG_CNTL1 0x0Au
#define BSP_SENSORS_AK8963_REG_ST2 0x09u
#define BSP_SENSORS_AK8963_WIA_VALUE 0x48u
#define BSP_SENSORS_AK8963_MODE_CONTINUOUS_16BIT_100HZ 0x16u
#define BSP_SENSORS_AK8963_STATUS_DATA_READY 0x01u
#define BSP_SENSORS_AK8963_STATUS_OVERFLOW 0x08u
#define BSP_SENSORS_AK8963_STATUS_DATA_ERROR 0x04u

#define BSP_SENSORS_BMP280_REG_CHIP_ID 0xD0u
#define BSP_SENSORS_BMP280_REG_CTRL_MEAS 0xF4u
#define BSP_SENSORS_BMP280_REG_CONFIG 0xF5u
#define BSP_SENSORS_BMP280_REG_PRESSURE_MSB 0xF7u
#define BSP_SENSORS_BMP280_CHIP_ID 0x58u
#define BSP_SENSORS_BMP280_CTRL_MEAS_VALUE 0x97u
#define BSP_SENSORS_BMP280_CONFIG_VALUE 0x14u

#define BSP_SENSORS_SPL06_REG_PRESSURE_MSB 0x00u
#define BSP_SENSORS_SPL06_REG_PRESSURE_CFG 0x06u
#define BSP_SENSORS_SPL06_REG_TEMPERATURE_CFG 0x07u
#define BSP_SENSORS_SPL06_REG_MODE_CFG 0x08u
#define BSP_SENSORS_SPL06_REG_INT_FIFO_CFG 0x09u
#define BSP_SENSORS_SPL06_REG_CHIP_ID 0x0Du
#define BSP_SENSORS_SPL06_CHIP_ID 0x10u
#define BSP_SENSORS_SPL06_PRESSURE_CFG_VALUE 0x56u
#define BSP_SENSORS_SPL06_TEMPERATURE_CFG_VALUE 0xD3u
#define BSP_SENSORS_SPL06_MODE_CFG_VALUE 0x07u
#define BSP_SENSORS_SPL06_INT_FIFO_CFG_VALUE 0x0Cu

typedef struct
{
    bool initialized;
    bsp_sensors_status_t status;
} bsp_sensors_context_t;

static bsp_sensors_context_t sensors_context = {0};

static uint16_t bsp_sensors_hal_address(uint8_t address)
{
    return (uint16_t)(address << 1);
}

static bool bsp_sensors_read(uint8_t device_address, uint8_t register_address, uint8_t *buffer, uint16_t length)
{
    if (buffer == NULL || length == 0u)
    {
        return false;
    }

    return HAL_I2C_Mem_Read(
               &hi2c1,
               bsp_sensors_hal_address(device_address),
               register_address,
               I2C_MEMADD_SIZE_8BIT,
               buffer,
               length,
               BSP_SENSORS_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool bsp_sensors_write(uint8_t device_address, uint8_t register_address, uint8_t value)
{
    uint8_t data = value;

    return HAL_I2C_Mem_Write(
               &hi2c1,
               bsp_sensors_hal_address(device_address),
               register_address,
               I2C_MEMADD_SIZE_8BIT,
               &data,
               1u,
               BSP_SENSORS_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool bsp_sensors_probe_device(uint8_t device_address)
{
    return HAL_I2C_IsDeviceReady(
               &hi2c1,
               bsp_sensors_hal_address(device_address),
               BSP_SENSORS_I2C_RETRIES,
               BSP_SENSORS_I2C_TIMEOUT_MS) == HAL_OK;
}

static bool bsp_sensors_probe_mpu(uint8_t address)
{
    uint8_t who_am_i = 0u;

    if (!bsp_sensors_probe_device(address))
    {
        return false;
    }

    if (!bsp_sensors_read(address, BSP_SENSORS_MPU_REG_WHO_AM_I, &who_am_i, 1u))
    {
        return false;
    }

    return (who_am_i & BSP_SENSORS_MPU_WHO_AM_I_MASK) == BSP_SENSORS_MPU_WHO_AM_I_VALUE;
}

static bool bsp_sensors_init_mpu(uint8_t address)
{
    if (!bsp_sensors_write(address, BSP_SENSORS_MPU_REG_PWR_MGMT_1, 0x80u))
    {
        return false;
    }

    HAL_Delay(20u);

    return bsp_sensors_write(address, BSP_SENSORS_MPU_REG_PWR_MGMT_1, BSP_SENSORS_MPU_CLOCK_PLL_XGYRO) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_SMPLRT_DIV, 0x00u) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_CONFIG, BSP_SENSORS_MPU_GYRO_DLPF_98HZ) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_GYRO_CONFIG, BSP_SENSORS_MPU_GYRO_FS_2000DPS) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_ACCEL_CONFIG, BSP_SENSORS_MPU_ACCEL_FS_16G) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_ACCEL_CONFIG_2, BSP_SENSORS_MPU_ACCEL_DLPF_41HZ) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_USER_CTRL, 0x00u) &&
           bsp_sensors_write(
               address,
               BSP_SENSORS_MPU_REG_INT_PIN_CFG,
               BSP_SENSORS_MPU_INT_CFG_BYPASS_EN | BSP_SENSORS_MPU_INT_CFG_ANYRD_2CLEAR) &&
           bsp_sensors_write(address, BSP_SENSORS_MPU_REG_INT_ENABLE, BSP_SENSORS_MPU_INT_STATUS_DATA_READY);
}

static bool bsp_sensors_probe_ak8963(void)
{
    uint8_t device_id = 0u;

    if (!bsp_sensors_probe_device(BSP_SENSORS_AK8963_ADDRESS))
    {
        return false;
    }

    if (!bsp_sensors_read(BSP_SENSORS_AK8963_ADDRESS, BSP_SENSORS_AK8963_REG_WIA, &device_id, 1u))
    {
        return false;
    }

    return device_id == BSP_SENSORS_AK8963_WIA_VALUE;
}

static bool bsp_sensors_init_ak8963(void)
{
    return bsp_sensors_write(BSP_SENSORS_AK8963_ADDRESS, BSP_SENSORS_AK8963_REG_CNTL1, 0x00u) &&
           bsp_sensors_write(
               BSP_SENSORS_AK8963_ADDRESS,
               BSP_SENSORS_AK8963_REG_CNTL1,
               BSP_SENSORS_AK8963_MODE_CONTINUOUS_16BIT_100HZ);
}

static bool bsp_sensors_probe_bmp280(void)
{
    uint8_t chip_id = 0u;

    if (!bsp_sensors_probe_device(BSP_SENSORS_BMP280_ADDRESS))
    {
        return false;
    }

    if (!bsp_sensors_read(BSP_SENSORS_BMP280_ADDRESS, BSP_SENSORS_BMP280_REG_CHIP_ID, &chip_id, 1u))
    {
        return false;
    }

    return chip_id == BSP_SENSORS_BMP280_CHIP_ID;
}

static bool bsp_sensors_init_bmp280(void)
{
    return bsp_sensors_write(
               BSP_SENSORS_BMP280_ADDRESS,
               BSP_SENSORS_BMP280_REG_CTRL_MEAS,
               BSP_SENSORS_BMP280_CTRL_MEAS_VALUE) &&
           bsp_sensors_write(
               BSP_SENSORS_BMP280_ADDRESS,
               BSP_SENSORS_BMP280_REG_CONFIG,
               BSP_SENSORS_BMP280_CONFIG_VALUE);
}

static bool bsp_sensors_probe_spl06(void)
{
    uint8_t chip_id = 0u;

    if (!bsp_sensors_probe_device(BSP_SENSORS_SPL06_ADDRESS))
    {
        return false;
    }

    if (!bsp_sensors_read(BSP_SENSORS_SPL06_ADDRESS, BSP_SENSORS_SPL06_REG_CHIP_ID, &chip_id, 1u))
    {
        return false;
    }

    return chip_id == BSP_SENSORS_SPL06_CHIP_ID;
}

static bool bsp_sensors_init_spl06(void)
{
    return bsp_sensors_write(
               BSP_SENSORS_SPL06_ADDRESS,
               BSP_SENSORS_SPL06_REG_PRESSURE_CFG,
               BSP_SENSORS_SPL06_PRESSURE_CFG_VALUE) &&
           bsp_sensors_write(
               BSP_SENSORS_SPL06_ADDRESS,
               BSP_SENSORS_SPL06_REG_TEMPERATURE_CFG,
               BSP_SENSORS_SPL06_TEMPERATURE_CFG_VALUE) &&
           bsp_sensors_write(
               BSP_SENSORS_SPL06_ADDRESS,
               BSP_SENSORS_SPL06_REG_INT_FIFO_CFG,
               BSP_SENSORS_SPL06_INT_FIFO_CFG_VALUE) &&
           bsp_sensors_write(
               BSP_SENSORS_SPL06_ADDRESS,
               BSP_SENSORS_SPL06_REG_MODE_CFG,
               BSP_SENSORS_SPL06_MODE_CFG_VALUE);
}

static int16_t bsp_sensors_read_be16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static int16_t bsp_sensors_read_le16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

static int32_t bsp_sensors_sign_extend_24(uint32_t value)
{
    if ((value & 0x00800000u) != 0u)
    {
        value |= 0xFF000000u;
    }

    return (int32_t)value;
}

void bsp_sensors_init(void)
{
    uint8_t imu_address = 0u;

    memset(&sensors_context, 0, sizeof(sensors_context));

    if (bsp_sensors_probe_mpu(BSP_SENSORS_MPU_ADDRESS_HIGH))
    {
        imu_address = BSP_SENSORS_MPU_ADDRESS_HIGH;
    }
    else if (bsp_sensors_probe_mpu(BSP_SENSORS_MPU_ADDRESS_LOW))
    {
        imu_address = BSP_SENSORS_MPU_ADDRESS_LOW;
    }

    if (imu_address != 0u)
    {
        sensors_context.status.imu_address = imu_address;
        sensors_context.status.imu_present = bsp_sensors_init_mpu(imu_address);
    }

    if (sensors_context.status.imu_present && bsp_sensors_probe_ak8963())
    {
        sensors_context.status.magnetometer_present = bsp_sensors_init_ak8963();
    }

    if (bsp_sensors_probe_bmp280())
    {
        sensors_context.status.barometer_present = bsp_sensors_init_bmp280();
        sensors_context.status.barometer_type = sensors_context.status.barometer_present
                                                  ? BSP_SENSORS_BAROMETER_BMP280
                                                  : BSP_SENSORS_BAROMETER_NONE;
    }
    else if (bsp_sensors_probe_spl06())
    {
        sensors_context.status.barometer_present = bsp_sensors_init_spl06();
        sensors_context.status.barometer_type = sensors_context.status.barometer_present
                                                  ? BSP_SENSORS_BAROMETER_SPL06
                                                  : BSP_SENSORS_BAROMETER_NONE;
    }

    sensors_context.initialized = true;
}

bool bsp_sensors_is_initialized(void)
{
    return sensors_context.initialized;
}

void bsp_sensors_get_status(bsp_sensors_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    *status = sensors_context.status;
}

bool bsp_sensors_is_data_ready(void)
{
    uint8_t status = 0u;

    if (!sensors_context.initialized || !sensors_context.status.imu_present)
    {
        return false;
    }

    if (HAL_GPIO_ReadPin(BSP_SENSORS_DRDY_PORT, BSP_SENSORS_DRDY_PIN) != GPIO_PIN_SET)
    {
        return false;
    }

    if (!bsp_sensors_read(sensors_context.status.imu_address, BSP_SENSORS_MPU_REG_INT_STATUS, &status, 1u))
    {
        return false;
    }

    return (status & BSP_SENSORS_MPU_INT_STATUS_DATA_READY) != 0u;
}

bool bsp_sensors_read_imu_raw(
    bsp_sensors_axis3i16_t *accelerometer,
    bsp_sensors_axis3i16_t *gyroscope,
    int16_t *temperature_raw)
{
    uint8_t raw_data[14] = {0};

    if (accelerometer == NULL || gyroscope == NULL || temperature_raw == NULL)
    {
        return false;
    }

    if (!sensors_context.initialized || !sensors_context.status.imu_present)
    {
        return false;
    }

    if (!bsp_sensors_read(sensors_context.status.imu_address, BSP_SENSORS_MPU_REG_ACCEL_XOUT_H, raw_data, sizeof(raw_data)))
    {
        return false;
    }

    accelerometer->x = bsp_sensors_read_be16(&raw_data[0]);
    accelerometer->y = bsp_sensors_read_be16(&raw_data[2]);
    accelerometer->z = bsp_sensors_read_be16(&raw_data[4]);
    *temperature_raw = bsp_sensors_read_be16(&raw_data[6]);
    gyroscope->x = bsp_sensors_read_be16(&raw_data[8]);
    gyroscope->y = bsp_sensors_read_be16(&raw_data[10]);
    gyroscope->z = bsp_sensors_read_be16(&raw_data[12]);

    return true;
}

bool bsp_sensors_read_magnetometer_raw(bsp_sensors_axis3i16_t *magnetometer)
{
    uint8_t raw_data[7] = {0};

    if (magnetometer == NULL)
    {
        return false;
    }

    if (!sensors_context.initialized || !sensors_context.status.magnetometer_present)
    {
        return false;
    }

    if (!bsp_sensors_read(BSP_SENSORS_AK8963_ADDRESS, BSP_SENSORS_AK8963_REG_ST1, raw_data, sizeof(raw_data)))
    {
        return false;
    }

    if ((raw_data[0] & BSP_SENSORS_AK8963_STATUS_DATA_READY) == 0u)
    {
        return false;
    }

    if ((raw_data[6] & (BSP_SENSORS_AK8963_STATUS_OVERFLOW | BSP_SENSORS_AK8963_STATUS_DATA_ERROR)) != 0u)
    {
        return false;
    }

    magnetometer->x = bsp_sensors_read_le16(&raw_data[1]);
    magnetometer->y = bsp_sensors_read_le16(&raw_data[3]);
    magnetometer->z = bsp_sensors_read_le16(&raw_data[5]);

    return true;
}

bool bsp_sensors_read_barometer_raw(bsp_sensors_baro_raw_t *barometer)
{
    uint8_t raw_data[6] = {0};

    if (barometer == NULL)
    {
        return false;
    }

    if (!sensors_context.initialized || !sensors_context.status.barometer_present)
    {
        return false;
    }

    if (sensors_context.status.barometer_type == BSP_SENSORS_BAROMETER_BMP280)
    {
        if (!bsp_sensors_read(BSP_SENSORS_BMP280_ADDRESS, BSP_SENSORS_BMP280_REG_PRESSURE_MSB, raw_data, sizeof(raw_data)))
        {
            return false;
        }

        barometer->pressure = (int32_t)((((uint32_t)raw_data[0]) << 12) |
                                        (((uint32_t)raw_data[1]) << 4) |
                                        ((uint32_t)raw_data[2] >> 4));
        barometer->temperature = (int32_t)((((uint32_t)raw_data[3]) << 12) |
                                           (((uint32_t)raw_data[4]) << 4) |
                                           ((uint32_t)raw_data[5] >> 4));
        return true;
    }

    if (sensors_context.status.barometer_type == BSP_SENSORS_BAROMETER_SPL06)
    {
        if (!bsp_sensors_read(BSP_SENSORS_SPL06_ADDRESS, BSP_SENSORS_SPL06_REG_PRESSURE_MSB, raw_data, sizeof(raw_data)))
        {
            return false;
        }

        barometer->pressure = bsp_sensors_sign_extend_24(
            ((uint32_t)raw_data[0] << 16) |
            ((uint32_t)raw_data[1] << 8) |
            raw_data[2]);
        barometer->temperature = bsp_sensors_sign_extend_24(
            ((uint32_t)raw_data[3] << 16) |
            ((uint32_t)raw_data[4] << 8) |
            raw_data[5]);
        return true;
    }

    return false;
}
