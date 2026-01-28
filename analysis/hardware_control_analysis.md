# ESP32-S3-BOX-3 Hardware & Control Analysis

## 1. Voice Control Capabilities
The device primarily operates as a **Voice Client**. It does not perform complex speech recognition locally (except for the wake word).

*   **Offline/Local Control:**
    *   **Wake Word**: "Hi Xiaozhi" (or similar). The device listens locally for this key phrase.
    *   **Logic**: Once woken, it records audio and streams it to the server.
    *   **Local Commands**: There are **no other offline voice commands** (e.g., "Turn on light") enabled by default in the examined code. All semantic understanding happens on the server side.

*   **Online Control (Server-Side):**
    *   The server (Xiaozhi/MCP) processes the speech text.
    *   It can send back JSON commands like `sys_cmd` (e.g., "reboot", "stop") or controlling IoT devices (via Home Assistant integration, if configured).

## 2. Hardware Control IOs (ESP32-S3-BOX-3)
Based on `main/boards/esp-box-3/config.h` and board logic, here are the pins you can control or that are already in use.

### A. User-Controllable IOs
These are clearly defined and available for interaction:
*   **BOOT Button**: `GPIO 0` (Used for "Toggle Chat" or "Config Mode").
*   **Backlight**: `GPIO 47` (PWM Controlled).
*   **Expansion Header (Pmod)**: The BOX-3 has a specific headers. While not explicitly defined in `config.h` as "User IO", the standard BOX-3 header exposes:
    *   `GPIO 8` (SDA) / `GPIO 18` (SCL) - Shared with codec, can attach I2S sensors (check address conflicts).
    *   *Note: Other GPIOs are heavily used by the Display and Audio subsystems.*

### B. System Reserved IOs (Do NOT modify unless expert)
*   **I2S Audio (Codec)**:
    *   MCLK: `GPIO 2`
    *   BCLK: `GPIO 17`
    *   WS: `GPIO 45`
    *   DOUT: `GPIO 15`
    *   DIN: `GPIO 16`
    *   **PA Control (Speaker Enable)**: `GPIO 46`
*   **Display (ILI9341)**:
    *   CS: `GPIO 5`
    *   DC: `GPIO 4`
    *   CLK: `GPIO 7`
    *   MOSI: `GPIO 6`
    *   RST: `GPIO 48`

## 3. Speaker Replacement Analysis
You asked if the speaker can be replaced because the sound quality is poor.

*   **Current Hardware Interface**:
    *   **Audio Codec**: Uses **ES8311** (DAC/ADC) and **ES7210** (ADC).
    *   **Output Path**: The I2S signal goes to the ES8311 Codec.
    *   **Amplifier**: The BOX-3 usually has built-in amplification (NS4168 or similar) driven by the codec's analog output.
    *   **Control Pin**: `GPIO 46` is used to enable/disable the Power Amplifier (PA) to prevent popping noises.

*   **Replacement Feasibility**: **YES, but with caveats.**
    *   **Interface**: The board usually has a **2-pin JST-style connector** for the speaker.
    *   **Impedance/Power**: You must match the amplifier's specs. Typically **4Ω or 8Ω**, rated for **3W**.
    *   **Why quality is poor**:
        1.  **Small Cavity**: The BOX-3 enclosure is small, limiting bass.
        2.  **Driver Quality**: The stock speaker is basic.
    *   **Recommendation**:
        *   You **CAN** unplug the internal speaker and connect a better quality driver (e.g., a larger cavity speaker from a laptop or portable speaker) to the same header.
        *   **Do NOT** connect a passive speaker that requires high power (e.g., 10W+), the internal amp cannot drive it.
        *   **DAC Mode**: If you want *Hi-Fi* quality, you would need an **external I2S DAC/Amp** (e.g., MAX98357A) connected to the GPIO pins, but this requires significant code changes (remapping I2S pins).
