#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

// Event Base Declaration
ESP_EVENT_DECLARE_BASE(IMU_AI_EVENT);

// Event IDs
typedef enum {
  IMU_AI_EVT_TAP,
  IMU_AI_EVT_SHAKE,
  IMU_AI_EVT_IDLE,
  IMU_AI_EVT_UNKNOWN
} imu_ai_event_id_t;

// Configuration Structure
typedef struct {
  // [Required] Externally initialized I2C Bus Handle (Void* to decouple C++
  // types if needed, but here we use direct type)
  i2c_master_bus_handle_t i2c_bus_handle;

  // [Optional] I2C Device Address (Default 0x60 if 0)
  uint8_t i2c_addr;

  bool enable_cli;         // Enable USB Serial/JTAG CLI for runtime tuning
  float default_threshold; // Default confidence threshold (0.0 - 1.0)
} imu_ai_config_t;

/**
 * @brief Initialize the IMU AI Library (Allocates memory, loads model, detects
 * sensor)
 * @param config Configuration struct
 * @return ESP_OK on success
 */
esp_err_t imu_ai_init(const imu_ai_config_t *config);

/**
 * @brief Start the inference loop task
 * @param priority Task priority
 * @param core_id  Core affinity (0 or 1, or tskNO_AFFINITY)
 * @return ESP_OK on success
 */
esp_err_t imu_ai_start(uint8_t priority, int core_id);

/**
 * @brief Stop the inference loop and free resources
 * @return ESP_OK on success
 */
esp_err_t imu_ai_stop(void);

/**
 * @brief Register an event handler for gesture events
 * @param callback User callback function
 * @param arg User argument
 * @return ESP_OK on success
 */
esp_err_t imu_ai_register_event_handler(esp_event_handler_t callback,
                                        void *arg);

/**
 * @brief Legacy API - Deprecated, will be removed
 */
void imu_streamer_start_legacy(void);

#ifdef __cplusplus
}
#endif
