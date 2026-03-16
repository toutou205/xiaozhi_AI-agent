# System Resource Chart Generation Guide (系统资源图表生成说明书)

本文档详细说明了如何使用项目中的 Python 脚本生成系统资源基线可视化报告图表。文档涵盖了**总控脚本**与**独立脚本**的使用方法，并明确了数据的直接来源。

---

## 1. 核心数据源说明 (Data Source Truth Table)

图表的准确性取决于数据源。请务必区分**静态编译数据**（Static）与**动态运行时数据**（Runtime）。

| 数据类型 | **直接来源 (Direct Source)** | **获取方式** | **对应脚本参数** | **用途** |
| :--- | :--- | :--- | :--- | :--- |
| **Flash 物理分区布局** | `partitions/v2/16m.csv` | 项目源码中的分区表文件 | `--partitions` | 绘制 Flash 旭日图 Inner Ring (房间规划) |
| **App 固件大小** | `build/xiaozhi.bin` | 编译生成的二进制文件 (字节大小) | `--app_size` (可自动探测) | 绘制 Flash 旭日图 Outer Ring (App 使用率) |
| **Assets 资源大小** | `build/assets.bin` | 编译生成的资源镜像文件 (字节大小) | `--asset_size` (可自动探测) | 绘制 Flash 旭日图 Outer Ring (资源使用率) |
| **RAM (堆内存) 数据** | **UART 串口日志** (运行时) | 设备运行 `resource_monitor` 任务打印的 Log | `--log` (需保存为 txt) | 绘制 RAM 蛋糕图 (SRAM/PSRAM 分布) |
| **Task (任务栈) 数据** | **UART 串口日志** (运行时) | 设备运行 `vTaskList` 打印的 Log | `--log` (需保存为 txt) | 绘制 Task 压力图 (栈水位风险) |

> [!NOTE]
> **关于 `current_log.txt` 的澄清**:
> `current_log.txt` 只是一个**中间载体**。它本身不是数据产生者，而是您手动从串口终端复制粘贴下来的“快照”。
> *   **Flash 分析脚本 (`gen_chart_flash.py`)**：**完全不使用**也不读取 log 文件，只看 bin 文件和 csv。
> *   **RAM/Task 分析脚本 (`gen_chart_ram.py`, `gen_chart_task.py`)**：必须依赖 log 文件来获取运行时状态。

---

## 2. 总控脚本使用说明 (Master Wrapper Guide)

**脚本路径**: `scripts/generate_resource_charts.py`

这是推荐的日常使用方式。它会自动探测 `build/` 目录下的 `.bin` 文件大小，并一次性调度所有子脚本生成三张图表。

### 2.1 一键生成指令 (One-Click Command)

在项目根目录下运行终端（确保已激活 Python 虚拟环境）：

```powershell
python scripts/generate_resource_charts.py --partitions partitions/v2/16m.csv --log current_log.txt --output docs/images/resource_report
```

### 2.2 工作流程 (Workflow)
1.  **自动探测**: 脚本自动检查 `build/xiaozhi.bin` 和 `build/assets.bin` 是否存在，并读取其文件大小（字节）。
2.  **调度 Flash**: 调用 `gen_chart_flash.py` 绘制存储分布图。
3.  **调度 RAM**: 调用 `gen_chart_ram.py` 读取 `--log` 文件绘制内存分布图。
4.  **调度 Task**: 调用 `gen_chart_task.py` 读取 `--log` 文件绘制任务压力图。

---

## 3. Flash 独立脚本使用说明 (Flash Script Guide)

**脚本路径**: `scripts/gen_chart_flash.py`

如果您只调节 Flash 图表的配色、字体或布局，可以直接运行此脚本，无需重新生成所有图表。

### 3.1 手动运行指令 (Manual Command)

由于独立脚本**不具备**自动探测 `build/` 目录的功能，您**必须手动传入**文件大小（字节）：

```powershell
# 示例：假设 App=2972256, Assets=7996097
python scripts/gen_chart_flash.py --partitions partitions/v2/16m.csv --app_size 2972256 --asset_size 7996097 --output docs/images/resource_report
```

### 3.2 配置与微调 (Configuration)

您可以在脚本开头部分修改以下常量来调整视觉效果：

*   **文件位置**: 打开 `scripts/gen_chart_flash.py`
*   **可配置项**:

    ```python
    # Font Settings
    FONT_SIZE_TITLE = 16
    FONT_SIZE_INNER = 10      # 内圈（分区名）字号
    FONT_SIZE_OUTER = 9       # 外圈（使用率）字号
    FONT_COLOR_INNER = 'black' # 内圈字体颜色 (建议 Black 以在浅色块上易读)

    # Label Position
    LABEL_DISTANCE_OUTER = 0.85 # 0.85 = 显示在环内; 1.05 = 显示在环外

    # Legend Settings
    LEGEND_FONT_SIZE = 13     # 图例字号
    LEGEND_TITLE = "Partition Map Details"

    # Color Palette (支持 Hex 或 Matplotlib 颜色名)
    COLOR_MAP_BASE = {
        'app_active': 'tab:blue',   # 蓝色
        'app_backup': 'tab:purple', # 紫色 (与 Active 区分)
        'assets':     'tab:orange', # 橙色
        # ...
    }
    ```

### 3.3 常见问题 (FAQ)

*   **Q: 为什么生成的 Assets 分区显示 95% Used？**
    *   **A**: 您的 `assets.bin` 文件（7.99MB）是按照分区大小（8MB）生成的完整镜像（包含填充）。脚本真实反映了**烧录文件**的大小。如果您想看实际有效数据大小，请使用未填充的原始文件（如 `generated_assets.bin`）的大小作为参数传入。

*   **Q: `current_log.txt` 这里没用吗？**
    *   **A**: 没用。Flash 图表只关心“规划图 (`.csv`)”和“装修建材 (`.bin`)”，不关心设备运行时的心跳。

---

## 4. 总结 (Summary)

*   **查看整体报告**: 运行总控脚本 -> 查看 `docs/System_Resource_Baseline_Report.md`。
*   **微调图片样式**: 编辑 `scripts/gen_chart_*.py` 中的 CONFIGURATION 部分 -> 重新运行脚本。
