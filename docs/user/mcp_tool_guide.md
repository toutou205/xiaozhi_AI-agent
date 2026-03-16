# MCP 工具指南与高级功能手册

本必须介绍 XiaoZhi ESP32 项目中 Model Context Protocol (MCP) 服务端提供的工具类型、使用方法以及隐藏的高级功能。

## 1. 工具类型：AddTool vs AddUserOnlyTool

在 `mcp_server.cc` 中，我们定义了两种类型的工具注册方式，它们决定了工具对智能体（Agent）的可见性。

| 特性 | `AddTool` (普通工具) | `AddUserOnlyTool` (用户专用工具) |
| :--- | :--- | :--- |
| **可见性** | **默认可见**。智能体连接后，只要请求工具列表，这些工具就会直接出现在 Prompt 中。 | **默认隐藏**。智能体必须在请求工具列表时显式携带参数 `{"withUserTools": true}` 才能看到它们。 |
| **设计初衷** | **核心高频功能**。如“查看设备状态”、“调节音量”。这些是智能体日常交互必须知道的能力。 | **危险/低频/敏感功能**。如“重启设备”、“固件升级”、“查看底层系统信息”。避免让智能体在日常对话中意外调用（或者因为 Context Window 有限而被挤占）。 |
| **使用场景** | 让 AI “知道”它能做这件事，并且希望它经常使用。例如：`self.debug.get_cpu_stats` 被归类为此类，以便随时调用。 | 只有在用户明确通过某种“高级/开发者模式”与 AI 交互，或者 AI 的 System Prompt 经过特殊设计知道去索取这些工具时才使用。 |

### ⚠️ 使用注意事项
1.  **Prompt 污染**：不要把所有工具都设为 `AddTool`。默认可见的工具越多，也会占用越多的 Context Token，导致响应变慢或 AI 混淆。
2.  **安全性**：涉及**重启** (`self.reboot`)、**修改系统设置** (`self.upgrade_firmware`) 的操作，务必保持为 `AddUserOnlyTool`，防止 AI 自作主张。

---

## 2. 功能清单

### A. 常用工具 (Common Tools)
这些工具默认对所有连接的 Agent 可见：

*   **`self.get_device_status`**
    *   **描述**: 获取设备实时状态（音量、屏幕、网络等）。
    *   **用途**: AI 决策前的“感知”步骤。
*   **`self.audio_speaker.set_volume`**
    *   **描述**: 调节扬声器音量 (0-100)。
*   **`self.debug.get_cpu_stats`** (新增)
    *   **描述**: 分析系统各任务的 CPU 占用率并给出优化建议。
    *   **用途**: 性能诊断与健康检查。

### B. 隐藏/受限工具 (User Only / Conditional)
这些工具默认不显示，除非客户端特殊请求，或者满足特定硬件条件：

#### 用户专用 (UserOnly)
*   **`self.get_system_info`**:
    *   获取底层硬件详情（Flash 大小、Heap 剩余、MAC 地址、Chip Model、UUID）。
*   **`self.reboot`**:
    *   执行系统软重启。
*   **`self.upgrade_firmware`**:
    *   传入 URL 下载并烧录新固件（OTA）。
*   **`self.assets.set_download_url`**:
    *   修改资源分区的下载源地址。

#### 条件启用 (Conditional)
以下工具仅在相应的驱动或库启用时才会被注册：

*   **`self.screen.set_brightness`**
    *   *条件*: 屏幕背光驱动有效 (`Backlight`存在)。
*   **`self.screen.set_theme`**
    *   *条件*: 启用 LVGL (`HAVE_LVGL`) 且配置了主题管理器。
    *   *功能*: 切换深色/浅色模式。
*   **`self.camera.take_photo`**
    *   *条件*: 检测到摄像头硬件。
    *   *功能*: 拍照并让大模型进行图像理解（Vision）。
*   **`self.screen.snapshot`** (UserOnly)
    *   *条件*: `CONFIG_LV_USE_SNAPSHOT` 宏开启。
    *   *功能*: 截取当前屏幕画面并上传到指定 URL。这是极其强大的 UI 调试工具。
*   **`self.screen.get_info`** (UserOnly)
    *   *条件*: 启用 LVGL。
    *   *功能*: 获取屏幕分辨率和色彩模式。
*   **`self.screen.preview_image`** (UserOnly)
    *   *条件*: 启用 LVGL。
    *   *功能*: 从 URL 下载图片并在屏幕上预览。

## 3. 开发建议
如果您需要开发新的调试工具：
- 如果是**只读**且**无副作用**的（如查询状态），推荐使用 `AddTool`。
- 如果是**有副作用**（如重启、重置）或**输出极长**（如全系统日志转存），推荐使用 `AddUserOnlyTool`。
