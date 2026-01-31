#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the IMU streaming task.
 * This function will block indefinitely in the streaming loop.
 */
void imu_streamer_start(void);

#ifdef __cplusplus
}
#endif
