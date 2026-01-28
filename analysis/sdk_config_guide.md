# SDK Configuration (menuconfig) Guide

This guide explains the key settings you see in the **ESP-IDF SDK Configuration Editor** (or `idf.py menuconfig`), specifically for the **Xiaomi ESP32** project and your **BOX-3** board.

## 1. Entering the Configuration
In VS Code, click the "Gear" icon (SDK Configuration Editor) at the bottom, or run `idf.py menuconfig` in the terminal.
Focus on the top-level menu often labeled **"Xiaozhi Assistant"** (Project Configuration).

## 2. Critical Settings for BOX-3

### A. Board Selection (Crucial!)
*   **Setting Name**: `Board Type`
*   **What it does**: Tells the code which pins correspond to the screen, audio, and buttons.
*   **Your Choice**: Select **`Espressif ESP-BOX-3`**.
    *   *Why?* If you choose the wrong board (e.g., standard S3), the screen won't turn on, and audio won't work because the pin mappings will be wrong.

### B. Wake Word (唤醒词)
*   **Setting Name**: `Wake Word Implementation Type`
*   **What it does**: Decides how the device listens for "Hi Xiaozhi".
*   **Your Choice**: **`Wakenet model with AFE`** (Recommended for S3).
    *   **AFE (Audio Front End)**: Includes Echo Cancellation (AEC) and Noise Reduction. This makes it much better at hearing you while it is playing music.
    *   *Note*: Ensure `Enable Audio Noise Reduction` is also checked (it usually is by default).

### C. Display Style (显示风格)
*   **Setting Name**: `Select display style`
*   **Options**:
    *   `Enable default message style`: Standard text UI.
    *   `Emote animation style`: The "Robot Face" with animated eyes.
*   **Your Choice**: **`Emote animation style`** is the coolest for BOX-3, but you can choose **Default** if you prefer a text-heavy interface.

### D. Flash Assets (烧录资源)
*   **Setting Name**: `Flash Assets`
*   **What it does**: The device needs fonts, images, and startup sounds stored in a special partition.
*   **Your Choice**: **`Flash Default Assets`** (Keep this selected).
    *   If you select "Do not flash", your device might crash on startup complaining about missing files.

## 3. Other Useful Settings

*   **Default Language**:
    *   Sets the default UI language (Chinese/English/etc.). Set to your preference.

*   **OTA URL**:
    *   `https://api.tenclass.net/xiaozhi/ota/`
    *   This is where the device checks for software updates. Leave this default unless you are running your own private OTA server.

*   **WiFi Configuration Method**:
    *   **`Hotspot`** (Recommended): The device creates a WiFi AP (like `Xiaozhi-xxxx`). You connect to it with your phone to send your home WiFi password.
    *   **`Esp Blufi`**: Uses Bluetooth to configure WiFi (requires Espressif app).

## 4. Summary Checklist for You
1.  [ ] **Board Type**: `Espressif ESP-BOX-3`
2.  [ ] **Wake Word**: `Wakenet model with AFE`
3.  [ ] **Flash Assets**: `Flash Default Assets`
4.  [ ] **Display Style**: `Emote` (or your preference)

Once set, click **Save** (disk icon) and then **Build** your project.
