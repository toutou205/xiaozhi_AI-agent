# ESP32-S3-BOX-3 存储与内存深度分析

本文档详细分析了工程在 ESP32-S3-BOX-3 硬件上的存储（Flash/NVS）与内存（PSRAM）使用情况，旨在帮助开发者理解固件烧录、分区布局、数据存储及 OTA 机制。

## 1. 硬件配置概览 (Hardware Overview)

针对 **ESP32-S3-BOX-3** 的硬件特性：
*   **SoC**: ESP32-S3
*   **Flash**: 16 MB (Quad SPI)
*   **PSRAM**: 16 MB (Octal SPI)
*   **SRAM**: 512 KB (Internal)

> **注意**: 该项目通过 `sdkconfig.defaults` 和 `boards/esp-box-3/config.json` 自动适配此硬件配置。

---

## 2. Flash 分区与烧录 (Flash Partitions)

### 2.1 分区表布局
项目使用 **16MB 定制分区表**，文件位于 `partitions/v2/16m.csv`。具体布局如下：

| Name      | Type | SubType | Offset   | Size      | Description |
| :---      | :--- | :---    | :---     | :---      | :--- |
| **bootloader** | - | - | 0x0 | - | (隐式) 启动引导程序 |
| **partition_table** | - | - | 0x8000 | - | (隐式) 分区表本身 |
| **nvs**       | data | nvs     | 0x9000   | 0x4000 (16KB) | 非易失性存储 (WiFi密码等) |
| **otadata**   | data | ota     | 0xd000   | 0x2000 (8KB) | OTA 引导数据 (记录当前活动分区) |
| **phy_init**  | data | phy     | 0xf000   | 0x1000 (4KB) | RF 射频初始化数据 |
| **ota_0**     | app  | ota_0   | 0x20000  | 0x3f0000 (~4MB) | **固件插槽 A** (默认启动) |
| **ota_1**     | app  | ota_1   | (Auto)   | 0x3f0000 (~4MB) | **固件插槽 B** (OTA 升级用) |
| **assets**    | data | spiffs* | 0x800000 | 8M        | 静态资源 (音频、图片、模型) |

> **关键点**:
> *   **assets 分区**: 虽然分区表标记为 `spiffs`，但实际代码 (`assets.cc`) 使用 **自定义二进制格式 (Mapped Assets)**。
> *   **固件大小限制**: app 分区 (ota_0/ota_1) 限制为约 4MB。
> *   **资源分区大**: 预留了 8MB 给 assets，用于存放大量多媒体资源。

### 2.2 烧录位置 (Flashing)
当使用 `idf.py build` 编译后，生成的二进制文件会烧录到以下偏移地址：
*   `bootloader.bin` -> **0x0**
*   `partition-table.bin` -> **0x8000**
*   `ota_data_initial.bin` -> **0xd000**
*   `xiaozhi.bin` (Application) -> **0x20000** (ota_0)
*   `assets.bin` -> **0x800000**

---

## 3. 应用数据存储 (NVS & WiFi)

项目使用 **NVS (Non-Volatile Storage)** 在 Flash 上保存关键配置信息。

### 3.1 WiFi 凭据 (NVS Namespace: "wifi")
WiFi 账号密码不由文件系统管理，而是直接写入 NVS 的 `"wifi"` 命名空间。
逻辑位于 `managed_components/esp-wifi-connect/ssid_manager.cc`。

*   **存储格式**: 支持存储多组 WiFi (最多 10 组)。
*   **键名 (Keys)**:
    *   第 1 组: key=`"ssid"`, key=`"password"`
    *   第 2 组: key=`"ssid1"`, key=`"password1"`
    *   ...
    *   第 10 组: key=`"ssid9"`, key=`"password9"`

### 3.2 系统设置 (NVS Namespaces)
*   **Assets 更新**: 命名空间 `"assets"`
    *   key=`"download_url"`: 存储资源包的 OTA 下载链接。
*   **其他**: `Settings` 类 (`main/settings.cc`) 支持任意命名空间的读写，项目可能还在其他模块使用了 NVS (如音量记忆等)。

---

## 4. 资源存储 (Assets Storage)

Box-3 的 **8MB assets 分区** 承载了大部分交互内容。

*   **加载机制**: `main/assets.cc`
    *   **不使用文件系统 (No VFS)**: 代码没有挂载 SPIFFS/LittleFS 文件系统。
    *   **内存映射 (MMAP)**: 使用 `esp_partition_mmap` 将整个 8MB 分区映射到内存地址空间。
    *   **自定义索引**: 分区头部包含文件索引表，直接通过内存指针访问数据 (Zero-copy)，极大提高了访问速度，特别是对于大图片和 AI 模型。
*   **内容**:
    *   `index.json`: 资源清单。
    *   `srmodels.bin`: 语音识别模型。
    *   `fonts.bin`: 字体文件。
    *   `*.jpg/png`: 界面图片、Emoji 动画。

---

## 5. OTA 升级机制 (Over-The-Air)

### 5.1 策略
*   **双分区 (A/B)**: 使用 `ota_0` 和 `ota_1` 交替升级。
*   **回滚支持**: `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` 已开启并在 `sdkconfig.defaults` 中配置。如果新固件启动失败，Bootloader 会自动回滚到旧版本。

### 5.2 流程
1.  **检查**: 应用程序检查服务器上的 `version.json`。
2.  **下载**: 下载新的 Firmware 写入非活动分区 (如当前在 ota_0，则写入 ota_1)。
3.  **切换**: 更新 `otadata` 分区，标记下一次从新分区启动。
4.  **重启**: 系统重启进入新固件。

---

## 6. PSRAM (外部内存) 使用

ESP32-S3-BOX-3 配备 16MB PSRAM，对于运行本项目至关重要。

*   **配置**: 在 `sdkconfig` 中开启 `CONFIG_ESP32S3_SPIRAM_SUPPORT`。
*   **用途**:
    *   **音频缓冲区**: 录音和播放的大块 RingBuffer。
    *   **显示帧缓冲**: LVGL 的全屏或部分渲染缓冲区 (Draw Buffers)。
    *   **AI 模型运行**: TensorFlow Lite / ESP-DL 模型推理时的张量运算内存。
    *   **大文件处理**: OTA 下载时的临时缓存。
*   **代码体现**:
    *   搜 `MALLOC_CAP_SPIRAM` 可见到显式申请 PSRAM 的位置。
    *   大内存分配 (如 > 10KB) 通常由 heap allocator 自动放入 PSRAM (如果配置了 `CONFIG_SPIRAM_USE_MALLOC`)。

---

## 7. 深入解析 (Q&A)

### Q1: 为什么可以通过在线烧录升级 `assets.bin`，并且看起来只更新了部分内容（如字体、唤醒词）？

**A: 这是一种“全量替换”的错觉，实际上是整个 Assets 分区的完整重写，但因为机制独立，不会影响主程序。**

1.  **独立分区**: `assets` 是一个独立的数据分区（位于 `0x800000`），与存放代码的 App 分区（`ota_0`/`ota_1`）物理隔离。
    *   **更新原理**: `Assets::Download` 函数 (`main/assets.cc`) 会从 URL 下载一个新的 `assets.bin` 文件。
    *   **写入过程**: 该函数会**擦除**整个 `assets` 分区的内容，并逐扇区写入新下载的二进制数据。
    *   **结果**: 无论你是只想更新一个字体文件，还是更新所有图片，服务器端都需要生成一个包含所有旧资源+新资源的完整 `assets.bin` 包。客户端下载并覆盖整个分区。
    *   **主程序完整性**: 因为 App 分区地址（`0x20000`）完全不同，所以升级资源绝对不会破坏主程序的逻辑。

2.  **“在线生成”的妙处**:
    *   虽然客户端是在全量下载，但服务器端（官网或配置工具）可能动态生成了这个 `assets.bin`。
    *   当你在线选择“更换唤醒词”时，服务器脚本（类似 `scripts/build_default_assets.py`）会在云端把基础资源（字体、图片）和你选择的新唤醒词模型打包成一个新的 `assets.bin` 供你下载。
    *   **体验上**: 用户感觉只是“更新了配置”，但**底层实现**是“重刷了资源分区”。

### Q2: 为什么我用 `idf.py flash` 烧录时，会把自己在线更新好的 assets 覆盖掉？

**A: 因为 `idf.py flash` 默认会将本地构建的所有分区镜像同步到设备上。**

1.  **构建系统行为**:
    *   每次编译时，CMake 脚本（`managed_components/espressif__esp_mmap_assets/project_include.cmake`）会检查本地的资源目录，并调用脚本生成一个本地的 `assets.bin`。
    *   这个本地 `assets.bin` 默认只包含你工程目录下 `assets/` 里的原始文件。
2.  **烧录行为**:
    *   当你执行 `idf.py flash` 时，工具链会读取 `partition_table`，发现 `assets` 分区需要被烧录。
    *   它会无情地用你**本地生成的旧版/默认版** `assets.bin` 覆盖掉设备里通过**在线 OTA 更新的最新版** `assets.bin`。
    *   **地址冲突**: 两者都指向 Flash 的 `0x800000` 地址，物理上是同一个位置。

### Q3: 整个工程的存储资源是如何分配的？烧录地址的底层逻辑是什么？

**Flash 地址分配逻辑 (16MB Total):**

| 地址范围 (Hex) | 长度 | 用途 | 谁占用？ | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| `0x000000` - `0x007FFF` | 32 KB | **Bootloader** | `bootloader.bin` | 芯片上电最先运行，负责加载 App |
| `0x008000` - `0x008FFF` | 4 KB | **Partition Table** | `partition-table.bin` | 也就是 `16m.csv` 的二进制形式，告诉系统后续分区在哪 |
| `0x009000` - `0x00CFFF` | 16 KB | **NVS** | `nvs` (Config) | 运行时动态数据：WiFi密码、唯一ID、音量设置 |
| `0x00D000` - `0x00EFFF` | 8 KB | **OTA Data** | `ota_data_initial.bin` | 只有 8KB，记录“当前应该从 ota_0 还是 ota_1 启动” |
| `0x00F000` - `0x00FFFF` | 4 KB | **PHY Init** | `phy_init` | 射频校准数据 |
| `0x020000` - `0x40FFFF` | ~4 MB | **App A (ota_0)** | `xiaozhi.bin` | **主程序代码** (出厂或升级版本A) |
| `0x410000` - `0x7FFFFF` | ~4 MB | **App B (ota_1)** | (Empty/Backups) | **主程序代码** (升级版本B，轮换使用) |
| `0x800000` - `0xFFFFFF` | 8 MB | **Assets** | `assets.bin` | **资源数据** (字体、图片、模型) |

**总结**:
*   **代码 (App)** 在 `0x20000` (或 `0x410000`)。
*   **资源 (Assets)** 在 `0x800000`。
*   **配置 (NVS)** 在 `0x9000`。
*   **`idf.py flash`** 就像一个搬运工，它手里拿着一份清单（分区表），清单上所有列出的 `.bin` 文件，它都会一股脑搬到对应的地址上去，不管那个地址上原来有什么。

---

## 8. 构建产物与烧录原理 (Build Artifacts & Flashing Principle)

### 8.1 关键构建产物 (`build/` 目录)

在执行 `idf.py build` 后，`build` 目录下会生成多个 `.bin` 文件。以下是必须烧录到 Box-3 的核心文件及其作用：

| 文件名 | 路径 (相对 build) | 作用 | 烧录地址 |
| :--- | :--- | :--- | :--- |
| **bootloader.bin** | `bootloader/bootloader.bin` | **二级引导程序**。芯片 ROM 里的代码运行后，首先加载它。它负责初始化硬件、校验分区表、决定从哪个 OTA 分区启动 App。 | `0x0000` |
| **partition-table.bin** | `partition_table/partition-table.bin` | **分区表二进制**。由 `partitions/v2/16m.csv` 编译而来，定义了 Flash 所有的隔间布局。 | `0x8000` |
| **ota_data_initial.bin** | `ota_data_initial.bin` | **OTA 初始数据**。初始化状态下，它告诉 Bootloader："请默认从 Factory 或 ota_0 分区启动"。 | `0xD000` |
| **xiaozhi.bin** | `xiaozhi.bin` | **主程序固件**。你编写的所有 C++ 业务逻辑、LVGL 界面代码、WiFi 连接逻辑都在这里。 | `0x20000` |
| **generated_assets.bin** | `generated_assets.bin` | **资源包**。由脚本自动打包 `assets/` 目录下的字体、图片、模型生成的。**注意：这就是那个会覆盖在线资源的“罪魁祸首”。** | `0x800000` |

### 8.2 `idf.py flash` 工作原理

`idf.py flash` 本质上是一个自动化的**批处理脚本**，其工作流程如下：

1.  **编译检查**: 确保所有 `.bin` 都是基于最新代码生成的。
2.  **生成烧录清单**: 读取 `build/flasher_args.json` 文件。这个 JSON 文件精准定义了哪些文件要烧录到哪些地址。
3.  **调用 esptool**: 最终通过调用 `esptool.py` (Espressif 的底层串口烧录工具) 执行实际的写入操作。

**底层指令等效于：**
```bash
esptool.py -p (PORT) -b 460800 --before default_reset --after hard_reset --chip esp32s3 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \
0x0 bootloader/bootloader.bin \
0x8000 partition_table/partition-table.bin \
0xd000 ota_data_initial.bin \
0x20000 xiaozhi.bin \
0x800000 generated_assets.bin
```

**为什么会覆盖？**
*   **无脑写入**: `write_flash` 命令是非常底层的，它**不会**去对比 Flash 里已有的数据是否比本地的“新”。它只知道你命令它把本地的 `generated_assets.bin` 写到 `0x800000`。
*   **全量覆盖**: 只要清单里有 `assets` 分区，它就会重写该区域。

### 8.3 如何避免覆盖在线资源？

如果你希望只更新代码 (`xiaozhi.bin`) 而保留通过在线 OTA 获取的 `assets`：

1.  **方法一 (推荐): 使用 `idf.py app-flash`**
    *   该指令只会烧录主程序 (`xiaozhi.bin`)，**跳过** Bootloader、分区表和 Assets 分区。
    *   指令：`idf.py -p COMx app-flash`

2.  **方法二: 手动指定文件**
    *   使用 `esptool.py` 手动只写 App 分区。
