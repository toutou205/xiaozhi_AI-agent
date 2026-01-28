# Noto Emoji Integration Guide

This guide explains how to replace the default robot animations with Google's [Noto Emoji Animations](https://googlefonts.github.io/noto-emoji-animation/).

## 1. Mapping Table
Match the Noto Emoji animation to the system request.

| System Key (`emote.json`) | Suggested Noto Emoji | File Name (Example) |
| :--- | :--- | :--- |
| **happy** | 😄 Grinning Face with Smiling Eyes | `smile.json` |
| **laughing** | 😆 Grinning Squinting Face | `laughing.json` |
| **funny** | 🤪 Zany Face | `zany_face.json` |
| **loving** | 🥰 Smiling Face with Hearts | `loving.json` |
| **embarrassed** | 😳 Flushed Face | `flushed.json` |
| **confident** | 😎 Smiling Face with Sunglasses | `sunglasses.json` |
| **delicious** | 😋 Face Savoring Food | `yummy.json` |
| **sad** | 😞 Disappointed Face | `sad.json` |
| **crying** | 😭 Loudly Crying Face | `crying.json` |
| **sleepy** | 😴 Sleeping Face | `sleeping.json` |
| **silly** | 😛 Face with Tongue | `tongue.json` |
| **angry** | 😠 Angry Face | `angry.json` |
| **surprised** | 😮 Face with Open Mouth | `open_mouth.json` |
| **shocked** | 😱 Face Screaming in Fear | `scream.json` |
| **thinking** | 🤔 Thinking Face | `thinking.json` |
| **winking** | 😉 Winking Face | `wink.json` |
| **relaxed** | 😌 Relieved Face | `relieved.json` |
| **confused** | 😕 Confused Face | `confused.json` |
| **neutral** | 😐 Neutral Face | `neutral.json` |
| **idle** | 😶 Face Without Mouth (or any subtle anim) | `idle.json` |

## 2. Conversion Workflow
Noto Emojis are usually provided as **Lottie (JSON)** or **WebP**. The device needs **.eaf** (via GIF).

### Step A: Download
1.  Go to [Noto Emoji Animation](https://googlefonts.github.io/noto-emoji-animation/).
2.  Find the emoji from the list above.
3.  Download the **Lottie (JSON)** version (preferred for clean resizing) or **GIF** if available.

### Step B: Resize (Important!)
The Noto source files (500x500) are too big, and 150x150 might be too small (though acceptable).
1.  **Download**: Get the **500x500 GIF** version.
2.  **Resize**: Go to [Ezgif.com / Resize](https://ezgif.com/resize).
    *   Upload your GIF.
    *   Set **Width** to `240` (or `320`).
    *   Method: Gifsicle (or simple).
    *   Click "Resize Image" and download the result.

### Step C: Convert GIF to EAF
1.  Open [Espressif GIF Tool](https://esp32-gif.espressif.com/).
2.  **Upload**: Select your **Resized GIF**.
3.  **Settings**:
    *   If you see a **Color Format** dropdown: Choose `RGB565` (Best performance, black bg) or `RGB565A8` (Transparency).
    *   **If you do NOT see the option**: The tool likely auto-detects.
        *   If your GIF has transparency -> It makes `RGB565A8`.
        *   If it's opaque -> It makes `RGB565`.
    *   *Tip*: For best performance, add a Black Background in Ezgif (under "Effects") before converting, so you get a smaller, opaque file.
4.  **Generate**: Click the button to download the `.eaf` (or `.bin`).

### Step D: Install & Build (Final Step)

Since you are modifying the core assets, you need to rebuild the `assets.bin` partition.

1.  **Files Ready**: I have already moved your renamed emojis to `main/assets/animations` and copied the necessary UI icons (`icon_wifi`, etc.) there to prevent errors.
2.  **Config Ready**: I have updated `main/boards/esp-box-3/emote.json` to map the system emotions to your new files (e.g., `happy` -> `smile.eaf`).

3.  **Run Build Command**:
    Open your terminal in the project root and run this command. It uses the `spiffs_assets/build.py` tool which correctly handles the `emote.json` metadata (fps, loops).

    ```bash
    python scripts/spiffs_assets/build.py \
    --target_board main/boards/esp-box-3 \
    --res_path main/assets/animations \
    --wakenet_model managed_components/espressif__esp-sr/model/wakenet_model/wn9_nihaoxiaozhi_tts \
    --text_font managed_components/78__xiaozhi-fonts/cbin/font_puhui_common_20_4.bin
    ```

    *   *Note*: This command points the resource path to our new `animations` folder.
    *   **CRITICAL WARNING**: The `assets` partition on ESP-BOX-3 is **8MB**. Your high-quality Noto Emojis (~1.5MB each) quickly exceed this.
        *   I have temporarily removed some large emotes to make the build succeed (~4MB used).
        *   **To add more**: You must resize them smaller (e.g., 160x160) or reduce frame count in the EAF converter to fit all of them within 8MB.

4.  **Flash Assets**:
    After the build completes, flash the generated assets binary:
    ```bash
    parttool.py write_partition --partition-name assets --input scripts/spiffs_assets/build/assets.bin
    ```
    *   *Note*: This updates *only* the assets. You can also run `idf.py flash` if you configured custom assets properly, but `parttool` is faster for testing.
