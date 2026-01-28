# Advanced Configuration & Build Optimization Guide

## 1. Detailed Kconfig Explanation
Here is a breakdown of the important `menuconfig` options found in `Kconfig.projbuild`.

### Xiaozhi Assistant (Root Menu)
*   **OTA URL**: The server address for checking updates. Default is `api.tenclass.net`. Change this only if you operate your own update server.
*   **Flash Assets**:
    *   `Default`: Flashes standard sound/image pack. **Use this.**
    *   `Custom`: Allows checking a custom asset file.
    *   `None`: Dangerous, device may fail if assets are missing.
*   **Default Language**: Sets the initial system language (e.g., `Chinese`, `English`).

### Board Type (CRITICAL)
This selects the hardware definition.
*   **`Espressif ESP-BOX-3`**: **MUST SELECT THIS for your board.**
*   *Others*: Definitions for various other dev kits (Korvo, M5Stack, etc.). Selecting the wrong one breaks pins.

### Display Style
*   **`Emote animation style`**: The Robot Face. Efficient, uses custom GFX engine.
*   **`Default message style`**: Text/List based UI. Uses LVGL.
*   **`WeChat`**: A QR-code style interface (specific use case).

### Wake Word Type
*   **`Wakenet model with AFE`**: (Recommended) Uses "Audio Front End" for echo cancellation + wake word. Best performance on S3.
*   **`Wakenet model without AFE`**: Older/simpler. Lower performance.
*   **`Custom Wake Word`**: Allows defining your own phrase (e.g., "Xiao Tu Dou") via phonemes.

### Audio Configuration
*   **`Enable Audio Noise Reduction`**: Turns on the algorithm to clean up microphone input. Keep **YES**.
*   **`Enable Device-Side AEC`**: Acoustic Echo Cancellation. Prevents the device from hearing itself play music. Keep **NO** unless you have tailored hardware tuning (BOX-3 usually works better with the AFE option above).

---

## 2. Compilation Speedup Tips

The ESP-IDF build system is CMake-based. It is *incremental* by default, meaning if you change one file, it only recompiles that file. However, linking can still take time.

### A. How to Build Faster
1.  **Do NOT Clean**: Never run `idf.py fullclean` unless you have a strange error. Just run `idf.py build`.
2.  **CCache (Compiler Cache)**:
    *   ESP-IDF supports `ccache`. Ensure it is installed on your system.
    *   In `menuconfig` -> `Component config` -> `Build type`, check if "Enable ccache" is present (usually auto-enabled if found).
3.  **Parallel Builds**:
    *   Careful: `idf.py` usually auto-detects CPU cores.
4.  **Targeted Building**:
    *   If you only changed the UI code, you generally still run `idf.py build` because the final binary needs to be linked. There is no easy "partial link" for embedded firmware.
    *   *However*, `idf.py flash` is smart. It only writes changed partitions.
    *   **Tip**: Use `idf.py app-flash` to only flash the application code, skipping the bootloader and partition table (saves ~10 seconds).

### B. "It scans too much!"
If `cmake` takes too long to scan for components:
*   This is hard to avoid in the first run. Subsequent runs should be fast (<2 seconds to start compiling) if you don't touch `CMakeLists.txt`.

## 3. Troubleshooting

### "CreateProcess failed" / "The parameter is incorrect"
**Error**: `ninja: fatal: CreateProcess: The parameter is incorrect. (is the command line too long?)`

**Cause**: You hit the Windows command line character limit (32,768 chars). This happens because there are too many include paths in the project or `ccache` is expanding the arguments too much.

**Solutions**:
1.  **Disable CCache (Recommended First Step)**:
    *   Go to `menuconfig` -> `Component config` -> `Build type`.
    *   Uncheck `Enable ccache`.
    *   This removes the ccache wrapper, shortening the command line significantly.
2.  **Shorten Project Path**:
    *   Your path `D:/Projecet/xiaozhi-esp32-2.1.0/` is moderately long. Moving it to `D:/xz` can save ~30 characters per include, which adds up.
3.  **Enable Long Paths**:
    *   Edit Windows Registry (`LongPathsEnabled`) to allow >260 chars, though `CreateProcess` limit is harder to bypass. Disabling CCache is usually the fix.

### "ninja: error: failed recompaction: Permission denied"
**Error**: `ninja: error: failed recompaction: Permission denied`

**Cause**: A file in the `build/` directory (like `.ninja_deps`) is locked by another process. This is common on Windows if:
*   You have two build terminals open.
*   The previous build crashed but passed `ninja.exe` is still running.
*   Antivirus is scanning the build folder.
*   VS Code is indexing files aggressively.

**Solution**:
1.  **Delete the `build` folder**: This is the fastest fix. Run `idf.py fullclean` or manually delete the folder.
2.  **Restart VS Code**: If deleting fails (saying "File in use"), close VS Code, delete the folder, and reopen.
