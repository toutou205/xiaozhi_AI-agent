#include "resource_monitor.h"
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>

#define TAG "ResourceMonitor"

void StartResourceMonitor(void) {
  xTaskCreate(
      [](void *arg) {
        // Wait for system to stabilize
        vTaskDelay(pdMS_TO_TICKS(80000));

        printf("--- BEGIN RESOURCE LOG ---\n");

        // 1. Heap Info (Internal SRAM vs External PSRAM)
        size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        size_t internal_largest =
            heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        printf("HEAP_INT_TOTAL: %d\n", internal_total);
        printf("HEAP_INT_FREE: %d\n", internal_free);
        printf("HEAP_INT_MAX_BLOCK: %d\n", internal_largest);
        printf("HEAP_EXT_FREE: %d\n", spiram_free);

        // 2. Task List (requires CONFIG_FREERTOS_USE_TRACE_FACILITY)
        // Allocate a large buffer to hold the task list text
        char *taskListBuffer = (char *)malloc(4096);
        if (taskListBuffer) {
          // vTaskList writes to the buffer: Name, State, Priority, Stack,
          // TaskNum
          vTaskList(taskListBuffer);
          printf("TASK_LIST:\n%s\n", taskListBuffer);
          free(taskListBuffer);
        } else {
          ESP_LOGE(TAG, "Failed to allocate buffer for vTaskList");
        }

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
        // 3. Run Time Stats
        char *runTimeStatsBuffer = (char *)malloc(4096);
        if (runTimeStatsBuffer) {
          vTaskGetRunTimeStats(runTimeStatsBuffer);
          printf("RUN_TIME_STATS:\n%s\n", runTimeStatsBuffer);
          free(runTimeStatsBuffer);
        } else {
          ESP_LOGE(TAG, "Failed to allocate buffer for vTaskGetRunTimeStats");
        }
#endif

        printf("--- END RESOURCE LOG ---\n");

        // Task self-delete
        vTaskDelete(NULL);
      },
      "resource_monitor", 4096, NULL, 1, NULL);
}
