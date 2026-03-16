#include "imu_streamer.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "sensor_icm42607.h"
#include <ctype.h>
#include <numeric>
#include <stdio.h>
#include <string.h>
#include <vector>

// Low-level USB Serial JTAG
#include "driver/usb_serial_jtag.h"

static const char *TAG = "imu_ai_lib";

ESP_EVENT_DEFINE_BASE(IMU_AI_EVENT);

// --- Internal State ---
static imu_ai_config_t g_config;
static bool g_is_initialized = false;
static esp_event_loop_handle_t g_event_loop =
    NULL; // Optional: If we want a private loop. For now use default.
static TaskHandle_t g_task_handle = NULL;
static bool g_stop_requested = false;

// --- Post-Processing Configuration ---
struct ClassConfig {
  float confidence_threshold;
  int min_consecutive_frames;
};

struct InferenceConfig {
  ClassConfig default_cfg;
  ClassConfig tap_cfg;
  ClassConfig shake_cfg;
  ClassConfig idle_cfg;
};

// Global config - Tunable parameters
static InferenceConfig inf_config = {
    .default_cfg = {.confidence_threshold = 0.70f, .min_consecutive_frames = 5},
    .tap_cfg = {.confidence_threshold = 0.98f, .min_consecutive_frames = 18},
    .shake_cfg = {.confidence_threshold = 0.85f, .min_consecutive_frames = 3},
    .idle_cfg = {.confidence_threshold = 0.50f, .min_consecutive_frames = 4},
};

struct InferenceState {
  int current_label_index;
  int candidate_label_index;
  int consecutive_count;
};

static InferenceState inf_state = {
    .current_label_index = -1,
    .candidate_label_index = -1,
    .consecutive_count = 0,
};

// Global Feature Buffer
static float *features = nullptr;
static size_t feature_ix = 0;

// --- CLI Helper functions (Keep existing logic but scoped) ---
static void print_usb(const char *format, ...) {
  if (!g_config.enable_cli)
    return;
  char loc_buf[256];
  va_list args;
  va_start(args, format);
  int n = vsnprintf(loc_buf, sizeof(loc_buf), format, args);
  va_end(args);
  if (n > 0) {
    usb_serial_jtag_write_bytes(loc_buf, n, 10);
  }
}

// ... (CLI Parsing Logic omitted for brevity but should be preserved or
// simplified) ... For this refactor, I'll keep a simplified CLI for threshold
// tuning or remove it if too complex. Let's keep the core parsing logic but
// simplify integration.

static bool parse_value(const char *token, float *f_val, int *i_val) {
  if (!token)
    return false;
  *f_val = atof(token);
  *i_val = atoi(token);
  return true;
}

static void handle_config_cmd(const char *cmd, const char *param,
                              const char *val_str, ClassConfig *cfg,
                              const char *name) {
  float f_val;
  int i_val;
  if (!parse_value(val_str, &f_val, &i_val))
    return;

  if (strcmp(param, "conf") == 0 || strcmp(param, "c") == 0) {
    cfg->confidence_threshold = f_val;
    print_usb(">> [OK] %s Confidence set to %.2f\n", name,
              cfg->confidence_threshold);
  } else if (strcmp(param, "frame") == 0 || strcmp(param, "f") == 0) {
    cfg->min_consecutive_frames = i_val;
    print_usb(">> [OK] %s Frames set to %d\n", name,
              cfg->min_consecutive_frames);
  }
}

// Internal debug flag
static bool g_raw_debug_mode = false; // Default off

static void process_line(char *line) {
  char *cmd = strtok(line, " ");
  if (!cmd) {
    print_usb("box3> ");
    return;
  }

  if (strcmp(cmd, "tap") == 0) {
    char *p = strtok(NULL, " ");
    char *v = strtok(NULL, " ");
    if (p && v)
      handle_config_cmd(cmd, p, v, &inf_config.tap_cfg, "Tap");
  } else if (strcmp(cmd, "shake") == 0) {
    char *p = strtok(NULL, " ");
    char *v = strtok(NULL, " ");
    if (p && v)
      handle_config_cmd(cmd, p, v, &inf_config.shake_cfg, "Shake");
  } else if (strcmp(cmd, "idle") == 0) {
    char *p = strtok(NULL, " ");
    char *v = strtok(NULL, " ");
    if (p && v)
      handle_config_cmd(cmd, p, v, &inf_config.idle_cfg, "Idle");
  } else if (strcmp(cmd, "debug") == 0) {
    const char *v = strtok(NULL, " ");
    if (v) {
      if (strcmp(v, "on") == 0) {
        g_raw_debug_mode = true;
        print_usb(">> [OK] Raw Debug ON\n");
      } else if (strcmp(v, "off") == 0) {
        g_raw_debug_mode = false;
        print_usb(">> [OK] Raw Debug OFF\n");
      } else {
        print_usb("Usage: debug <on|off>\n");
      }
    } else {
      print_usb("Usage: debug <on|off>\n");
    }
  } else if (strcmp(cmd, "status") == 0) {
    print_usb("\n--- System Status ---\n");
    print_usb("Raw Debug:    %s\n", g_raw_debug_mode ? "ON" : "OFF");
    print_usb("Default Conf: %.2f\n",
              inf_config.default_cfg.confidence_threshold);
    print_usb("[Tap]   Conf: %.2f, Frames: %d\n",
              inf_config.tap_cfg.confidence_threshold,
              inf_config.tap_cfg.min_consecutive_frames);
    print_usb("[Shake] Conf: %.2f, Frames: %d\n",
              inf_config.shake_cfg.confidence_threshold,
              inf_config.shake_cfg.min_consecutive_frames);
    print_usb("[Idle]  Conf: %.2f, Frames: %d\n",
              inf_config.idle_cfg.confidence_threshold,
              inf_config.idle_cfg.min_consecutive_frames);
    print_usb("Last Detection: %s\n",
              inf_state.current_label_index >= 0 ? "Active" : "None");
  } else if (strcmp(cmd, "help") == 0) {
    print_usb("\nCommands:\n");
    print_usb("  status                  Show current config\n");
    print_usb("  debug <on|off>          Toggle Raw Sensor Logs\n");
    print_usb("  tap conf <float>        Set Tap Threshold (0.1-1.0)\n");
    print_usb("  tap frame <int>         Set Tap Min Frames\n");
    print_usb("  shake conf <float>      Set Shake Threshold\n");
    print_usb("  idle conf <float>       Set Idle Threshold\n");
  } else {
    print_usb("Unknown command: %s. Try 'help'.\n", cmd);
  }
  print_usb("box3> ");
}

static void check_usb_input() {
  if (!g_config.enable_cli)
    return;
  uint8_t data[64];
  int len = usb_serial_jtag_read_bytes(data, sizeof(data), 0);
  if (len > 0) {
    static char line_buffer[128];
    static int line_pos = 0;
    usb_serial_jtag_write_bytes(data, len, 0); // Echo
    for (int i = 0; i < len; i++) {
      if (data[i] == '\n' || data[i] == '\r') {
        if (line_pos > 0) {
          line_buffer[line_pos] = 0;
          print_usb("\n");
          process_line(line_buffer);
          // Prompt is now handled inside process_line to ensure it appears
          // after command output
          line_pos = 0;
        } else {
          // Empty enter
          print_usb("\nbox3> ");
        }
      } else if (line_pos < 127) {
        line_buffer[line_pos++] = data[i];
      }
    }
  }
}

// --- Inference Logic ---

void process_inference_result(ei_impulse_result_t *result) {
  int max_idx = -1;
  float max_val = -1.0f;

  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result->classification[ix].value > max_val) {
      max_val = result->classification[ix].value;
      max_idx = ix;
    }
  }

  const char *label = result->classification[max_idx].label;
  ClassConfig *active_cfg = &inf_config.default_cfg;
  imu_ai_event_id_t event_id = IMU_AI_EVT_UNKNOWN;

  if (strstr(label, "tap") || strstr(label, "Tap")) {
    active_cfg = &inf_config.tap_cfg;
    event_id = IMU_AI_EVT_TAP;
  } else if (strstr(label, "shake") || strstr(label, "Shake")) {
    active_cfg = &inf_config.shake_cfg;
    event_id = IMU_AI_EVT_SHAKE;
  } else if (strstr(label, "idle") || strstr(label, "Idle")) {
    active_cfg = &inf_config.idle_cfg;
    active_cfg->confidence_threshold = 0.5f; // Lower threshold for idle
    event_id = IMU_AI_EVT_IDLE;
  }

  // Debounce Logic
  if (max_val < active_cfg->confidence_threshold) {
    inf_state.consecutive_count = 0;
  } else {
    if (max_idx == inf_state.candidate_label_index) {
      inf_state.consecutive_count++;
    } else {
      inf_state.candidate_label_index = max_idx;
      inf_state.consecutive_count = 1;
    }

    if (inf_state.consecutive_count >= active_cfg->min_consecutive_frames) {
      if (inf_state.current_label_index != inf_state.candidate_label_index) {
        inf_state.current_label_index = inf_state.candidate_label_index;

        // State Changed -> Post Event
        if (event_id != IMU_AI_EVT_UNKNOWN) {
          esp_event_post(IMU_AI_EVENT, event_id, NULL, 0, 0);
          print_usb("[EVT] %s (%.2f)\n", label, max_val);
        }
      }
      inf_state.consecutive_count = active_cfg->min_consecutive_frames;
    }
  }
}

// --- Library Interface ---

esp_err_t imu_ai_init(const imu_ai_config_t *config) {
  if (!config || !config->i2c_bus_handle) {
    return ESP_ERR_INVALID_ARG;
  }
  g_config = *config;

  // 1. Initialize Driver with injected handle
  esp_err_t ret =
      sensor_icm42607_init((i2c_master_bus_handle_t)g_config.i2c_bus_handle);
  if (ret != ESP_OK)
    return ret;

  // 2. Alloc Memory
  if (features == nullptr) {
    features =
        (float *)malloc(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE * sizeof(float));
  }
  if (!features)
    return ESP_ERR_NO_MEM;

  // 3. Init USB CLI if enabled
  if (g_config.enable_cli) {
    usb_serial_jtag_driver_config_t usb_cfg = {.tx_buffer_size = 1024,
                                               .rx_buffer_size = 1024};
    // Try to install. If it fails (already installed), we proceed anyway.
    usb_serial_jtag_driver_install(&usb_cfg);

    // Always print banner if CLI is enabled
    print_usb("\033[2J\033[H\n*** IMU AI CLI (Lib Mode) ***\nbox3> ");
  }

  inf_config.default_cfg.confidence_threshold = g_config.default_threshold;
  g_is_initialized = true;
  return ESP_OK;
}

static void imu_worker_task(void *arg) {
  // Optimized: 25Hz (40ms) to reduce CPU usage.
  // Previously 50Hz (20ms) which caused ~33% CPU load.
  const int64_t interval_ms = 40;
  const TickType_t xFrequency = pdMS_TO_TICKS(interval_ms);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  icm42607_data_t sens_data = {0};
  int loop_count = 0;

  while (!g_stop_requested) {
    // 1. Poll USB Input (Throttled: every 10 cycles = 400ms)
    // Reduce I/O overhead in the critical path
    if (loop_count++ % 10 == 0) {
      check_usb_input();
    }

    // 2. Precise Scheduling (Yields CPU completely between ticks)
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    if (sensor_icm42607_read_data(&sens_data) == ESP_OK) {
      static int debug_cnt = 0;
      if (g_raw_debug_mode && ++debug_cnt % 50 == 0) {
        ESP_LOGI(TAG, "Raw: Acc(%.2f, %.2f, %.2f) Gyro(%.2f, %.2f, %.2f)",
                 sens_data.acc_x, sens_data.acc_y, sens_data.acc_z,
                 sens_data.gyr_x, sens_data.gyr_y, sens_data.gyr_z);
      }
      // ... DSP logic same as before ...
      size_t axes = EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
      size_t samples_to_shift = 1;
      memmove(features, features + (axes * samples_to_shift),
              (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - (axes * samples_to_shift)) *
                  sizeof(float));
      size_t append_idx = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - axes;
      features[append_idx + 0] = sens_data.acc_x;
      features[append_idx + 1] = sens_data.acc_y;
      features[append_idx + 2] = sens_data.acc_z;
      features[append_idx + 3] = sens_data.gyr_x;
      features[append_idx + 4] = sens_data.gyr_y;
      features[append_idx + 5] = sens_data.gyr_z;

      if (feature_ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        feature_ix += axes;
        continue;
      }

      signal_t signal;
      numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
                                &signal);
      ei_impulse_result_t result = {0};
      if (run_classifier(&signal, &result, false) == EI_IMPULSE_OK) {
        process_inference_result(&result);
      }
    }
  }
  vTaskDelete(NULL);
}

esp_err_t imu_ai_start(uint8_t priority, int core_id) {
  if (!g_is_initialized)
    return ESP_ERR_INVALID_STATE;
  g_stop_requested = false;

  xTaskCreatePinnedToCore(imu_worker_task, "imu_ai_task", 4096, NULL, priority,
                          &g_task_handle, core_id);
  return ESP_OK;
}

esp_err_t imu_ai_stop(void) {
  g_stop_requested = true;
  return ESP_OK;
}

esp_err_t imu_ai_register_event_handler(esp_event_handler_t callback,
                                        void *arg) {
  return esp_event_handler_register(IMU_AI_EVENT, ESP_EVENT_ANY_ID, callback,
                                    arg);
}

void imu_streamer_start_legacy(void) {
  // Wrapper for legacy compatibility if needed
  // Not implemented fully as we want to force new API usage
}
