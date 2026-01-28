# Compilation & Flashing Guide for ESP32-S3-BOX-3

This guide assumes you have the **ESP-IDF v5.5.2** environment already set up.

## 1. Set the Target
First, tell ESP-IDF that you are building for the `esp32s3` chip.

```powershell
idf.py set-target esp32s3
```

## 2. Configure the Board
You must select the specific **BOX-3** board configuration from the menu.

1.  Run the configuration menu:
    ```powershell
    idf.py menuconfig
    ```

2.  Navigate to:
    `Xiaozhi Assistant` -> `Board Type`

3.  Select:
    `Espressif ESP-BOX-3` (Press Space to select, Enter to confirm).

4.  (Optional) While in `menuconfig`, you can also configure your WiFi credentials if you want them hardcoded (though using the mobile app or "Hotspot" generic provisioning is recommended).

5.  **Save & Exit**: Press `S` to Save, then `Esc` multiple times to Exit.

## 3. Build the Project
Compile the source code. This may take a few minutes the first time.

```powershell
idf.py build
```

## 4. Flash & Monitor
Connect your ESP32-S3-BOX-3 via USB-C. Ensure the correct COM port is detected.

```powershell
# Replace COMx with your actual port (e.g., COM3, COM5)
# If you don't know the port, you can usually omit -p COMx and idf.py will try to auto-detect.
idf.py -p COMx flash monitor
```

*   `flash`: Writes the firmware to the device.
*   `monitor`: Opens the serial console so you can see the logs.

## 5. Troubleshooting
*   **Missing Dependencies**: If the build fails, try running `idf.py fullclean` and then `idf.py build` again.
*   **Port Not Found**: Check your USB cable and drivers (CP210x or built-in USB-JTAG).
*   **Boot Loop**: If the device keeps restarting, check the serial monitor logs. Power issues (insufficient USB power) are a common cause with the screen and speaker active.
