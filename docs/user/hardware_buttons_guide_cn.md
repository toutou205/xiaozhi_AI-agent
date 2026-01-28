# ESP32-S3-BOX-3 按键使用指南

本文档详细说明了 ESP32-S3-BOX-3 开发板顶部的按键功能、对应的引脚关系以及如何在代码中进行配置。

## 概览

ESP32-S3-BOX-3 顶部主要包含三个按键：
1.  **Mute (静音键)**: 红色圆圈触摸屏上方右侧。
2.  **Boot (多功能/配置键)**: 红色圆圈触摸屏上方左侧。
3.  **Reset (复位键)**: 通常位于侧面或作为隐藏按键，用于硬件复位。

---

## 1. 静音按钮 (Mute Button)

静音按钮是一个特殊的硬件隐私开关，具有物理切断麦克风电路的功能。

*   **丝印标识**: Mute (或图标)
*   **对应引脚**: **GPIO 1**
*   **硬件功能**:
    *   该按钮连接到板载的逻辑门电路。
    *   按下时，会**物理切断**麦克风的信号，确保绝对的隐私安全。
    *   GPIO 1 作为一个**状态输入**引脚，告知 ESP32 当前的静音状态（高电平或低电平）。
*   **软件配置**:
    *   虽然可以通过软件读取 GPIO 1 的状态来更新 UI（例如显示静音图标），但**无法通过软件禁用**其物理静音功能。这是硬件层面的强制行为。
    *   **代码位置**: `examples/factory_demo/managed_components/espressif__esp-box-3/include/bsp/esp-box-3.h`
        ```c
        #define BSP_BUTTON_MUTE_IO    (GPIO_NUM_1)
        ```
    *   **示例用法**:
        在 `factory_demo` 中，它被配置为打印日志并触发音频静音逻辑：
        ```c
        // 注册回调函数
        bsp_btn_register_callback(BSP_BUTTON_MUTE, BUTTON_PRESS_DOWN, mute_btn_handler, (void *)BUTTON_PRESS_DOWN);
        ```

---

## 2. Boot 按钮 (Boot/Config Button)

Boot 按钮是 ESP32 系列芯片的标准配置按钮，用于控制启动模式，同时也可作为通用的用户按键使用。

*   **丝印标识**: Boot
*   **对应引脚**: **GPIO 0**
*   **硬件功能**:
    *   **下载模式 (Download Mode)**: 在按住 Boot 键的同时按下 Reset 键，ESP32-S3 将进入串口下载模式。这是烧录固件时的常用操作。
*   **软件功能 (作为用户按键)**:
    *   设备正常启动后，GPIO 0 可以作为普通的输入引脚使用 (Active Low)。
    *   可以使用 BSP (Board Support Package) 将其配置为任意功能的触发源。
*   **代码位置**: `examples/factory_demo/managed_components/espressif__esp-box-3/include/bsp/esp-box-3.h`
        ```c
        #define BSP_BUTTON_CONFIG_IO  (GPIO_NUM_0)
        ```
    *   注意：在代码中通常定义为 `BSP_BUTTON_CONFIG`。
*   **示例用法**:
    在 `factory_demo` 中，它被配置为**长按重置 Wi-Fi 配置**：
    ```c
    // app_rmaker.c
    void app_rmaker_start(void)
    {
        // 注册长按事件的回调函数，用于重置 Wi-Fi
        bsp_btn_register_callback(BSP_BUTTON_CONFIG, BUTTON_LONG_PRESS_START, wifi_credential_reset, NULL);
        // ...
    }
    ```

## 3. 按键配置 API 参考

项目使用 `esp-iot-solution` 中的 `iot_button` 组件来管理按键事件。您可以在自己的应用中这样配置：

```c
#include "bsp/esp-bsp.h"

// 定义回调函数
static void my_btn_handler(void *handle, void *arg)
{
    // 处理按键逻辑
    printf("Button Pressed!\n");
}

// 在初始化阶段注册
void app_main(void)
{
    // ... 其他初始化 ...
    bsp_board_init(); // 初始化板级支持包，包括按键

    // 为 Boot 键 (BSP_BUTTON_CONFIG) 注册按下事件
    bsp_btn_register_callback(BSP_BUTTON_CONFIG, BUTTON_PRESS_DOWN, my_btn_handler, NULL);
}
```

## 总结

| 按键名称 | 引脚 (GPIO) | 硬件特性 | 常见用途 | 可配置性 |
| :--- | :--- | :--- | :--- | :--- |
| **Mute** | 1 | 物理麦克风切断 | 隐私静音、状态指示 | 软件逻辑可配，硬件静音不可禁 |
| **Boot** | 0 | 启动模式选择 | 固件烧录、功能触发 (如重置) | 完全可配置 |
