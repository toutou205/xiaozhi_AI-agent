#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float acc_x;
  float acc_y;
  float acc_z;
  float gyr_x;
  float gyr_y;
  float gyr_z;
  float temp;
} icm42607_data_t;

/**
 * @brief Initialize the ICM-42607-P sensor
 *
 * @return esp_err_t ESP_OK on success
 */
#include "driver/i2c_master.h"

/**
 * @brief Initialize the ICM-42607-P sensor
 *
 * @param bus_handle Handle to the initialized I2C master bus
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_icm42607_init(i2c_master_bus_handle_t bus_handle);

/**
 * @brief Read data from the sensor
 *
 * @param data Pointer to data structure to fill
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_icm42607_read_data(icm42607_data_t *data);

typedef enum {
  ICM_ACCE_FS_16G = 0,
  ICM_ACCE_FS_8G = 1,
  ICM_ACCE_FS_4G = 2,
  ICM_ACCE_FS_2G = 3,
} icm_acce_fs_t;

typedef enum {
  ICM_GYRO_FS_2000DPS = 0,
  ICM_GYRO_FS_1000DPS = 1,
  ICM_GYRO_FS_500DPS = 2,
  ICM_GYRO_FS_250DPS = 3,
} icm_gyro_fs_t;

typedef enum {
  ICM_ODR_100HZ = 9,
  ICM_ODR_200HZ = 7, // Standard map usually different, check logic
  ICM_ODR_50HZ = 10
} icm_odr_t;

/**
 * @brief Set Accelerometer Configuration
 */
esp_err_t sensor_icm42607_set_accel_config(icm_acce_fs_t fs, icm_odr_t odr);

/**
 * @brief Set Gyroscope Configuration
 */
esp_err_t sensor_icm42607_set_gyro_config(icm_gyro_fs_t fs, icm_odr_t odr);

/**
 * @brief Enable Data Ready Interrupt on INT1
 */
esp_err_t sensor_icm42607_enable_drdy_int(void);

#ifdef __cplusplus
}
#endif
