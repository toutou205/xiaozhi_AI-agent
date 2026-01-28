# OTA System Logic Analysis

This document details the firmware update (OTA) logic for the Xiaozhi ESP32 system, based on source code analysis.

## 1. Summary of OTA Logic

*   **Trigger Condition**: The OTA check is triggered **automatically on startup**, deeply integrated into the main application initialization flow. It is **Blocking** (synchronous) during the startup phase, meaning the device checks for updates before entering the main "Idle" or "Listening" state.
*   **Protocol**: HTTP/HTTPS (JSON-based version negotiation).
*   **Verification**: SHA256 / Secure Boot compatible (standard ESP-IDF OTA).
*   **Fail-Safe**: Uses `rollback` mechanism. If the new firmware fails to boot, it reverts to the factory/previous partition.

## 2. Trigger Mechanism (Code Trace)

The update check is encapsulated in `Application::CheckNewVersion()` and is called during the application startup thread.

**Call Stack:**
1.  `app_main()` (in `main.cc`) -> Initializes Setup.
2.  `Application::Start()` (in `application.cc`) -> Starts the main logic.
3.  `std::thread([this]() { ... })` -> Spawns a dedicated startup thread.
4.  `CheckNewVersion()` (in `application.cc:402`) -> **The Core Trigger**.

> **Conclusion**: The device checks for updates **every time it boots up**, immediately after connecting to Wi-Fi.

## 3. Detailed Logic Flow (Step-by-Step)

The `CheckNewVersion()` function performs the following logic (Source: `main/application.cc` & `main/ota.cc`):

1.  **Wi-Fi Connection**: Waits for `wifi_connected` event.
2.  **Logic Loop** (Retry Mechanism):
    *   **GET/POST Request**: Sends device info (Mac, Chip ID) to the `OTA URL`.
    *   **Config Priority**:
        1.  `NVS` (Web Admin setting: `ota_url`) **[Highest Priority]**
        2.  `Kconfig` (Compile time default: `CONFIG_OTA_URL`)
    *   **JSON Response Parsing**:
        *   Expects: `{"firmware": { "version": "...", "url": "...", "force": 0 }}`.
        *   Checks `version`: Uses semantic versioning comparison (e.g., `2.2.0 > 2.1.0`).
        *   Checks `force`: If `force: 1`, ignores version check and upgrades anyway.
3.  **Decision**:
    *   **If New Version**: Calls `UpgradeFirmware()`.
        *   Downloads binary stream.
        *   Writes to Next OTA Partition.
        *   On success -> **Reboot**.
    *   **If No New Version**:
        *   Calls `ota_->MarkCurrentVersionValid()` to confirm the current boot was successful (anti-rollback).
        *   Proceeds to `ShowActivationCode` or Main Application Loop.

## 4. Logic Diagram (Mermaid)

```mermaid
flowchart TD
    Start([Device Boot]) --> Init[Hardware Init]
    Init --> WiFi{Wi-Fi Connected?}
    WiFi -- No --> RetryWifi[Retry]
    WiFi -- Yes --> OTA_Check[Application::CheckNewVersion]

    subgraph "OTA Process (ota.cc)"
        OTA_Check --> GetURL[Get URL (NVS > Default)]
        GetURL --> HTTP_Req[HTTP Request (version.json)]
        HTTP_Req --> Valid_Res{Valid JSON?}
        
        Valid_Res -- No --> LogErr[Log Error & Skip]
        Valid_Res -- Yes --> Compare{New Version > Current?}
        
        Compare -- "Yes (or Force=1)" --> Upgrade[UpgradeFirmware]
        Compare -- No --> Validate[Mark Current Img Valid]
    end

    Upgrade --> Download[Download .bin]
    Download --> WriteFlash[Write to Next Partition]
    WriteFlash --> Verify[Hash Verification]
    Verify -- Success --> Reboot([System Reboot])
    Verify -- Fail --> LogErr

    Validate --> Activation{Need Activation?}
    Activation -- Yes --> ShowCode[Show Code]
    Activation -- No --> MainLoop([Enter Main Listening Loop])
    LogErr --> MainLoop
```

## 5. Source Reference (Evidence)

*   **Trigger**: `Application::CheckNewVersion` definition at `main/application.cc` (Line 402).
*   **Startup Call**: `Application::Start` spawns the thread checking this function.
*   **URL Logic**: `Ota::GetCheckVersionUrl` at `main/ota.cc` (Line 43) prefers NVS `ota_url`.
*   **Upgrade Logic**: `Ota::Upgrade` at `main/ota.cc` (Line 264) handles the streaming and partition switches.
*   **Version Comparison**: `Ota::IsNewVersionAvailable` at `main/ota.cc` (Line 387) parses semantic versions.
