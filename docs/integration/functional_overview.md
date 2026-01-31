# TinyML Box3 BMI270 (ICM-42607 Edition) - 功能清单

本文档总结了当前工程的核心功能模块与技术实现。

## 1. 硬件驱动层 (Hardware Drivers)

### 传感器支持
*   **型号**: ICM-42607-P (替代原 BMI270)
*   **接口**: I2C (Master)
*   **参数配置**:
    *   **加速度量程**: +/- 16g (用于捕捉高动态手势)
    *   **陀螺仪量程**: 2000 dps
    *   **采样率 (ODR)**: 100Hz (与 TinyML 模型对齐)
*   **关键特性**:
    *   **Sensitivity Bug Fix**: 包含针对驱动程序 Sensitivity 计算错误的 Workaround，确保物理量纲正确转为 IEEE 754 浮点数 (单位: m/s²)。
    *   **Low Noise Mode**: 开启低噪声模式以提升信号质量。

## 2. 核心业务与推理 (Core Logic & Inference)

### Edge Impulse 集成
*   集成 Edge Impulse C++ SDK。
*   支持 TFLite Micro 推理引擎。
*   **Tensor Arena**: 静态内存分配，支持运行时元数据解析。

### 数据流处理 (IMU Streamer)
*   **滑动窗口 (Sliding Window)**: 实现 Rolling Buffer 机制，动态填充传感器数据。
*   **输入维度**: 6轴 (Acc X/Y/Z + Gyr X/Y/Z)。

### 后处理状态机 (Inference State Machine)
用于提升用户体验，过滤误触与抖动：
*   **防抖动 (Debouncing)**: 连续 N 帧维持同一分类才切换状态。
*   **置信度阈值 (Confidence Threshold)**: 必须超过设定值 (如 0.98) 才视为有效分类。
*   **状态锁定**: 在数据不稳定时保持上一有效状态，避免结果跳变。
*   **支持类别**:
    *   `Tap` (敲击)
    *   `Shake` (摇动)
    *   `Idle` (静置)

## 3. 运行时调优 (Runtime Tuning CLI)

基于 **USB Serial/JTAG** 的高性能交互终端，解决了传统 UART 在高吞吐下的卡死与丢包问题。

### 交互特性
*   **连接方式**: 通过 USB 线直连 (无需额外串口转 USB 工具)。
*   **非阻塞架构**: CLI 解析与推理任务分时复用，互不干扰。
*   **用户体验**:
    *   **Startup Banner**: 启动时显示使用说明与状态。
    *   **Prompt**: `box3> ` 提示符。
    *   **Smart Echo**: 支持退格键修改指令，提供 `[OK]` 或 `[ERROR]` 明确反馈。

### 指令集 (Commands)
输入 `help` 可查看最新指令列表：

*   **查询**:
    *   `help`: 显示帮助菜单。
    *   `status`: 显示当前各分类的 `Confidence` (置信度) 和 `Debounce Frames` (防抖帧数)。

*   **修改参数** (即时生效):
    *   `tap conf <0.0-1.0>`: 设置 Tap 的置信度阈值 (推荐 >0.95)。
    *   `tap frame <int>`: 设置 Tap 的最小连续触发帧数。
    *   *示例*: `tap conf 0.98`
    *   `shake conf/frame ...`: 设置 Shake 参数。
    *   `idle conf/frame ...`: 设置 Idle 参数。

## 4. 工程与工具 (Infrastructure & Tools)

### 自动化集成 Skill
*   路径: `.agent/skills/integrate-edge-impulse`
*   **deploy_model.py**: 
    *   自动化清洗 Zip 包（移除 Arduino 示例等）。
    *   自动注入/更新 CMakeLists.txt。
    *   分析 `model_metadata.h` 并输出 RAM 占用警告。

### 可视化验证
*   `tools/serial_plotter.py`: Python 脚本，用于电脑端实时波形绘制，辅助验证传感器数据方向与噪声。

### 构建系统
*   基于 ESP-IDF 组件化架构 (`components/modules`, `components/drivers`)。
*   CMake 自动依赖管理。
