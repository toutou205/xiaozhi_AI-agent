# 可行性评估报告：IMU 动作识别 (Edge Impulse)

**评估结论**: **【可行 - 需重构 (Feasible with Refactoring)】**

本文档基于 `modules/imu_streamer` 和 `edge_impulse` 代码及 ESP32-S3-BOX-3 硬件现状进行评估。

## 1. 硬件资源评估


| 资源项 | 需求 (New Feature) | 现状 / 限制 (Current) | 评估结果 |
| :--- | :--- | :--- | :--- |
| **I2C 总线** | 需共享 I2C Master (ICM42607) | BOX-3 已有 I2C 总线 (Touch/Audio) | **兼容** (需确认从机地址不冲突) |
| **RAM (Heap)** | Arena: ~3.8KB; Buffer: ~3KB; Task: ~4KB | 剩余 Heap > 100KB (预估) | **无风险** (总占用约 11KB) |
| **Flash** | Edge Impulse SDK (~150KB+) | App 分区 (16MB Flash) 空间充足 | **无风险** |
| **CPU 算力** | 100Hz 推理 (每 10ms 一次) | 双核 240MHz, 当前负载较低 | **无风险** (TFLite Micro 效率高) |

## 2. 软件架构评估

### 2.1 代码现状 (imu_streamer.cpp)
**风险点**:
提供的 `imu_streamer.cpp` 是一个 **独立的 Demo 程序**，包含 `while(1)` 死循环和直接的 `usb_serial_jtag` 操作。
*   **阻塞问题**: 如果直接调用 `imu_streamer_start()`，会卡死主线程 (`Application::Run`)。
*   **串口冲突**: 该 Demo 直接操作 `USB Serial/JTAG` 寄存器，可能与 ESP-IDF 标准 `console` 或 `log` 冲突。

### 2.2 集成方案 (Integration Plan)
必须将 `imu_streamer` 拆解为 **"非阻塞服务 (Service)"**：

1.  **Driver层**: 保留 `sensor_icm42607`，确保使用 `Board` 单例提供的 I2C 句柄，而不是自己重新初始化 I2C。
2.  **Service层 (需新建)**:
    *   剥离 CLI 交互逻辑。
    *   将 `while(1)` 循环改为 `Process()` 方法，由 `Application` 的定时器或独立 Task 周期性调用。
3.  **CLI层**: 将 `check_usb_input` 的调试功能整合进现有的 Shell 或移除 (生产环境不需要)。

## 3. 碰撞测试

*   **引脚冲突**: 需检查 `sensor_icm42607.c` 中定义的 I2C 引脚是否与 `esp_box3_board.cc` 中的 `I2C_NUM_0` 引脚 (GPIO 8/18 等) 一致。
*   **任务饿死**: 推理任务 (10ms 周期) 优先级不应高于音频处理任务，否则会导致爆音。

## 4. 结论与建议

**Go/No-Go**: **GO (可以实施)**

**关键路径**:
1.  **Refactor**: 将 `imu_streamer.cpp` 重构为 `IMUService` 类。
2.  **Board Integration**: 在 `esp_box3_board.cc` 中添加 `GetIMU()` 接口。
3.  **Non-blocking**: 确保推理逻辑分片执行，不占用主循环超过 5ms。
