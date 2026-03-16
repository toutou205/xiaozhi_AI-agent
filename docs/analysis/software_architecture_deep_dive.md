# 软件架构拓扑与优化深度分析报告

## 1. 深度解答：架构观测与实时性 (Deep Dive)

### Q1: 如何解决“单次快照”的局限性？(Sampling Strategy)
目前的 `resource_monitor` 是一个“定格动画”。为了优化决策，确实需要“监控录像”。

**建议实施方案**：
1.  **周期采样 (Polling)**:
    - 修改代码，创建一个 `PerformanceMonitor` 任务。
    - **频率**: 1Hz (每秒一次)。
    - **持续**: 300秒 (涵盖空闲、唤醒、对话、播报全流程)。
2.  **数据流式传输**:
    - 不要 Print 到串口 (会阻塞)，而是将 `(TaskName, CPU%, MinFreeStack)` 写入环形缓冲区 (RingBuffer)。
    - 通过 MQTT 或 WebSocket 实时推送到电脑端，或者存入 SD 卡/Flash 文件系统。
3.  **触发式抓取 (Triggered Dump)**:
    - 当 `IDLE` 任务低于 20% 时，自动触发一次“案发现场”快照，记录谁抢占了 CPU。

### Q2: 为什么 `imu_ai_task` 在静态分析中“隐身”了？(Static vs Dynamic)
这是一个经典的**“黑盒组件”**问题。
- **静态观测 (Static Analysis)**: 只能看到**“我们写的代码”**。
    - 扫描 `main/` 目录只能看到显式调用 `xTaskCreate` 的代码。
    - `imu_ai_task` 往往封装在 `managed_components` (如 `sscma_client` 或 `imu_streamer`) 的**预编译库 (.a)** 或**外部引用头文件**中。如果不去解压这些库的源码，静态分析就像“X光”穿不透铅板，导致“灯下黑”。
- **动态观测 (Dynamic Observation)**: 看到的是**“内核调度的现状”**。
    - 无论代码写在哪里（库、组件、ROM），只要它申请了 CPU 资源，FreeRTOS 内核就会给它发身份证 (TCB)。
    - **用途区别**:
        - **静态**: 设计架构、检查逻辑死锁、理解数据流。
        - **动态**: 抓“偷跑”资源的元凶（如本次发现的 IMU AI，33% CPU），验证实时性。

### Q3: 真正的优先级阶层 (Real Priority Hierarchy) 是什么？
在 FreeRTOS (ESP32) 中，**数字越大 = 权力越大**。

**系统现有权力阶梯 (Hierarchy)**:
1.  **上帝阶层 (Prio 23-24)**: `wifi`, `ipc`.
    - **特征**: 必须瞬时响应，否则网络断连或双核崩溃。**它们可以随时打断**下面的所有人。
2.  **实时阶层 (Prio 18-20)**: `sys_evt`, `tiT` (LwIP).
    - **特征**: 处理系统级事件，需快速响应。
3.  **关键业务 (Prio 8)**: `audio_input`.
    - **特征**: 录音不能卡，一卡就有不可逆的爆音/丢字，必须比 AI 计算更优先。
4.  **计算阶层 (Prio 3-5)**: `imu_ai_task`, `mqtt`, `audio_detection`, `taskLVGL` (部分配置).
    - **特征**: CPU 消耗大户。它们会被 WiFi 打断，但在本系统中，`imu_ai_task` (Prio 5) 会压制 `main` (Prio 1)。
5.  **平民阶层 (Prio 1)**: `main`.
    - **特征**: 应用逻辑和状态机。只有等上面大佬都休息了，它才能动。

**风险发现**: 核心业务逻辑 `main` 只有 Prio 1。若 `imu_ai_task` (Prio 5) 持续高负载 (33%)，可能导致业务逻辑（如按键响应、状态流转）出现数百毫秒的延迟。

---

## 2. 进阶追问：优化与决策 (Optimization & Strategy)

### Q4: `imu_ai_task` 是否需要被优化？
**结论：必须优化。** 33% 的 CPU 占用对于一个辅助功能（IMU手势/抬腕）来说**过高**，严重挤占了主业务资源。

**优化策略 (Strategy)**:
1.  **降频 (Reduce Frequency)**:
    - 检查 IMU 采样率。如果是 100Hz，能否降到 25Hz？手势识别通常不需要极高采样率。
2.  **卸载 (Offload)**:
    - 利用 IMU 传感器自带的 DMP (Digital Motion Processor) 或 FIFO。让硬件做步数/抬腕检测，触发中断后再唤醒 CPU，而不是 CPU 轮询读取。
3.  **让权 (Yield)**:
    - 在 `imu_ai_task` 的循环中增加 `vTaskDelay`。目前的 33% 占用暗示它可能在一个紧凑循环中 polling。
4.  **降级 (Deprioritize)**:
    - 将其优先级从 5 降至 1 或 0。让它只在 CPU 空闲时运行，绝不能抢占 `audio_input` 或 `main`。

**预期效果**:
- CPU 占用率降至 <5%。
- 系统发热降低，续航提升。
- 避免音频处理被人为打断。

### Q5: 任务优先级 (Priority) 的设置标准与流程是什么？
**量化标准 (Rate Monotonic Scheduling - RMS 原则)**:
- **频率越高，优先级越高**。
- **截止时间 (Deadline) 越短，优先级越高**。

**设计流程 (Workflow)**:
1.  **初始设定 (经验值)**:
    - **硬件驱动 (HW)**: 20+ (Wi-Fi, BT)
    - **硬实时 (Audio/Motor)**: 10-15 (不能抖动)
    - **软实时 (UI/Sensor)**: 5 (偶尔卡一下没死人)
    - **业务逻辑 (App)**: 1-3
    - **后台 (Logging)**: 0
2.  **动态采集 (Measure)**:
    - 运行典型场景（如：播放音乐 + 疯狂晃动设备），分别记录 `RunTimeStats`。
3.  **调优 (Tune)**:
    - **KPI**: 是否出现音频 Buffer Underflow (爆音)？UI 是否掉帧？
    - **动作**: 如果音频卡顿，提升 Audio Prio。如果 UI 卡顿，检查是否有高 Prio 任务（如 `imu_ai_task`）霸占 CPU，降级该任务。
4.  **标准化 (Standardize)**:
    - 形成项目的 `Priority Map` 文档，新任务必须在此表中申请“铺位”。

### Q6: 拓扑图“快照”信息的价值是什么？
虽然是快照，但对于**嵌入式实时系统 (RTOS)**，它极具代表性。
- **稳定性**: 嵌入式系统的任务列表通常是**静态**的（Boot 时创建，一直跑到死）。不像服务器进程那样朝生暮死。
- **统计学意义**: `vTaskGetRunTimeStats` 统计的是**自启动以来的总时间占比 (% Time)**，而不是“当前这一毫秒”的瞬时值。
    - **价值点 1**: 33% 的 `imu_ai_task` 说明它**长期平均**占据了 1/3 的算力，这是一个**确定性特征**，而非偶然波动。
    - **价值点 2**: 栈水位 (Min Free Stack) 是历史最低值。快照里的这个值能直接告诉你**是否分配了过多的内存**（例如：分配 8K，只用了 1K，浪费 7K）。

### Q7: 支持 MQTT 本地自检吗？资源消耗如何？
**完全支持，且是 IoT 设备的标准设计模式。**

**实现逻辑**:
1.  **周期触发**: 每 10 分钟或在特定事件（报错）时。
2.  **采集**: 调用 `vTaskGetRunTimeStats` 获取文本。
3.  **压缩**: 文本格式太费流量，应解析为 JSON 或二进制：`{ "tasks": [ {"n":"main", "c":1}, {"n":"imu", "c":33} ] }`。
4.  **推送**: Publish 到 Topic `device/status/report`。

**资源消耗评估**:
- **CPU**: 低。`vTaskGetRunTimeStats` 需遍历链表并关闭中断，耗时约 50-200us（视任务数而定）。每分钟做一次几乎无感。
- **Memory**: 需要约 2KB-4KB 的临时 Buffer 来暂存统计数据。用完即 `free`，对系统长期运行无影响。
- **Bandwidth**: 极低。每条消息 ~500 Bytes。


---

## 3. 代码级深度诊断：`imu_ai_task` (Code Level Diagnosis)

经过对 `modules/imu_streamer/src/imu_streamer.cpp` 的深度扫描，我们找到了 33% CPU 占用的根源。

### 3.1 诊断发现 (Findings)
1.  **高频推理 (High Frequency)**:
    - 代码设定 `const int64_t interval_us = 20000;` (50Hz)。即每 20ms 就要进行一次完整的 `run_classifier` (DSP + 神经网络推理)。对于简单的 Tap/Shake 检测，50Hz 属于杀鸡用牛刀。
2.  **忙等循环 (Busy Wait)**:
    - 使用了 `if (now < next_time) vTaskDelay(1);`。这种手动软定时不如 FreeRTOS 的 `vTaskDelayUntil` 高效，且仅自旋等待 1 Tick，导致上下文切换频繁。
3.  **数据搬运 (Heavy Memmove)**:
    - 每次循环都执行 `memmove` 滑动窗口来处理 Tensor 数据。这是内存操作密集型逻辑。
4.  **繁杂的 I/O (Verbose I/O)**:
    - 循环内包含 `check_usb_input()` 和大量的 `snprintf` 字符串格式化操作，在嵌入式实时任务中这是大忌。

### 3.2 优化方案 (Optimization Plan)

| 优化点 | 修改前 (Before) | 修改后 (After) | 预期收益 |
| :--- | :--- | :--- | :--- |
| **降频** | 20ms (50Hz) | **40ms (25Hz)** | CPU 占用直接减半 (33% -> ~16%) |
| **调度** | `vTaskDelay(1)` | `vTaskDelayUntil` | 减少无谓的 Context Switch，让出更多空闲切片 |
| **I/O** | 每次循环检查 USB | 每 10 次循环检查一次 | 降低外设轮询开销 |
| **优先级** | Prio 5 | **Prio 1** (同 main) | 彻底消除对 Audio (Prio 8) 的潜在干扰 |

**预期效果**: 综合以上措施，CPU 占用率有望从 **33% 降至 8-10%**，且不会明显影响“抬腕亮屏”的体验（人眼反应速度约 200ms，40ms 的采样延迟完全可接受）。

---

## 4. 可视化：为什么不是饼状图？ (Visualization)

`vTaskGetRunTimeStats` 本质上是一个生成 **CSV/文本表格** 的内核函数，它不知道什么是“图形”。饼状图是 **由人（或上位机工具）** 根据数据绘制的。

为了满足您的需求，我为您准备了一个 **Python 脚本**，可以将串口打印的文本一键转换为饼状图。

### 脚本逻辑
1.  **输入**: 复制串口监视器中的 `TASK_LIST` 或 `RUN_TIME_STATS` 文本块。
2.  **处理**: 正则解析任务名和 CPU 时间/占比。
3.  **输出**: 生成 `cpu_usage_pie_chart.html` (交互式网页图表) 或 `.png` 图片。

**使用方法**:
```bash
python scripts/visualize_runtime_stats.py
# (粘贴日志数据)
```
此方案支持本地离线生成，数据安全且直观。

### 4.1 生成结果 (Generated Chart)
![CPU Usage Pie Chart](cpu_usage_pie_chart.png)



### 3.3 优化结果验证 (Verification)

**测试时间**: 2026-02-05
**测试场景**: 25Hz 采样 + 400ms IO 轮询 + 消除重复文件
**实测数据**:

```text
RUN_TIME_STATS:
imu_ai_task     13648707                16%   <-- (Before: 33-34%)
opus_codec      17392467                21%
audio_input     35154786                43%
...
```

**结论**:
1.  **CPU 占用减半**: 从 34% 降至 16%，降幅达 **~53%**。
2.  **正常水平**: 考虑到 `run_classifier` 需要进行 DSP 和矩阵运算，在 ESP32-S3 上 16% 的占用是合理的（平均每次推理约耗时 6-7ms）。此优化已成功释放了宝贵的 CPU 资源给音频和 Wi-Fi 任务。
