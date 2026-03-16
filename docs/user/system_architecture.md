# System Architecture & Topology

## 1. System Topology Overview

The Xiaozhi ESP32 client is designed with a layered architecture, centering around a singleton `Application` controller that orchestrates data flow between the Hardware Abstraction Layer (HAL), Audio Services, and Network Protocols.

```mermaid
graph TD
    %% (0) Style Definitions
    classDef hardware fill:#e1f5fe,stroke:#01579b,stroke-width:2px;
    classDef core fill:#fff3e0,stroke:#e65100,stroke-width:2px;
    classDef service fill:#e8f5e9,stroke:#1b5e20,stroke-width:2px;
    classDef cloud fill:#f3e5f5,stroke:#4a148c,stroke-width:2px;

    %% (1) Hardware Specifics
    subgraph Hardware_Layer ["(1) Hardware Layer <br/> "]
        direction TB
        MainBoard["ESP32-S3 Board"]:::hardware
        Peripherals["Peripherals"]:::hardware
        IMU["IMU Sensor"]:::hardware
    end

    %% (2) Board Abstraction Layer (HAL)
    subgraph HAL ["(2) Board Abstraction Layer <br/> "]
        direction TB
        Board_Factory["Board Factory"]:::service
        I_Display["Display Interface"]:::service
        I_Audio["Audio Codec Interface"]:::service
        I_Network["Network Interface"]:::service
        
        Board_Factory --> I_Display
        Board_Factory --> I_Audio
        Board_Factory --> I_Network
    end

    %% (3) Application Core
    subgraph Core_Logic ["(3) Application Core <br/> "]
        direction TB
        App["Application (Singleton)"]:::core
        EventQueue["Main Event Loop"]:::core
        StateMachine["Device State Machine"]:::core
        
        App --> EventQueue
        App --> StateMachine
    end

    %% (4) System Services
    subgraph Services ["(4) System Services <br/> "]
        direction TB
        AudioService["Audio Service"]:::service
        McpServer["MCP Server"]:::service
        OTA["OTA & Version Manager"]:::service
        Assets["Asset Manager"]:::service
        
        AudioService -- "Voice Data" --> Protocol
        McpServer -- "Tool Result" --> Protocol
    end

    %% (5) Communication Layer
    subgraph Protocol_Layer ["(5) Communication Layer <br/> "]
        direction TB
        Protocol["Protocol Interface"]:::service
        MqttImpl["MQTT Implementation"]:::service
        WsImpl["WebSocket Implementation"]:::service
        
        MqttImpl -.-> Protocol
        WsImpl -.-> Protocol
    end

    %% (6) Cloud Server
    subgraph Cloud_Server ["(6) Cloud Server <br/> "]
        Server["Xiaozhi Server"]:::cloud
    end

    %% Hardware to HAL
    MainBoard --> Board_Factory
    Peripherals --> Board_Factory
    IMU -- "Events: Tap or Shake" --> App

    %% HAL to Core
    I_Network -- "Network Events" --> App
    
    %% Core Orchestration
    App -- "Init & Control" --> Board_Factory
    App -- "Control: Start or Stop" --> AudioService
    App -- "Send or Receive" --> Protocol
    App -- "Register Tools" --> McpServer

    %% Audio Flow
    AudioService -- "PCM Playback" --> I_Audio
    I_Audio -- "PCM Recording" --> AudioService
    
    %% Server Comms
    Protocol <-->|"(6) JSON Control and Bin Audio"| Server
```

## 2. Core Modules Breakdown

### 2.1 Application Core (`main/application.cc`)
The `Application` class implements the Singleton pattern and serves as the central nervous system of the device.
- **Event Loop**: It maintains a FreeRTOS `EventGroup` to handle asynchronous events (Network, Audio, Buttons, IMU) in a non-blocking manner (`Application::Run`).
- **State Machine**: Manages logical states (Idle, Listening, Speaking, Connecting) via `DeviceStateMachine`, ensuring valid transitions.
- **Orchestration**: Directs the flow of data. For example, when the Wake Word is detected, it transitions the state to `Listening` and commands the `AudioService` to stream data to the `Protocol` layer.

### 2.2 Board Abstraction Layer (`main/boards/`)
To support the diverse range of hardware (ESP32-S3-BOX, generic dev kits, proprietary boards), the system uses a Factory Pattern.
- **`Board` Base Class**: Defines virtual methods for `GetDisplay()`, `GetAudioCodec()`, `GetNetwork()`, etc.
- **Concrete Implementations**:
  - `WifiBoard`: Standard WiFi-based boards.
  - `Ml307Board`: Cellular (4G) based boards.
- **Build System Integration**: `CMakeLists.txt` dynamically selects the implementation based on `CONFIG_BOARD_TYPE`.

### 2.3 Audio Service (`main/audio/`)
The `AudioService` handles the complex pipeline of voice interaction.
- **Dual-Task Architecture**:
  - **Input Task**: Captures raw PCM from `AudioCodec` -> Processors (AEC, AGC, VAD, NS) -> `Encoder` -> Protocol Send Queue.
  - **Output Task**: Protocol Decode Queue -> `Decoder` -> Resampler -> `AudioCodec` Playback.
- **Codecs**: Supports Opus for efficient network transmission.
- **Wake Word**: Runs a lightweight edge-inference model (ESP-SR or custom) to detect activation phrases ("Xiaozhi").

### 2.4 Communication Protocol (`main/protocols/`)
Abstracts the transport layer, supporting both MQTT and WebSocket.
- **Unified Interface**: `Protocol` class defines `SendAudio`, `SendText`, `OnIncomingAudio`, etc.
- **Data Encapsulation**:
  - **Control Plane**: JSON messages for state sync, TTS text, STT results, and MCP tool calls.
  - **Data Plane**: Binary Opus packets for real-time low-latency audio streaming.

### 2.5 MCP (Model Context Protocol) Server (`main/mcp_server.cc`)
Enables the AI agent to interact with the device's physical capabilities.
- **Tool Registration**: Modules register capabilities (e.g., "turn_on_light", "get_battery") as tools.
- **JSON Schema**: Generates tool definitions dynamically to send to the AI model.
- **Execution**: when the Server sends a tool call request, `McpServer` looks up the callback and executes it, returning the result.

### 2.6 IMU Gesture Intelligence (`components/imu_streamer/`)
Uses the 6-axis IMU (ICM-42670-P) for local interaction.
- **Gesture Detection**: Direct integration of motion algorithms to detect Tap and Shake events.
- **Event Injection**: Injects events into the main `Application` event loop to trigger Wake-up or Interrupt actions without network dependency.

## 3. Data Flow Example: "Voice Interaction"

1.  **Wake Up**:
    *   User says "Xiaozhi".
    *   `AudioService` detects Wake Word -> Signals `Application`.
    *   `Application` plays "Ding" sound -> Sets State to `Listening`.
2.  **Streaming**:
    *   `AudioService` starts encoding Mic data -> Pushes to Send Queue.
    *   `Application` creates `Protocol` packet -> Sends to Server.
3.  **Processing**:
    *   Server processes Audio -> Returns STT text (JSON).
    *   `Application` displays User text on Screen.
4.  **Response**:
    *   Server sends TTS start command + Audio Stream.
    *   `Protocol` receives Audio -> Pushes to Decode Queue.
    *   `AudioService` decodes -> Plays on Speaker.
5.  **Execution (Optional)**:
    *   If user asked "Turn on light", Server sends MCP JSON.
    *   `McpServer` executes generic GPIO tool -> Light turns on.
