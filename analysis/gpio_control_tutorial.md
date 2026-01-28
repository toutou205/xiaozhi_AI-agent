# Tutorial: How to Add Custom GPIO Control (MCP Tool)

This guide shows you how to add a voice command to control a GPIO pin (e.g., "Turn on the light") on your **ESP32-S3-BOX-3**.

**Goal**: Control a LED connected to **GPIO 8**.

---

## Step 1: Define the Pin
Open **`main/boards/esp-box-3/config.h`** and add your pin definition.

```c
// ... existing code ...
#define AUDIO_I2S_GPIO_DIN      GPIO_NUM_16

// Add this line:
#define MY_LIGHT_PIN           GPIO_NUM_8 
```

---

## Step 2: Initialize the Pin
Open **`main/boards/esp-box-3/esp_box3_board.cc`**.
Find the `EspBox3Board` class. You need to configure the pin as an output in the constructor.

```cpp
// In EspBox3Board class
class EspBox3Board : public WifiBoard {
    // ...
    // Add a helper function to init the pin
    void InitializeMyLight() {
        gpio_reset_pin(MY_LIGHT_PIN);
        gpio_set_direction(MY_LIGHT_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(MY_LIGHT_PIN, 0); // Default off
    }

public:
    EspBox3Board() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeSpi();
        InitializeIli9341Display();
        InitializeButtons();
        GetBacklight()->RestoreBrightness();
        
        // Add this line to call your init function:
        InitializeMyLight();
        
        // Ensure InitializeTools() is called (see Step 3)
        InitializeTools(); 
    }
    // ...
```

---

## Step 3: Add the "Tool" (The Functions)
Still in **`main/boards/esp-box-3/esp_box3_board.cc`**, add the `InitializeTools` method to your class (if it doesn't exist) or edit it.

You need to include `mcp_server.h` at the top of the file first:
```cpp
#include "mcp_server.h" 
#include <driver/gpio.h> // Ensure this is included for gpio functions
```

Then add the tool definition inside `EspBox3Board` class:

```cpp
    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();

        // Tool 1: Turn Light On/Off
        mcp_server.AddTool("self.light.switch",    // Tool Name (Unique ID)
            "Turn the light on or off. Use this when user says 'Turn on/off light'.", // Description for AI
            PropertyList({
                Property("enable", kPropertyTypeBoolean) // Argument: true/false
            }), 
            [this](const PropertyList& properties) -> ReturnValue {
                bool enable = properties["enable"].value<bool>();
                
                // Hardware Action
                gpio_set_level(MY_LIGHT_PIN, enable ? 1 : 0);
                
                // Feedback to User
                GetDisplay()->ShowNotification(enable ? "Light ON" : "Light OFF");
                
                return true; // Success
            });
    }
```

---

## Step 4: Compile and Test
1.  Run `idf.py build`.
2.  Flash `idf.py -p COMx flash monitor`.
3.  Say: **"Xiaozhi, turn on the light"** (or "打开灯" if in Chinese).
4.  The server will recognize the intent and call `self.light.switch(true)`.

## Summary of Logic Flow
`Voice Command` -> `MCP Server (Cloud)` -> `Device (Main)` -> `self.light.switch` Tool -> `GPIO 8 High`
