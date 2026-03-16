核心结论：一份合格的《系统资源基线可视化报告》必须是**“静态尸检”与“动态心电图”的结合**。它不能只告诉你“还剩多少”，必须告诉你“还能塞进什么”。

以下是该文档的详细工程规范，分为输入、输出、可视化细节与合格标准四个维度。

---

### 一、 输入信息与资料清单 (Input Prerequisites)

你需要采集两类完全不同的数据：**编译时数据（Static）** 和 **运行时数据（Runtime）**。

#### 1. 静态编译数据 (Static Build Artifacts)

* **来源**: 编译输出文件。
* **具体指令/文件**:
* **`idf.py size`**: 获取总体的 Flash/RAM 占用概览。
* **`idf.py size-components`**: 获取每个组件（如 `lvgl`, `esp_audio`）具体吃了多少内存。这是做“代码裁剪”的依据。
* **`partitions.csv`**: 获取物理分区表布局（NVS, Factory, SPIFFS 的起始地址和大小）。
* **`project_name.map`**: （可选，深度分析用）用于追踪具体的函数是否错误地放在了 IRAM 中。



#### 2. 动态运行时数据 (Dynamic Runtime Logs)

* **来源**: 设备串口日志 (UART Logs)。你需要在代码特定位置（如 `app_main` 末尾或周期性 Task 中）打印堆栈信息。
* **必需的 Log 抓取指令**:
* **堆内存水位**:
```c
// 必须区分 内部SRAM 和 外部PSRAM
ESP_LOGI("MEM", "Internal Free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
ESP_LOGI("MEM", "SPIRAM Free: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
ESP_LOGI("MEM", "Largest Free Block: %d bytes", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

```


* **任务栈水位 (High Water Mark)**:
* 打印所有 Task 的历史最小剩余栈空间（防止栈溢出）。
* *工具*: `vTaskList()` (需在 menuconfig 开启 FreeRTOS Trace)。





---

### 二、 文档输出格式 (Output Format)

* **推荐格式**: Markdown (集成 Mermaid 图表) 或 PDF (嵌入 Python 生成的高清图)。
* **文档结构标准**:
1. **Executive Summary (高层结论)**: 一句话告诉老板/客户，资源够不够？（例：“剩余 Flash 充足，但内部 SRAM 仅剩 20KB，高风险。”）
2. **Flash Resource Analysis (存储分析)**: 静态分区饼图 + 组件占用 Treemap。
3. **RAM Resource Analysis (内存分析)**: 堆叠柱状图（SRAM vs PSRAM）。
4. **Task Health (任务健康度)**: 任务栈使用率横向条形图。



---

### 三、 核心图表定义 (Visualization Specs)

一份合格报告必须包含以下 **3 幅核心图表**，缺一不可。

#### 图表 1: Flash 物理分区全景图 (The "Estate Map")

* **类型**: **嵌套饼图 (Nested Pie Chart)** 或 **旭日图 (Sunburst)**。
* **内容**:
* **内圈**: 物理分区 (Factory App, NVS, SPIFFS, Bootloader, Reserved)。
* **外圈**: App 分区内的组件分布 (LVGL, ESP-ADF, Wi-Fi Stack, User Code)。


* **含义**:
* *一眼看出*: “我们的代码体积主要被谁吃掉了？”
* *决策点*: 如果 App 分区已占 90%，是否需要压缩 SPIFFS 空间来扩容 App？



#### 图表 2: 内存分层堆叠图 (The "Memory Layer Cake")

* **类型**: **堆叠柱状图 (Stacked Bar)**。
* **X轴**: 两个柱子 —— [Internal SRAM] 和 [External PSRAM]。
* **堆叠层级 (从下往上)**:
1. **Static (.data/.bss)**: 全局变量，死都移不走的。
2. **IRAM (.text)**: 中断代码，最贵资源。
3. **Dynamic Heap (Used)**: 运行时 `malloc` 占用的。
4. **Free (Fragmented)**: 碎片化的剩余空间（不可用）。
5. **Free (Largest Block)**: 真正可用来分配大数组的连续空间。


* **含义**:
* *一眼看出*: “PSRAM 买了没用上？SRAM 是不是快爆了？”
* *决策点*: 如果 SRAM 的 Free Block < 50KB，严禁引入新的大型蓝牙/Wi-Fi 库。



#### 图表 3: 任务栈压力测试图 (The "Task Pressure Gauge")

* **类型**: **子弹图 (Bullet Chart)** 或 **横向条形图**。
* **内容**: 列出 Top 10 任务。
* 灰色背景条：分配的栈大小 (Stack Size)。
* **红色实条**: 历史最大使用量 (Watermark)。


* **含义**:
* *一眼看出*: “哪个任务分配了 4KB 栈却只用了 500字节？（浪费）” “哪个任务只剩 50 字节就要溢出了？（危险）”



---

### 四、 合格标准 (Success Criteria / KPIs)

如果不满足以下指标，这份报告就是**不合格**的：

1. **必须区分 SRAM 与 PSRAM**:
* *不合格*: 笼统地说“内存还剩 4MB”。(SRAM 可能只有 10KB 了，系统随时会崩)。
* *合格*: “Internal SRAM 剩余 30KB (危)，PSRAM 剩余 4MB (充裕)。”


2. **必须包含“碎片化指数”**:
* *不合格*: “剩余内存 100KB”。
* *合格*: “剩余内存 100KB，但最大连续块只有 10KB。” (这意味着你甚至无法申请一个 320x240 的单色图片 buffer)。


3. **必须包含“OTA 冗余评估”**:
* 报告必须回答：“当前 App 大小是否允许进行 OTA 升级？” (即 `App_Size * 2 <= App_Partition_Total`)。


4. **可视化红线**:
* 所有图表必须画出 **80% 警戒线**。任何资源超过此线，必须标记为 **AMBER** 或 **RED** 风险等级。



---

### 下一步行动建议

你可以直接使用我之前提供的 Python 脚本作为原型。为了满足上述标准，你需要补充 **运行时 Log 数据**。

**行动指令**:
请在现有设备上运行以下代码片段，并将串口打印的 Log 发给我。我将为你生成符合上述标准的《系统资源基线可视化报告》雏形。

```c
// 在系统稳定运行 1 分钟后调用
void print_system_baseline() {
    printf("--- BEGIN RESOURCE LOG ---\n");
    // 1. Heap Info
    printf("HEAP_INT_TOTAL: %d\n", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    printf("HEAP_INT_FREE: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("HEAP_INT_MAX_BLOCK: %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    printf("HEAP_EXT_FREE: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    // 2. Task List (需要开启 FreeRTOS Trace)
    char *taskListBuffer = malloc(1024);
    if (taskListBuffer) {
        vTaskList(taskListBuffer);
        printf("TASK_LIST:\n%s\n", taskListBuffer);
        free(taskListBuffer);
    }
    printf("--- END RESOURCE LOG ---\n");
}

```