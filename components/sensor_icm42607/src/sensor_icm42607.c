#include "sensor_icm42607.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sensor_icm42607";

#define I2C_MASTER_FREQ_HZ 400000 /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS 1000

#define ICM42670_I2C_ADDRESS                                                   \
  0x68 /*!< I2C address of ICM42670 (0x68 or 0x69)                             \
        */

// Register Map (ICM-42607-P/ICM-42670-P) - Verified from Legacy Driver
#define REG_WHO_AM_I 0x75
#define REG_PWR_MGMT0 0x1F
#define REG_GYRO_CONFIG0 0x20
#define REG_ACCEL_CONFIG0 0x21
#define REG_TEMP_DATA1 0x09    // Starts at 0x09 (TempH, TempL, AccXH...)
#define REG_ACCEL_DATA_X1 0x0B // 0x0B
#define REG_GYRO_DATA_X1 0x11  // 0x11
#define REG_INT_SOURCE0 0x65
#define REG_INT_CONFIG 0x14

// Global Sensitivities (Default 16G, 2000DPS)
static float g_acce_sensitivity = 2048.0f;
static float g_gyro_sensitivity = 16.4f;

// Helper functions using New Driver API
static esp_err_t write_reg(i2c_master_dev_handle_t handle, uint8_t reg,
                           uint8_t data) {
  uint8_t buf[2] = {reg, data};
  return i2c_master_transmit(handle, buf, sizeof(buf), -1);
}

static esp_err_t read_reg(i2c_master_dev_handle_t handle, uint8_t reg,
                          uint8_t *data, size_t len) {
  return i2c_master_transmit_receive(handle, &reg, 1, data, len, -1);
}

// Device Handle
static i2c_master_dev_handle_t dev_handle = NULL;

esp_err_t sensor_icm42607_init(i2c_master_bus_handle_t bus_handle) {
  if (bus_handle == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // 1. Add Device to Bus
  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ICM42670_I2C_ADDRESS,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };

  esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Add device failed");
    return ret;
  }

  ESP_LOGI(TAG, "ICM-42607-P Device Added (New Driver)");

  // --- Verify Device ID ---
  uint8_t who_am_i = 0;
  ret = read_reg(dev_handle, REG_WHO_AM_I, &who_am_i, 1);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read WHO_AM_I");
    return ret;
  }
  ESP_LOGI(TAG, "Found Device ID: 0x%02x", who_am_i);

  // --- Configuration Sequence ---

  // 1. PWR_MGMT0: Enable Gyro & Accel (Low Noise Mode)
  ret = write_reg(dev_handle, REG_PWR_MGMT0, 0x0F);
  ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set PWR_MGMT0");

  vTaskDelay(pdMS_TO_TICKS(50)); // Wait for sensors up

  // 2. GYRO_CONFIG0: 2000DPS, 100Hz ODR
  // Bits 7:5 = FS_SEL -> 000 = ±2000 dps (16.4 LSB/dps)
  // Bits 3:0 = ODR_CONF -> 9 = 100Hz
  // Value = 0x09
  ret = write_reg(dev_handle, REG_GYRO_CONFIG0, 0x09);
  ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set GYRO_CONFIG0");
  g_gyro_sensitivity = 16.4f;

  // 3. ACCEL_CONFIG0: 16G, 100Hz ODR
  // Bits 7:5 = FS_SEL -> 000 = ±16g (2048 LSB/g)
  // Bits 3:0 = ODR -> 9 = 100Hz
  // Value = 0x09
  ret = write_reg(dev_handle, REG_ACCEL_CONFIG0, 0x09);
  ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set ACCEL_CONFIG0");
  g_acce_sensitivity = 2048.0f;

  ESP_LOGI(TAG,
           "ICM-42607-P Initialized Successfully (Register Config Complete)");
  return ESP_OK;
}

esp_err_t sensor_icm42607_set_accel_config(icm_acce_fs_t fs, icm_odr_t odr) {
  if (dev_handle == NULL)
    return ESP_FAIL;

  // Bits 7:5 = FS, Bits 3:0 = ODR
  uint8_t val = (fs << 5) | (odr & 0x0F);
  esp_err_t ret = write_reg(dev_handle, REG_ACCEL_CONFIG0, val);

  if (ret == ESP_OK) {
    switch (fs) {
    case ICM_ACCE_FS_16G:
      g_acce_sensitivity = 2048.0f;
      break;
    case ICM_ACCE_FS_8G:
      g_acce_sensitivity = 4096.0f;
      break;
    case ICM_ACCE_FS_4G:
      g_acce_sensitivity = 8192.0f;
      break;
    case ICM_ACCE_FS_2G:
      g_acce_sensitivity = 16384.0f;
      break;
    }
  }
  return ret;
}

esp_err_t sensor_icm42607_set_gyro_config(icm_gyro_fs_t fs, icm_odr_t odr) {
  if (dev_handle == NULL)
    return ESP_FAIL;

  // Bits 7:5 = FS, Bits 3:0 = ODR
  uint8_t val = (fs << 5) | (odr & 0x0F);
  esp_err_t ret = write_reg(dev_handle, REG_GYRO_CONFIG0, val);

  if (ret == ESP_OK) {
    switch (fs) {
    case ICM_GYRO_FS_2000DPS:
      g_gyro_sensitivity = 16.4f;
      break;
    case ICM_GYRO_FS_1000DPS:
      g_gyro_sensitivity = 32.8f;
      break;
    case ICM_GYRO_FS_500DPS:
      g_gyro_sensitivity = 65.5f;
      break;
    case ICM_GYRO_FS_250DPS:
      g_gyro_sensitivity = 131.0f;
      break;
    }
  }
  return ret;
}

esp_err_t sensor_icm42607_enable_drdy_int(void) {
  if (dev_handle == NULL)
    return ESP_FAIL;

  // 1. Configure INT1 pin (Push-Pull, Active High is default)
  // REG_INT_CONFIG (0x14) - Default usually fine

  // 2. Enable Data Ready Interrupt
  // REG_INT_SOURCE0 (0x65): Bit 3 = UI_DRDY_INT1_EN
  uint8_t val = 0x08; // Bit 3
  return write_reg(dev_handle, REG_INT_SOURCE0, val);
}

esp_err_t sensor_icm42607_read_data(icm42607_data_t *data) {
  if (dev_handle == NULL)
    return ESP_FAIL;

  // Burst Read 14 bytes starting from TEMP_DATA (0x09)
  uint8_t raw_buf[14];
  esp_err_t ret = read_reg(dev_handle, REG_TEMP_DATA1, raw_buf, 14);
  if (ret != ESP_OK)
    return ret;

  // Parse Temp (Big Endian)
  int16_t t_cnt = (raw_buf[0] << 8) | raw_buf[1];
  data->temp = (t_cnt / 128.0f) + 25.0f;

  // Parse Accel (Big Endian)
  int16_t ax_cnt = (raw_buf[2] << 8) | raw_buf[3];
  int16_t ay_cnt = (raw_buf[4] << 8) | raw_buf[5];
  int16_t az_cnt = (raw_buf[6] << 8) | raw_buf[7];

  // Parse Gyro (Big Endian)
  int16_t gx_cnt = (raw_buf[8] << 8) | raw_buf[9];
  int16_t gy_cnt = (raw_buf[10] << 8) | raw_buf[11];
  int16_t gz_cnt = (raw_buf[12] << 8) | raw_buf[13];

  // Use dynamic sensitivities
  data->acc_x = (ax_cnt / g_acce_sensitivity) * 9.81f;
  data->acc_y = (ay_cnt / g_acce_sensitivity) * 9.81f;
  data->acc_z = (az_cnt / g_acce_sensitivity) * 9.81f;

  data->gyr_x = gx_cnt / g_gyro_sensitivity;
  data->gyr_y = gy_cnt / g_gyro_sensitivity;
  data->gyr_z = gz_cnt / g_gyro_sensitivity;

  return ESP_OK;
}
