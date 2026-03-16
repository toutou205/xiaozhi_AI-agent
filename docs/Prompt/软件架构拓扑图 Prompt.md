《软件架构拓扑图》是系统的“空中交通管制图”。它不关心每一辆车（变量）的内部引擎如何工作，但必须精确掌控**车流（数据流）从哪里来、经过哪条路（IPC）、要在哪里停（Task）**。

以下是针对这份文档的深度拆解：

---

### 1. 输入信息清单 (Input Prerequisites)

要绘制一张能用于“防堵车”的拓扑图，你需要向 AI 提供以下两类数据：

#### A. 静态代码特征 (Static Code Artifacts)

- **任务创建 (Task Creation)**:
    
    - 搜索关键字：`xTaskCreate`, `xTaskCreatePinnedToCore`, `pthread_create`。
        
    - **关键参数**：任务名 (Name)、优先级 (Priority)、堆栈大小 (Stack Size)、核心绑定 (Core ID)。
        
- **通信对象 (IPC Objects)**:
    
    - **队列/管道**：`xQueueCreate`, `rb_create` (Ringbuffer), `audio_pipeline` (ESP-ADF 特有)。
        
    - **同步信号**：`xSemaphoreCreateBinary`, `xSemaphoreCreateMutex`, `xEventGroupCreate`。
        
- **数据流向逻辑 (Flow Logic)**:
    
    - 谁是**生产者**？(搜索 `xQueueSend`, `rb_write`, `i2s_read`)
        
    - 谁是**消费者**？(搜索 `xQueueReceive`, `rb_read`, `i2s_write`)
        
- **硬件中断 (ISR)**:
    
    - 搜索 `gpio_install_isr_service`, `spi_bus_initialize` 中的回调函数。这是数据的**源头**。
        

#### B. 动态运行时数据 (Dynamic Runtime Logs)

- **必选 Log**:
    
    - **`vTaskList` 输出**：这是最真实的快照，能看到隐藏的任务（如 LwIP 协议栈内部任务）。
        
    - **`vTaskGetRunTimeStats` 输出**：提供 CPU 占用率，帮助判断哪些节点是“繁忙节点”。
        

---

### 2. 输出格式与内容要素 (Output Format & Content)

**推荐格式**：**Mermaid `graph TD`** (适合版本管理) 或 **Draw.io/Visio** (适合演示)。

#### 图中必须包含的图例 (Legend):

1. **节点 (Nodes)**:
    
    - **[矩形] 任务 (Task)**: 必须标注 `(Prio: X, Core: Y)`。
        
    - **[菱形/圆柱] 缓冲 (Buffer/Queue)**: 必须标注 `(Size: N)`。
        
    - **[圆形] 硬件/ISR**: 数据源头或终点。
        
2. **连线 (Edges)**:
    
    - **粗实线 (===)**: **高带宽数据流** (High Bandwidth)。例如：麦克风 -> PCM数据 -> 唤醒词引擎。
        
    - **细实线 (---)**: **低速控制流** (Control Plane)。例如：按键事件 -> 播放暂停。
        
    - **虚线 (-.-)**: **同步/通知** (Signal)。例如：Semaphore give/take。
        

#### Mermaid 伪代码示例：

代码段

``` mermaid
graph TD
    %% 硬件层
    Mic[Mic Hardware] -->|ISR / DMA| I2S_Driver
    
    %% 高带宽路径 (音频流)
    I2S_Driver ==>|Raw PCM Ringbuf| Audio_Task[Audio Task<br/>Prio:20 Core:1]
    Audio_Task ==>|Processed PCM| WakeWord_Engine[WakeWord Task<br/>Prio:1]
    
    %% 控制路径
    Button_ISR -.->|Sem_Btn_Press| Main_Ctrl[Main Control Task<br/>Prio:5]
    Main_Ctrl -->|Stop Command| Audio_Task
```

---

### 3. 信息精度与分辨率等级 (Resolution Level)

**核心原则：系统级精度 (System Level)，而非函数级精度。**

- **可见 (Visible)**:
    
    - **任务级交互**: 必须看到 Task A 发数据给 Task B。
        
    - **共享资源**: 必须看到 Task A 和 Task B 都在读写同一个 `Global_Config_Struct` 或 `Queue`。
        
    - **数据包类型**: 连线上需标注传递的是什么（如 `Audio Chunk (512B)` 或 `JSON Event`）。
        
- **不可见 (Hidden - 噪音)**:
    
    - **局部变量**: Task 内部定义的 `int i` 或临时 buffer。
        
    - **函数调用链**: Task A 内部调用了 `func1() -> func2()`，若不涉及挂起/等待，则折叠不显示。
        
    - **简单的 Get/Set**: 对非竞争资源的简单读写。
        

**分辨率标准**：

> "如果把拓扑图比作地图，我要看到的是**高速公路和立交桥**，而不是每一辆车里的乘客（变量），也不需要看清路边的井盖（局部函数）。"

---

### 4. 合格标准 (Qualification Criteria)

一份合格的《软件架构拓扑图》必须能回答以下 **4 个致命问题**。如果回答不了，就是不合格。

#### KPI 1: 关键路径可见性 (Critical Path Visibility)

- **指标**：从 `Mic Hardware` 到 `Speaker Hardware` 的整条音频链路，是否用**粗线**连续贯通？
    
- **合格**：能一眼看出音频流经过了 3 个任务，跨越了 2 个核心。
    
- **不合格**：线条断裂，或者混杂在按键控制线中看不清。
    

#### KPI 2: 优先级倒置预警 (Priority Risk Check)

- **指标**：高优先级任务（如 Prio 20）是否在等待一个低优先级任务（Prio 5）持有的锁？
    
- **合格**：图中明确标注了任务优先级，且依赖关系清晰。
    
- **不合格**：没有标注优先级，无法评估实时性风险。
    

#### KPI 3: 瓶颈识别 (Bottleneck ID)

- **指标**：是否存在“多对一”的拥堵点？
    
- **合格**：能看到 `Display Task` 的输入队列有 5 个不同的来源（UI, WiFi, System, Log），提示该队列可能溢出。
    

#### KPI 4: 核心负载平衡 (Core Affinity)

- **指标**：是否能区分 Core 0 和 Core 1 的负载？
    
- **合格**：通过颜色或子图 (Subgraph) 将任务按核心分组。例如：左边是 Core 0 (Wi-Fi/BT)，右边是 Core 1 (Audio/AI)。
    

---
