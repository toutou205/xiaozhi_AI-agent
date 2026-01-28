# OTA 升级实战指南

本指南将指导你利用当前的 `xiaozhi-esp32` 工程，在局域网内搭建一个简单的 OTA 测试环境，亲身体验固件升级流程。

## 前置准备
*   **硬件**: ESP32-S3-BOX-3 设备
*   **软件**: VS Code + ESP-IDF 环境 (已就绪)
*   **环境**: PC 和设备需在同一局域网 (连接同一个 WiFi)

---

## 第一步：修改版本号

为了让设备识别到“新版本”，你需要先编译一个版本号比当前更高的固件。

1.  打开根目录下的 **`CMakeLists.txt`** 文件。
2.  找到以下代码（约第 7 行）：
    ```cmake
    set(PROJECT_VER "2.1.0")
    ```
3.  将其修改为更高的版本，例如 **`2.2.0`**：
    ```cmake
    set(PROJECT_VER "2.2.0")
    ```

## 第二步：编译新固件

在 VS Code 终端中运行编译命令：

```powershell
idf.py build
```

编译成功后，生成的固件位于：
`build/xiaozhi.bin`

## 第三步：搭建本地 HTTP 服务器

我们使用 Python 自带的 HTTP 服务器功能快速搭建一个文件服务器。

1.  **准备文件目录**:
    在任意位置（例如桌面）创建一个名为 `ota_server` 的文件夹。

2.  **复制固件**:
    将刚才编译好的 `build/xiaozhi.bin` 复制到 `ota_server` 文件夹中。

3.  **创建版本描述文件**:
    在 `ota_server` 文件夹中新建一个名为 `version.json` 的文本文件，内容如下（**请替换 IP 为你电脑的局域网 IP**）：

    ```json
    {
        "firmware": {
            "version": "2.2.0",
            "url": "http://<你的电脑IP>:8000/xiaozhi.bin",
            "force": 0
        }
    }
    ```
    > **如何查看电脑 IP**: 在终端运行 `ipconfig`，找到 **IPv4 地址**。

4.  **启动服务器**:
    在 VS Code 终端中，`cd` 进入该文件夹并启动服务器：
    ```powershell
    # Windows PowerShell
    cd path/to/ota_server
    python -m http.server 8000
    ```
    *此时，你的电脑 8000 端口已开放 Web 服务。*

## 第四步：配置设备 OTA

1.  **进入配网模式**:
    *   长按设备顶部的 **Boot** 按键（如果有）或通过组合键复位进入配网模式。
    *   或者如果你的设备已经连网，也可以通过设备的 IP 进入后台（如果开启了 web server），但最稳妥的是重置配网。
2.  **连接热点**: 电脑或手机连接设备热点 `SmartAgent-XXXX` (我们之前改的前缀)。
3.  **访问后台**: 浏览器打开 `192.168.4.1`。
4.  **设置 OTA 地址**:
    *   点击 **Advanced** 标签页。
    *   在 **Custom OTA URL** 输入框中，填写 JSON 文件的地址：
        `http://<你的电脑IP>:8000/version.json`
    *   **重要**: 确保 IP 正确且设备能 ping 通电脑。
5.  **保存配置**: 点击 **Save**。

## 第五步：触发升级与验证

1.  点击保存后，设备会自动重启并连接 WiFi。
2.  **观察终端日志** (通过 USB 连接查看串口监视器 `idf.py monitor`):
    *   你会看到日志提示 `Checking version from: http://.../version.json`。
    *   接着显示 `New version available: 2.2.0`。
    *   随即开始下载：`Upgrading firmware from http://.../xiaozhi.bin`。
    *   进度条走完后，显示 `Firmware upgrade successful` 并自动重启。
3.  **验证**:
    *   重启后，观察启动日志开头，此时 `Project version` 应显示为 **`2.2.0`**。

---

## 进阶：使用 Cloudflare Tunnel 进行公网 OTA

如果你的 ESP32 设备位于外网，或者你想通过公网进行 OTA 升级测试，可以使用 Cloudflare Tunnel 将本地的 Python HTTP 服务器暴露到公网。

### 1. 安装与启动 Cloudflare Tunnel

在运行 Python HTTP 服务器的终端（即 SSH 连接的 Linux 服务器）中，安装并启动 `cloudflared`：

```bash
# 假设已经安装 cloudflared
cloudflared tunnel --url http://localhost:8000
```

终端会输出一个临时的公网地址，例如：
`https://romantic-curie-12345.trycloudflare.com`

### 2. 更新 version.json

修改 `ota_server` 目录下的 `version.json`，将 `url` 字段替换为 Cloudflare 提供的公网地址：

```json
{
    "firmware": {
        "version": "2.2.0",
        "url": "https://romantic-curie-12345.trycloudflare.com/xiaozhi.bin",
        "force": 0
    }
}
```

### 3. 配置设备 OTA 地址

将设备的 OTA 检查地址（Upgrade URL）设置为新的 `version.json` 公网地址：
`https://romantic-curie-12345.trycloudflare.com/version.json`

> **注意**: 确保 `sdkconfig` 中启用了 `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE` (默认开启)，以便 ESP32 能够验证 Cloudflare 的 HTTPS 证书。

---

## 常见问题排查
*   **Connect timeout / Connection failed**: 设备无法访问电脑 IP。
    *   检查电脑防火墙是否允许 Python 访问网络。
    *   检查电脑和设备是否在同一 WiFi 下。
    *   尝试在电脑浏览器访问 `http://localhost:8000/version.json` 确认服务器是否启动。
