
### 3.1 Data Reliability & Verification (数据可靠性论证)

*   **Data Source (数据来源)**:
    *   **File**: `current_log.txt` (Line 2-5)
    *   **Origin Code**: `main/resource_monitor.cc` -> `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)`
    *   **Mechanism**: The data is a **real-time runtime snapshot** taken 30 seconds after boot.
*   **Validity (真实性)**:
    *   The data is **True and Accurate** for the specific moment of capture (T+30s).
    *   **Limitation**: As a snapshot, it may miss peak usage spikes during heavy operations (e.g., initial WiFi connection or OTA writing). Continuous monitoring (e.g., every 1s) would be required for a complete profile.

### 3.2 SRAM Warning Threshold (SRAM 预警线)

A warning line has been added at **Free < 80KB**.

*   **Rationale (数理依据)**:
    1.  **WiFi/BT Burst**: Espressif documentation states WiFi/BT stacks can require **~50KB** of contiguous memory during heavy RX/TX or connection handling.
    2.  **SSL/TLS Overhead**: A single secure HTTPS connection (mbedtls) requires **~30KB** for handshake buffers (input/output).
    3.  **Fragmentation Safety Factor**: `HEAP_INT_FREE` (Total Free) is often larger than `HEAP_INT_MAX_BLOCK` (Largest Contiguous). In your log, Free is **81KB** but Max Block is **59KB**.
*   **Conclusion**: reliable system operation requires a "Safety Airbags" buffer.
    *   **Critical < 50KB**: High risk of connection drop or allocation failure.
    *   **Warning < 80KB**: Healthy margin for fragmentation and concurrent tasks.

### 3.3 PSRAM Optimization Strategy (PSRAM 调优)

*   **Observation**: PSRAM usage is very low (~1MB used out of 16MB), while SRAM is tight (~80KB free).
*   **Root Cause**: By default, ESP32 allocates memory in Internal SRAM for speed. Only explicit requests (`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`) or large buffers configured in `menuconfig` go to PSRAM.
*   **Optimization Recommendations**:
    1.  **Move Audio Buffers**: Ensure audio ringbuffers (Maix/ESP-ADF) are allocated in PSRAM.
    2.  **Move Display Buffers**: LVGL frame buffers should be in PSRAM (check `lv_conf.h` or display driver config).
    3.  **Enable Stack in PSRAM**: In `menuconfig`, enable `Support for external RAM` -> `Allow .bss segment placed in external memory` (Advanced).
    4.  **WiFi Buffers**: Enable `WiFi: Allocate buffers in SPIRAM` in `menuconfig` to free up ~30-40KB of internal SRAM.
