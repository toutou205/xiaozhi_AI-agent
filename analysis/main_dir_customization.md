# Main Directory Customization Guide (ESP-BOX-3 Focus)

This guide provides a roadmap of the `main/` directory, specifically tailored for customizing the **ESP32-S3-BOX-3**.

## 1. Directory Overview (`main/`)

| File/Folder | Purpose | Customization Potential |
| :--- | :--- | :--- |
| **`application.cc`** | **The Brain.** Main event loop. Handles WiFi events, wake word triggers, and coordinates Audio <-> Network <-> Display. | **High:** Edit this to change *behavior* (e.g., what happens when wake word is detected, or adding new state logic). |
| **`boards/`** | Hardware definitions. | **Critical:** Contains your BOX-3 specific pinouts and initialization code. |
| **`display/`** | UI logic. | **High:** Modify `emote_display.cc` for the robot face or `lvgl_display/` for standard UI. |
| **`audio/`** | Audio pipeline (I2S reading, Codec control). | **Medium:** Only touch if replacing microphones or speakers. |
| **`protocols/`** | MQTT, WebSocket, HTTP logic. | **Medium:** Edit if you are connecting to a custom non-Xiaozhi backend. |
| **`assets/`** | Fonts, Sounds, Locales. | **High:** Add new languages, custom fonts, or startup sounds here. |

---

## 2. ESP-BOX-3 Specific Customization
Your board's logic lives in: **`main/boards/esp-box-3/`**

### A. Changing Hardware Pins (`config.h`)
*   **File**: `main/boards/esp-box-3/config.h`
*   **Use Case**: You want to plug a sensor into the headers.
*   **Action**: Check this file to see which GPIOs are free.
    *   `I2S_GPIO_...`: Audio pins (Do not touch).
    *   `I2C_GPIO_...`: Touch screen & Audio config pins (Shared bus).
*   **Customization**: If you add a new button or LED, define its GPIO here.

### B. Changing Hardware Initialization (`esp_box3_board.cc`)
*   **File**: `main/boards/esp-box-3/esp_box3_board.cc`
*   **Use Case**: You added a new I2C sensor (e.g., AHT20 temp sensor) or want to change display brightness logic.
*   **Action**: Add your sensor initialization code in the `Initialize()` method.

### C. Customizing "Emotes" (`emote.json` / `flash_args`)
*   **File**: `main/boards/esp-box-3/emote.json`
*   **Use Case**: You want the robot to look "surprised" when it is actually "happy".
*   **Action**: Change the mapping in this JSON file.

---

## 3. Common Customization Scenarios

### Scenario 1: "I want to change what happens when I say 'Hi Xiaozhi'"
1.  **Entry Point**: `main/application.cc`
2.  **Look for**: `Application::WakeWordDetected()` (or similar event handler).
3.  **Action**: You can add code here to toggle a GPIO (turn on a light) *immediately* upon wake, before waiting for the server.

### Scenario 2: "I want to add a Temperature Sensor"
1.  **Wiring**: Connect sensor to the I2C pins (defined in `config.h`, likely GPIO 8/18 on Pmod header).
2.  **Driver**: Add the sensor driver component to `idf_component.yml`.
3.  **Init**: Initialize it in `main/boards/esp-box-3/esp_box3_board.cc`.
4.  **Logic**: Read the sensor in `main/application.cc` (e.g., in a timer loop) and send the data via MQTT using `protocols/`.

### Scenario 3: "I want to change the UI completely"
1.  **Style**: Choose `Emote` vs `Default` in `menuconfig`.
2.  **Code**:
    *   **Emote Mode**: Edit `main/display/emote_display.cc`.
    *   **Standard Mode**: Edit `main/display/lvgl_display/lvgl_display.cc`.
3.  **Assets**: Add new images to `main/assets/`.

### Scenario 4: "I want to connect to my own private server"
1.  **Config**: Change `OTA_URL` in `menuconfig`.
2.  **Protocol**: If you use a different protocol than the default WebSocket/MQTT structure, edit `main/protocols/websocket_protocol.cc` or `mqtt_protocol.cc`.
