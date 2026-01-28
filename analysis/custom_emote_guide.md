# Tutorial: How to Customise Emotes (animations)

This guide shows you how to replace the robot face expressions with your own animations using **Figma + Lottie**.

**Core Concept**: The ESP32 "Emote Style" uses a special format called **`.eaf`** (Espressif Animation Format) or **GIFs** converted to this format.

## Overview of Workflow
1.  **Design**: Create animation in **Figma** (with Lottie plugin) or After Effects.
2.  **Export**: Export as **Lottie JSON**.
3.  **Convert to GIF**: Convert Lottie to GIF (using LottieFiles website or plugin).
4.  **Convert to EAF**: Convert GIF to `.eaf` using Espressif's tool.
5.  **Upload**: Put the file on the device.
6.  **Configure**: Update `emote.json`.

---

## Step 1: Design & Export (Figma -> GIF)
1.  **Figma**: Create your frame (e.g., 240x240 or 320x240 depending on screen).
2.  **Lottie**: Use the **LottieFiles** plugin to animate and export as **JSON**.
3.  **To GIF**:
    *   Go to [LottieFiles.com/preview](https://lottiefiles.com/preview).
    *   Upload your JSON.
    *   Click "Convert to GIF".
    *   **Important**: Keep the resolution low (e.g., match your screen width, like 240px or 320px) to save performance.

## Step 2: Convert GIF to Device Format (.eaf)
The device needs a highly optimized format.
1.  Open the [Espressif GIF Tool](https://esp32-gif.espressif.com/).
2.  **Color Format**: Select **RGB565** (No transparency) or **RGB565A8** (If you need transparent background).
    *   *Tip*: Using RGB565 (black background) is faster and saves memory.
3.  **Upload GIF**: Select your GIF file.
4.  **Convert**: Click generate. It will download a file (e.g., `my_anim.eaf` or `.bin`).
    *   Make sure to rename it to `my_emote.eaf`.

## Step 3: Install the Emote
You need to put this file into the device's filesystem.

### Option A: Build into Firmware (Easiest for Dev)
1.  Copy your `.eaf` file to `main/assets/animations/` (create folder if needed).
    *   *Note*: You might need to check where `emote.json` loads from.
    *   Actually, the project downloads assets from a URL or reads from SPIFFS partition.
    *   **Recommendation**: Place it in `main/board/esp-box-3/assets/` if it exists, or just replace an existing file to test.
    *   **Correct Way**: The `emote.json` implies files are loaded by filename.
    *   If using the "Assets Partition", put files in a folder that gets flashed to the `assets` partition.

### Option B: Update `emote.json`
1.  Open `main/boards/esp-box-3/emote.json`.
2.  Find the emotion you want to change (e.g., "happy").
    ```json
    {"emote": "happy", "src": "my_emote.eaf", "loop": true, "fps": 20}
    ```
3.  **Flash Filesystem**: You need to upload these files.
    *   If the project uses a "Storage" URL, you must upload the file to your server.
    *   If you are testing locally, use the "Upload Tool" (if available) or `idf.py build flash` if the assets are compiled in.

## Step 4: Settings Verification
Ensure your project is using the "Emote Styles".
*   In `menuconfig`: `Component config` -> `BSP` -> `Display Style` -> **Emote**.

---

## Detailed Config (Reference)
*   **FPS**: 20-30 is recommended for ESP32.
*   **Resolution**: 320x240 (Landscape) is standard for BOX-3.
