核心结论：**《依赖与版本锁定清单》是软件世界的“物料清单 (BOM)”。它必须消除任何“大约”、“最新版”或“兼容”的模糊字眼，确保在任何时间、任何机器上编译出的二进制文件比特级一致 (Bit-exact)。**

以下是生成该文档的详细作业指南。

---

### 1. 输入信息与资料清单 (Input Checklist)

要生成这份清单，你需要收集**三个维度**的源数据：**框架层、组件层、环境层**。

#### A. 核心文件 (The "Truth" Files)

这是最直接的证据，必须提供给 AI：

1. **`idf_component.lock`** (关键):
    
    - _作用_: 记录了组件管理器实际下载的**确切版本**和**SHA256哈希值**。这是“现状”的铁证。
        
2. **`idf_component.yml`** (意图):
    
    - _作用_: 记录了开发者人为设定的**版本约束**（例如 `version: "^1.0.0"`）。对比 Lock 文件，可以发现哪些组件被“悄悄升级”了。
        
3. **`.gitmodules` & `git submodule status`**:
    
    - _作用_: 处理那些没有通过组件管理器，而是通过 Git 子模块引入的库（旧项目常见）。
        

#### B. 环境指纹 (Environment Fingerprint)

1. **ESP-IDF 框架版本**:
    
    - _指令_: 运行 `idf.py --version`。
        
    - _注意_: 必须精确到 commit hash（如 `v5.1.2-15-g89a3b2`），不能只说 `v5.1`。
        
2. **编译器版本**:
    
    - _指令_: `xtensa-esp32s3-elf-gcc --version`。
        
    - _作用_: 不同的编译器版本优化逻辑不同，可能导致同样的 C 代码产生不同的 Bug。
        
3. **Python 依赖 (可选但推荐)**:
    
    - _文件_: `requirements.txt` 或 `pip freeze` 输出。
        
    - _作用_: 防止因 `esptool` 或 `kconfig` 版本差异导致烧录失败。
        

---

### 2. 文档输出格式 (Output Format)

这份文档不应只是一张列表，而应是一份**“软件供应链审计表”**。

**推荐格式**: Markdown 表格 + 风险标注。

#### **示例文档结构**

> # 依赖与版本锁定清单 (Dependency Manifesto)
> 
> **Project**: Xiaozhi-ESP32 | **Date**: 2026-02-06
> 
> **Build System**: ESP-IDF `v5.1.2` (Commit: `c3a5b2...`)
> 
> **Compiler**: xtensa-esp32s3-elf-gcc `12.2.0`
> 
> ### 1. 核心组件审计表 (Component Audit)
> 
> |**组件名称 (Component)**|**锁定版本 (Locked Ver)**|**来源 (Source)**|**约束范围 (Constraint)**|**风险等级**|**备注/Hash**|
> |---|---|---|---|---|---|
> |**espressif/esp_rainmaker**|`1.3.0`|Registry|`^1.0`|🟢 Safe|-|
> |**lvgl/lvgl**|`8.3.11`|Registry|`==8.3.11`|🟡 Freeze|**UI 强依赖**，切勿升级到 v9.x|
> |**espressif/esp-adf**|`v2.6-beta`|Git Submodule|-|🔴 **Risk**|处于 Beta 阶段，API 可能变更|
> |**my_private_lib**|`0.0.1`|Local|Path: `../libs`|🟡 Local|依赖本地路径，CI 服务器需同步|
> 
> ### 2. 传递依赖透视 (Transitive Dependencies)
> 
> _以下组件未在 `idf_component.yml` 中声明，但被其他组件依赖引入：_
> 
> - `espressif/esp_diagnostics` (v1.0.0) - _Introduced by esp_rainmaker_
>     
> - `espressif/button` (v3.0.1) - _Introduced by esp-box bsp_
>     
---

### 3. 合格标准 (Qualification Criteria)

一份合格的《依赖与版本锁定清单》必须通过以下 **KPI 考核**：

1. **哈希级精度 (Hash-Level Precision)**:
    
    - **指标**: 任何来自 Git 的依赖，必须记录 **Commit SHA** (如 `a1b2c3d`)，绝不能只记录分支名 (如 `master`)。
        
    - _理由_: `master` 每天都在变，今天能编译，明天可能就挂了。
        
2. **来源溯源 (Source Traceability)**:
    
    - **指标**: 每一行必须明确来源是 `IDF Registry`（官方仓库）、`Git`（代码库）还是 `Local`（本地文件）。
        
    - _理由_: 混合来源是构建失败的头号原因（例如服务器上没有你的本地路径）。
        
3. **传递依赖显性化 (Transitive Clarity)**:
    
    - **指标**: 必须列出“隐性依赖”。
        
    - _理由_: 你可能只引入了 `esp-box`，但它私自带入了 `esp-sr` (语音识别)。如果 `esp-sr` 升级了导致内存暴涨，你必须知道是谁把它带进来的。
        
4. **复现性测试 (Reproduction Test)**:
    
    - **终极指标**: 拿这份清单给一个新的工程师，他在新电脑上配置环境后，编译出的 `.bin` 文件大小（字节数）是否与原环境**误差在 1KB 以内**？（如果能做到 Bit-exact 最好，但在嵌入式开发中较难，通常要求功能一致）。
        
