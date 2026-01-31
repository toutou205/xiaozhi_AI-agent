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

static const char *TAG = "imu_streamer";

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
  int print_interval_ms;
};

// Global config - Tunable parameters
static InferenceConfig inf_config = {
    .default_cfg = {.confidence_threshold = 0.70f, .min_consecutive_frames = 5},
    .tap_cfg = {.confidence_threshold = 0.98f, .min_consecutive_frames = 18},
    .shake_cfg = {.confidence_threshold = 0.85f, .min_consecutive_frames = 3},
    .idle_cfg = {.confidence_threshold = 0.50f, .min_consecutive_frames = 4},
    .print_interval_ms = 0,
};

struct InferenceState {
  int current_label_index;
  int candidate_label_index;
  int consecutive_count;
  int64_t last_print_time;
};

static InferenceState inf_state = {
    .current_label_index = -1,
    .candidate_label_index = -1,
    .consecutive_count = 0,
    .last_print_time = 0,
};

// Global Feature Buffer
static float *features = nullptr;
static size_t feature_ix = 0;

// Helper to parsing float/int values safely
static bool parse_value(char *token, float *f_val, int *i_val) {
  if (!token)
    return false;
  *f_val = atof(token);
  *i_val = atoi(token);
  return true;
}

static void print_usb(const char *format, ...) {
  char loc_buf[256];
  va_list args;
  va_start(args, format);
  int n = vsnprintf(loc_buf, sizeof(loc_buf), format, args);
  va_end(args);
  if (n > 0) {
    usb_serial_jtag_write_bytes(loc_buf, n, 10);
  }
}

static void show_help() {
  print_usb("\n=== Help Menu ===\n");
  print_usb("Commands:\n");
  print_usb("  status              : Show current config\n");
  print_usb("  tap conf <0.0-1.0>  : Set Tap confidence threshold\n");
  print_usb("  tap frame <int>     : Set Tap min consecutive frames\n");
  print_usb("  shake conf <0.0-1.0>: Set Shake confidence threshold\n");
  print_usb("  shake frame <int>   : Set Shake min consecutive frames\n");
  print_usb("  idle conf/frame ... : Set Idle config\n");
  print_usb("  help                : Show this message\n");
  print_usb("=================\n");
}

static void handle_config_cmd(char *cmd, char *param, char *val_str,
                              ClassConfig *cfg, const char *name) {
  float f_val;
  int i_val;

  if (!parse_value(val_str, &f_val, &i_val)) {
    print_usb(">> [ERROR] Invalid value format for %s\n", name);
    print_usb(">> Example: %s conf 0.85\n", cmd);
    return;
  }

  if (strcmp(param, "conf") == 0 || strcmp(param, "c") == 0) {
    if (f_val < 0.0f || f_val > 1.0f) {
      print_usb(">> [ERROR] Confidence must be 0.0 - 1.0\n");
      return;
    }
    cfg->confidence_threshold = f_val;
    print_usb(">> [OK] %s Confidence set to %.2f\n", name,
              cfg->confidence_threshold);
  } else if (strcmp(param, "frame") == 0 || strcmp(param, "f") == 0) {
    if (i_val < 1) {
      print_usb(">> [ERROR] Frames must be >= 1\n");
      return;
    }
    cfg->min_consecutive_frames = i_val;
    print_usb(">> [OK] %s Frames set to %d\n", name,
              cfg->min_consecutive_frames);
  } else {
    print_usb(">> [ERROR] Unknown property '%s'. Use 'conf' or 'frame'\n",
              param);
  }
}

static void process_line(char *line) {
  // Basic tokenizer
  char *cmd = strtok(line, " ");
  if (!cmd)
    return;

  if (strcmp(cmd, "status") == 0) {
    print_usb("\n>> --- Current Status ---\n");
    print_usb(">> Tap   : Conf=%.2f, Frames=%d\n",
              inf_config.tap_cfg.confidence_threshold,
              inf_config.tap_cfg.min_consecutive_frames);
    print_usb(">> Shake : Conf=%.2f, Frames=%d\n",
              inf_config.shake_cfg.confidence_threshold,
              inf_config.shake_cfg.min_consecutive_frames);
    print_usb(">> Idle  : Conf=%.2f, Frames=%d\n",
              inf_config.idle_cfg.confidence_threshold,
              inf_config.idle_cfg.min_consecutive_frames);
  } else if (strcmp(cmd, "help") == 0) {
    show_help();
  } else if (strcmp(cmd, "tap") == 0 || strcmp(cmd, "shake") == 0 ||
             strcmp(cmd, "idle") == 0) {
    char *param = strtok(NULL, " ");
    char *val = strtok(NULL, " ");

    // Map command to config object
    ClassConfig *target_cfg = NULL;
    if (strcmp(cmd, "tap") == 0)
      target_cfg = &inf_config.tap_cfg;
    else if (strcmp(cmd, "shake") == 0)
      target_cfg = &inf_config.shake_cfg;
    else if (strcmp(cmd, "idle") == 0)
      target_cfg = &inf_config.idle_cfg;

    if (param && val) {
      handle_config_cmd(cmd, param, val, target_cfg, cmd);
    } else {
      print_usb(">> [ERROR] Missing arguments for %s\n", cmd);
      print_usb(">> Usage: %s [conf|frame] <value>\n", cmd);
    }
  } else {
    print_usb(">> [ERROR] Unknown command: '%s'. Type 'help' for list.\n", cmd);
  }
}

// Re-using the logic from UartDemo for non-blocking read
static void check_usb_input() {
  static char line_buffer[128];
  static int line_pos = 0;
  uint8_t data[64];

  // Non-blocking read
  int len = usb_serial_jtag_read_bytes(data, sizeof(data), 0);

  if (len > 0) {
    // Echo back for better UX
    usb_serial_jtag_write_bytes(data, len, 0);

    for (int i = 0; i < len; i++) {
      // Handle Backspace (0x08) and Delete (0x7F)
      if (data[i] == 0x08 || data[i] == 0x7F) {
        if (line_pos > 0) {
          line_pos--;
          // Visual backspace handling (back, space, back)
          const char *bs = "\b \b";
          usb_serial_jtag_write_bytes(bs, 3, 0);
        }
        continue;
      }

      if (data[i] == '\n' || data[i] == '\r') {
        if (line_pos > 0) {
          line_buffer[line_pos] = '\0';
          print_usb("\n"); // Newline after command enter
          process_line(line_buffer);
          print_usb("box3> "); // Reprompt
          line_pos = 0;
        } else {
          print_usb("\r\nbox3> "); // Empty enter
        }
      } else {
        if (line_pos < sizeof(line_buffer) - 1) {
          // Filter printable chars only
          if (data[i] >= 32 && data[i] <= 126) {
            line_buffer[line_pos++] = (char)data[i];
          }
        }
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

  if (strstr(label, "tap") != NULL || strstr(label, "Tap") != NULL) {
    active_cfg = &inf_config.tap_cfg;
  } else if (strstr(label, "shake") != NULL || strstr(label, "Shake") != NULL) {
    active_cfg = &inf_config.shake_cfg;
  } else if (strstr(label, "idle") != NULL || strstr(label, "Idle") != NULL) {
    active_cfg = &inf_config.idle_cfg;
  }

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

        char buf[128];
        int n = snprintf(buf, sizeof(buf), "State: %s (Conf: %.2f)\n",
                         result->classification[max_idx].label, max_val);
        usb_serial_jtag_write_bytes(buf, n, 0); // Direct USB write
      }
      inf_state.consecutive_count = active_cfg->min_consecutive_frames;
    }
  }
}

void imu_streamer_start(void) {
  ESP_LOGI(TAG, "Starting IMU Streamer with Low-Level USB Driver...");

  // Initialize USB Serial JTAG Driver manually
  usb_serial_jtag_driver_config_t usb_config = {
      .tx_buffer_size = 1024,
      .rx_buffer_size = 1024,
  };

  // Try to install driver, ignore if already installed
  esp_err_t err = usb_serial_jtag_driver_install(&usb_config);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "Failed to install USB driver: %d", err);
  }

  // --- Inference Loop ---
  // const int64_t interval_us = 10000; // 100Hz
  // Increased to 20ms (50Hz) to give more time for USB handling
  const int64_t interval_us = 20000;
  int64_t next_time = esp_timer_get_time();

  icm42607_data_t sens_data = {0};
  esp_err_t rslt;

  features =
      (float *)malloc(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE * sizeof(float));
  if (features == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate feature buffer");
    return;
  }

  // Startup Banner
  print_usb("\033[2J\033[H"); // Clear Screen
  print_usb("\n\n");
  print_usb("*********************************************\n");
  print_usb("*         IMU Gesture CLI Ready             *\n");
  print_usb("*********************************************\n");
  print_usb("* Type 'help' to see available commands.    *\n");
  print_usb("* Example: tap conf 0.95                    *\n");
  print_usb("*********************************************\n");
  print_usb("box3> ");

  while (1) {
    // 1. Poll USB Input (Simulating multitasking)
    check_usb_input();

    int64_t now = esp_timer_get_time();
    if (now < next_time) {
      vTaskDelay(1); // Yield to other tasks
      continue;
    }
    next_time += interval_us;

    rslt = sensor_icm42607_read_data(&sens_data);
    if (rslt == ESP_OK) {
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
      int err = numpy::signal_from_buffer(
          features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
      if (err != 0)
        continue;

      ei_impulse_result_t result = {0};
      err = run_classifier(&signal, &result, false);
      if (err != EI_IMPULSE_OK)
        continue;

      process_inference_result(&result);

    } else {
      // Don't log read errors to avoid flooding if sensor disconnects
    }
  }
}
