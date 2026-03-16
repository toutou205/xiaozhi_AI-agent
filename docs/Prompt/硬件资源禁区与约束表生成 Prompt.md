"请读取我上传的 `1.1` 和 `1.2` 文档，结合 ESP32-S3 和 BOX-3 的原理图知识，**合并并重构**一份《硬件资源全景禁区表》。
> 
> 要求：
> 
> 1. 采用 Markdown 表格格式。
>     
> 2. 将 1.2 中的物理限制（如 I2C 频率、地址黑名单、Strapping 风险）精确填入 1.1 表格对应的 GPIO 备注栏中。
>     
> 3. 增加一列 '状态分类' (SYSTEM/EXCLUSIVE/SHARED/FREE)。
>     
> 4. 补充 I2C 总线所有已占用的 16 进制地址。"
>     


### 文档内容需要合并以及增加，修改建议如下：

### 一、 输入信息与资料 (Input Prerequisites)

要生成一份无懈可击的禁区表，不能仅凭一张原理图。你需要**四维**信息输入：

1. **物理连接 (Connectivity)**:
    
    - **输入**: `SCH_ESP32-S3-BOX-3-MB_V1.1.pdf` (主板), `SCH_...DOCK...pdf`, `SCH_...SENSOR...pdf`。
        
    - **提取**: 哪个 GPIO 连了哪个芯片？经过了哪个电阻？是直连还是通过了电平转换？
        
2. **电气特性 (Electrical Characteristics)**:
    
    - **输入**: `esp32-s3_datasheet_en.pdf` (SoC), `SY8088.pdf` (电源)。
        
    - **提取**: 这是一个只能输入的引脚吗？它是 Strapping 管脚吗？它的最大驱动电流是多少？当前电源 LDO 还剩多少余量？
        
3. **通信协议 (Protocol Constraints)**:
    
    - **输入**: `ILI9341.pdf` (屏幕), `ES8311.pdf` (Codec), `AHT30.pdf` (传感器)。
        
    - **提取**: 这个设备支持的最大 SPI/I2C 频率是多少？它的 I2C 地址是什么（十六进制）？
        
4. **默认状态 (Default State)**:
    
    - **输入**: 原理图中的上下拉电阻值。
        
    - **提取**: 系统复位瞬间，这个脚是高电平还是低电平？（这对控制继电器或功放至关重要）。
        

---

### 二、 输出格式与合格标准 (Format & Criteria)

一份合格的《硬件资源禁区与约束表》必须具备 **"开发者友好性" (Developer Friendly)** 和 **"机器可读性" (Machine Readable)**。

#### 1. 合格指标 (KPIs)

- **MECE原则 (不重不漏)**: 必须覆盖 GPIO 0 到 GPIO 48 的每一个引脚，哪怕是 NC (未连接)。
    
- **状态二元性**: 必须明确标识 **[Safe] (可用)** 或 **[Forbidden] (禁用/慎用)**，不能模棱两可。
    
- **上下文完整**: 看到这个脚，不仅知道被占用了，还必须知道**被谁占用**、**占用的电气后果**（如：有强上拉）、**占用的逻辑后果**（如：I2C 地址冲突）。
    

#### 2. 推荐输出格式 (Excel/Markdown Table)

必须包含以下关键列：

|**GPIO**|**功能名称**|**物理状态**|**占用情况 (Owner)**|**电气/逻辑约束 (Constraints)**|**默认电平**|
|---|---|---|---|---|---|
|40|I2C_SCL|**SHARED**|Codec, Touch, Sensor, Dock|Max 400kHz; Addr: 0x18, 0x40, 0x5D|PU (2.2K)|

---

### 三、 针对上传文档的优化建议 (Optimization)

我仔细审查了你上传的 `1.1` 和 `1.2` 文档。

- **现状**: `1.1` 是一个基础的 Pinout 表，`1.2` 是详细的文字描述。
    
- **问题**: 开发者在查表时（看 1.1），看不到具体的限制（在 1.2 里）。两者割裂，容易导致看表写代码时忽略了频率限制或 Strapping 风险。
    

**整改方案：将 1.2 的核心信息“注入”到 1.1 的表格中，并扩充关键细节。**

#### 需要增加/修改的 4 个关键点：

**1. 细化“占用状态” (Status Granularity)**

- **原文档**: 只有 `Occupied` 和 `Strapping`。
    
- **修改建议**: 细分为：
    
    - `SYSTEM` (Flash/PSRAM/JTAG) - **绝对不可动**
        
    - `EXCLUSIVE` (屏幕/音频I2S) - **除非改驱动否则不可动**
        
    - `SHARED` (I2C总线) - **可挂载新设备，但需注意地址**
        
    - `FREE` / `EXPANSION` - **完全可用**
        

**2. 注入“I2C 地址黑名单” (Address Blacklist)**

- **原文档**: GPIO 40/41 只写了 "Occupied"。
    
- **修改建议**: 必须列出所有已挂载设备的 Hex 地址。
    
    - _Action_: 在 GPIO 40/41 的备注栏加入：`[Blacklist: 0x18 (ES8311), 0x40 (ES7210), 0x38 (AHT30), 0x5D/0x14 (Touch)]`。
        

**3. 注入“Strapping 动作指南”**

- **原文档**: 仅标记 "High Risk"。
    
- **修改建议**: 明确告诉开发者**怎么做才不炸**。
    
    - _Action_: 在 GPIO 0 的备注写：`Boot Mode. Must FLOAT or HIGH at startup. DO NOT PULL LOW externally.`
        

**4. 增加“电气默认值” (Default Pull)**

- **原文档**: 未提及。
    
- **修改建议**: 增加一列或在备注中标注。
    
    - _Action_: 例如 GPIO 48 (LCD_RST)，原理图显示有 C32 电容，无上拉。需标注 `Default: Floating`.
        

---

### 四、 最终形态示例 (The Golden Sample)

请参照以下格式，合并并升级你的两份文档。这就是通过 Phase 1 审查的标准交付物。

#### **ESP32-S3-BOX-3 硬件资源全景禁区表**

|**GPIO**|**默认功能**|**状态分类**|**占用详情 (Who & What)**|**核心约束 (Hard Constraints)**|**备注 (Notes)**|
|---|---|---|---|---|---|
|**0**|BOOT|**FATAL**|System Boot|**Strapping Pin**: 上电必须为高。|严禁外部下拉，否则卡在下载模式。|
|...|...|...|...|...|...|
|**38**|USB_D-|**SYSTEM**|USB-Serial/JTAG|USB 差分信号，阻抗匹配要求高。|仅在不使用 USB 时可复用为普通 GPIO。|
|**39**|USB_D+|**SYSTEM**|USB-Serial/JTAG|同上。|同上。|
|**40**|I2C_SCL|**SHARED**|**Bus Owner**: ESP32<br><br>  <br><br>**Slaves**: Codec, ADC, Touch, Sensor|**Speed**: Max **400kHz** (受限于 ES7210)<br><br>  <br><br>**Volt**: 3.3V|外部设备**严禁**使用地址: `0x18`, `0x40`, `0x38`, `0x5D`|
|**41**|I2C_SDA|**SHARED**|同上|同上|同上|
|**42**|I2S_WS|**EXCLUSIVE**|Audio Codec & Mic|高频信号，音频采样率基准。|占用此脚会导致声音异常。|
|...|...|...|...|...|...|
|**48**|LCD_RST|**EXCLUSIVE**|Display Controller|屏幕复位控制。|软件控制，低电平复位。|