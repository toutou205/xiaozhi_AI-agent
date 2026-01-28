# Xiaozhi ESP32 Project Interpretation & Adaptation Guide

This guide interprets the `xiaozhi-esp32` project structure and provides specific instructions on how to modify the display interface for the **ESP32-S3-BOX-3**.

## 1. Project Overview
The project is based on **ESP-IDF** and is structured to support multiple boards. It uses a component-based architecture where the main logic resides in the `main` directory.

### Key Directories
- **`main/boards/`**: Contains board-specific configurations (pin definitions, initialization).
- **`main/display/`**: Contains display drivers and UI logic.
- **`main/application.cc`**: The main application logic binding everything together.

## 2. ESP32-S3-BOX-3 Support
Good news! The project already contains a board definition for **ESP-S3-BOX-3** in:
`main/boards/esp-box-3/`

This means "adaptation" is mostly about configuration and customization rather than porting from scratch.

### Board Initialization
The file `main/boards/esp-box-3/esp_box3_board.cc` handles:
- **I2C/SPI Initialization**: Sets up communication buses.
- **Display Driver**: Uses the **ILI9341** driver.
- **Touch/Audio**: Initializes audio codecs and buttons.

## 3. Display Interface Architecture
The project supports two primary display modes. You need to know which one needs modification:

### Mode A: Emote Style (Robot Face)
This is the default "cute robot" interface with animated eyes.
- **Key File**: [`main/display/emote_display.cc`](file:///d:/Projecet/xiaozhi-esp32-2.1.0/main/display/emote_display.cc)
- **Implementation**: Uses a lightweight custom graphics engine (`gfx`) instead of LVGL to save resources for smooth animations.
- **Modification Points**:
  - **Eyes Animation**: `EmoteEngine::SetEyes`
  - **Status Icons**: `EmoteEngine::SetIcon`
  - **Layout**: `SetupUI` function defines where elements (Clock, Toast, Eyes) are placed.

### Mode B: LVGL Interface (Standard UI)
This is a more traditional UI using the LVGL library.
- **Key File**: [`main/display/lvgl_display/lvgl_display.cc`](file:///d:/Projecet/xiaozhi-esp32-2.1.0/main/display/lvgl_display/lvgl_display.cc)
- **Implementation**: Uses standard LVGL widgets (Labels, Images, etc.).
- **Modification Points**:
  - **Status Bar**: `UpdateStatusBar` function.
  - **Notifications**: `ShowNotification`.
  - **Widgets**: Look for `lv_obj_create`, `lv_label_create` to add new UI elements.

## 4. How to Modify the Interface

### Scenario 1: I want to move the "Eyes" or change the Layout
1. Open [`main/display/emote_display.cc`](file:///d:/Projecet/xiaozhi-esp32-2.1.0/main/display/emote_display.cc).
2. Locate the `SetupUI` function (around line 227).
3. You will see lines like `gfx_obj_align(g_obj_anim_eye, GFX_ALIGN_LEFT_MID, 10, 30);`.
4. **Action**: Change the alignment or coordinates `(x, y)` to move elements.

### Scenario 2: I want to use LVGL instead of the "Face"
1. Open [`main/boards/esp-box-3/esp_box3_board.cc`](file:///d:/Projecet/xiaozhi-esp32-2.1.0/main/boards/esp-box-3/esp_box3_board.cc).
2. Look for `InitializeIli9341Display`.
3. Check the condition `#if CONFIG_USE_EMOTE_MESSAGE_STYLE`.
4. **Action**: You can force the use of `SpiLcdDisplay` (which wraps LVGL in some configurations) or modify `menuconfig` to disable `CONFIG_USE_EMOTE_MESSAGE_STYLE`.

### Scenario 3: I want to change the Pin Definitions for a custom S3 Board
1. Open [`main/boards/esp-box-3/esp_box3_board.cc`](file:///d:/Projecet/xiaozhi-esp32-2.1.0/main/boards/esp-box-3/esp_box3_board.cc).
2. Locate `InitializeSpi()` and `InitializeIli9341Display()`.
3. **Action**: Modify `GPIO_NUM_X` values to match your hardware connections.

## 5. Summary
- **Board Config**: `main/boards/esp-box-3/esp_box3_board.cc`
- **Face UI**: `main/display/emote_display.cc`
- **General UI**: `main/display/lvgl_display/`
