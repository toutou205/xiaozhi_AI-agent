# ESP32-S3-BOX-3 硬件资源全景禁区与约束表

> **版本 info**: Validated for ESP32-S3-BOX-3 (V1.0/V1.1)
> **生成时间**: 2026-02-03 18:20:00
> **参考文献**: 
> - `SCH_ESP32-S3-BOX-3_V1.0`
> - `ESP32-S3 Datasheet`
> - `ILI9341 / ES8311 / ES7210 / AHT30 Datasheets`

这份表格合并了物理连接、电气特性、通信协议约束及启动状态要求，旨在帮助开发者快速判断 GPIO 可用性并避免硬件冲突。

### 状态分类说明 (Status Legend)
- **FATAL / SYSTEM**: **绝对禁区**。涉及系统启动 (Strapping)、Flash/PSRAM 或 JTAG 调试。触碰极易导致无法启动或系统崩溃。
- **EXCLUSIVE**: **独占资源**。已被板载核心外设（屏幕、音频 PA、Codec）占用。除非重写底层驱动或移除硬件，否则不可复用。
- **SHARED**: **共享总线**。如 I2C 总线。允许挂载新设备，但必须严格遵守电气特性（频率、电压）和逻辑地址约束。
- **FREE**: **完全可用**。未连接主要外设，或专为扩展预留（PMOD/Sensor Header）。

---

### 全景禁区表 (Panorama Constraints Table)

| GPIO | 默认功能 | 状态分类 | 占用详情 (Who & What) | 核心约束 (Hard Constraints) | 备注 (Default Level & Warnings) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | **BOOT** | **FATAL** | **System**: Boot Button | **Strapping Pin**:<br>上电采样阶段必须为 **HIGH** (进入运行模式)。<br>拉低 (LOW) 进入下载模式。 | **Default**: Pull-up (R16 1K).<br>严禁外部强下拉，否则设备无法启动。 |
| **1** | MUTE_STATUS | **EXCLUSIVE** | **System**: Mute Circuit | 硬件静音状态反馈。 | 连接至静音按键电路，不建议复用。 |
| **2** | AUDIO_MCLK | **EXCLUSIVE** | **Audio**: ES7210 / ES8311 | 主时钟信号，音频系统心脏。 | 复用将导致音频系统失效。 |
| **3** | TOUCH_INT | **SYSTEM** | **Touch**: GT911 Interrupt<br>**JTAG**: TCK | **Strapping Pin (JTAG)**:<br>控制 JTAG 接口使能。<br>Dock PMOD 1 Pin 3 引出。 | **Default**: Floating.<br>注意由触摸屏触发的中断电平，可能影响 JTAG 调试连接。 |
| **4** | LCD_DC | **EXCLUSIVE** | **Display**: ILI9341 D/C<br>**Dock**: PMOD 1 Pin 4 | **SPI Speed**:<br>Write Max 10MHz | 若使用屏幕，此脚不可动。<br>若做纯无头设备(Headless)，可复用但需移除屏幕驱动。 |
| **5** | LCD_CS | **EXCLUSIVE** | **Display**: ILI9341 CS<br>**Dock**: PMOD 1 Pin 5 | 同上 | 同上 |
| **6** | LCD_CLK | **EXCLUSIVE** | **Display**: ILI9341 SCLK<br>**Dock**: PMOD 1 Pin 8 | 同上 | 同上 |
| **7** | LCD_MOSI | **EXCLUSIVE** | **Display**: ILI9341 MOSI<br>**Dock**: PMOD 1 Pin 7 | 同上 | 同上 |
| **8** | - | **FREE** | **Dock**: PMOD 1 Pin 10 | - | **Default**: Weak Pull-up.<br>完全空闲，安全可用。 |
| **9** | - | **FREE** | **Sensor**: IR Receiver<br>**Dock**: PMOD 2 Pin 7 | - | Sensor 板上接红外接收头。 |
| **10** | - | **FREE** | **Sensor**: Battery ADC<br>**Dock**: PMOD 2 Pin 10 | **Analog Capable** | Sensor 板上接分压电阻测电压。 |
| **11** | - | **FREE** | **Sensor**: SD_CMD (MOSI)<br>**Dock**: PMOD 2 Pin 8 | **SDIO / SPI** | Sensor 板用于 TF 卡。 |
| **12** | - | **FREE** | **Sensor**: SD_CLK<br>**Dock**: PMOD 2 Pin 9 | **SDIO / SPI** | 同上 |
| **13** | - | **FREE** | **Sensor**: SD_D0 (MISO)<br>**Dock**: PMOD 2 Pin 2 | **SDIO / SPI** | 同上 |
| **14** | - | **FREE** | **Sensor**: SD_DET<br>**Dock**: PMOD 2 Pin 3 | - | Sensor 板用于 TF 卡检测。 |
| **15** | I2S_Dsdin | **EXCLUSIVE** | **Audio**: ES8311 DAC Data | I2S Protocol | 音频播放数据线。 |
| **16** | I2S_Sdout | **EXCLUSIVE** | **Audio**: ES7210 ADC Data | I2S Protocol | 麦克风录音数据线。 |
| **17** | I2S_SCLK | **EXCLUSIVE** | **Audio**: Bit Clock | I2S Protocol | 音频位时钟。 |
| **18** | - | **FREE** | - | - | **Default**: Weak Pull-up.<br>完全空闲，安全可用。 |
| **19** | USB_D- | **SYSTEM** | **USB**: Native USB-Serial/JTAG | USB 差分信号 (90Ω 阻抗) | 仅在完全放弃 USB 功能时可作 GPIO。 |
| **20** | USB_D+ | **SYSTEM** | **USB**: Native USB-Serial/JTAG | 同上 | 同上 |
| **21** | - | **FREE** | **Sensor**: Pin Header | - | Sensor 板上有排针引出，未连接特定外设。 |
| **22-25** | - | **NC** | Not Connected | - | 未引出。 |
| **26** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **27** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **28** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **29** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **30** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **31** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **32** | FLASH_SPI | **SYSTEM** | **Internal**: SPI Flash | **Absolute Forbidden** | 严禁使用。 |
| **33** | PSRAM_SPI | **SYSTEM** | **Internal**: Octal PSRAM | **Absolute Forbidden** | 模组 (ESP32-S3R16V) 内部专用。 |
| **34** | PSRAM_SPI | **SYSTEM** | **Internal**: Octal PSRAM | **Absolute Forbidden** | 同上。 |
| **35** | PSRAM_SPI | **SYSTEM** | **Internal**: Octal PSRAM | **Absolute Forbidden** | 同上。 |
| **36** | PSRAM_SPI | **SYSTEM** | **Internal**: Octal PSRAM | **Absolute Forbidden** | 同上。 |
| **37** | PSRAM_SPI | **SYSTEM** | **Internal**: Octal PSRAM | **Absolute Forbidden** | 同上。 |
| **38** | - | **SHARED** | **Sensor**: Radar Output (PH)<br>**Dock**: Pin Header | - | **Default**: Floating.<br>Sensor 板上连接雷达模块输出。 |
| **39** | LED_GREEN | **SHARED** | **MB**: Green LED<br>**Sensor**: IR Transmitter | Current Limit | 复用时注意 LED 会随平翻转。 |
| **40** | **I2C_SCL** | **SHARED** | **Bus Owner**: ESP32<br>**Slaves**: Codec, ADC, Touch, Sensor | **Speed**: Max **400kHz** (受限 ES7210)<br>**Pull-up**: 2.2K | **Address Blacklist (Hex)**:<br>`0x18` (ES8311)<br>`0x40` (ES7210)<br>`0x38` (AHT30)<br>`0x5D`, `0x14` (GT911 Touch)<br>`0x68` (IMU) |
| **41** | **I2C_SDA** | **SHARED** | 同上 | 同上 | 同上 |
| **42** | I2S_LRCK | **EXCLUSIVE** | **Audio**: WS (Word Select) | I2S Protocol | 音频帧同步信号。 |
| **43** | U0TXD | **SHARED** | **Debug**: USB-UART Bridge | - | 默认 Log 输出口。复用将丢失 Log。 |
| **44** | U0RXD | **SHARED** | **Debug**: USB-UART Bridge | - | 默认 Log 输入口。 |
| **45** | VDD_SPI | **FATAL** | **System**: Flash Config | **Strapping Pin**:<br>决定 Flash 电压 (1.8V vs 3.3V)。 | **Default**: Internal Pull-down (Usually).<br>外部严禁拉高! 模组内设为 1.8V，拉高可能烧毁 Flash/PSRAM。 |
| **46** | PA_CTRL | **FATAL** | **System**: Boot Log / PA Enable | **Strapping Pin**:<br>上电采样: ROM Log控制。<br>运行期: 音频功放使能。 | **Default**: Pull-down (R76 100K).<br>上电期间必须保持低电平。若外接模块输出高，将导致乱码或启动异常。 |
| **47** | LCD_BL | **EXCLUSIVE** | **Display**: Backlight Control | Logic High = On | 背光控制。 |
| **48** | LCD_RST | **EXCLUSIVE** | **Display**: Reset | Active Low | **Default**: C32 (100nF) to GND, No Pull-up.<br>悬空/电容下拉。 |

---

### 补充约束说明

#### 1. I2C 总线挂载指南 (GPIO 40/41)
该总线是板级最繁忙的通道。若需在 PMOD 或 Sensor Header 上挂载新的 I2C 设备：
*   **必须检查地址冲突**：对照上方表格的 **Address Blacklist**。
*   **必须限制速率**：初始化 `i2c_config_t` 时，`master.clk_speed` **不可超过 400000 (400kHz)**。
*   **电源**: 确认外挂设备电压域为 3.3V。

#### 2. 电源负载警告 (Power Budget)
*   **3.3V 总线极限**: 板载 LDO (SY8088) 提供最大 **1A** 电流。
*   **现有负载**: ESP32-S3 RF (350mA) + LCD (50mA) + Audio (50mA) $\approx$ 450mA。
*   **剩余可用**: **< 500mA**。
*   **警告**: 若外接激光雷达、大扭矩舵机等高功率设备，请务必使用 **Dock 板上的 USB VBUS (5V)** 并自行降压，或使用独立供电，严禁直接从 3.3V 排针取电。

#### 3. 屏幕与 SPI 限制
*   ILI9341 屏幕接在 SPI2 (FSPI)。
*   虽然 ESP32-S3 支持 80MHz SPI，但 ILI9341 的 Write Cycle 限制了最高稳定频率约 **10MHz - 40MHz** (取决于线束质量和驱动优化，Datasheet 标称读写周期限制较严)。实际应用常用 20-40MHz，但需注意信号完整性。表格中保守标注 10MHz 是基于 Datasheet 的严格读写周期计算。
