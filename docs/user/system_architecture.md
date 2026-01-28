# Xiaozhi AI Interaction Architecture

This document illustrates the system architecture and interaction flow between the local ESP32-S3-BOX-3 hardware and the Xiaozhi AI Cloud.

## 1. High-Level System Overview

The system uses a hybrid protocol approach:
- **MQTT**: Used for control signaling, session management, and text data (STT/LLM responses).
- **UDP**: Used for real-time, low-latency, encrypted audio streaming (Upstream & Downstream).

```mermaid
graph TD
    subgraph "Local Hardware (ESP32-S3-BOX-3)"
        Driver[Hardware Drivers]
        App[Application Logic]
        Audio[Audio Service]
        Proto[Protocol Layer]
        
        Driver <--> Audio
        Audio <--> App
        App <--> Proto
    end

    subgraph "Xiaozhi AI Cloud"
        MqttBroker[MQTT Broker]
        AudioServer[Audio Server]
        Core[AI Core Service]
        
        MqttBroker <--> Core
        AudioServer <--> Core
    end

    Proto -- "MQTT (JSON Control/Text)" --> MqttBroker
    Proto -- "UDP (Opus Encoded Audio)" --> AudioServer
```

## 2. Audio Pipeline & Wake Word Detection (Local)

This diagram details the local processing flow from microphone input to audio transmission. The **Wake Word Engine** runs locally on the ESP32 for low-latency activation.

```mermaid
graph LR
    %% (1) Hardware Input Layer
    Mic["Microphone (I2S)"] --> |"Raw PCM"| Codec["Audio Codec (ES7210)"]
    Codec --> |"Input Task"| AFE["AFE Audio Processor"]
    
    %% (2) ESP-SR Local Processing
    subgraph ESP_SR ["(2) ESP-SR (Local Processing) <br/> "]
        AFE --> |"Processed Audio"| WakeNet["Wake Word Engine"]
        AFE --> |"Voice Activity"| VAD["VAD Detector"]
    end

    %% (3) Event & State Management
    WakeNet --> |"Hi Xiaozhi"| AppEvent{Wake Event}
    AppEvent -- "Start Listening" --> StateMgr["State Machine"]
    
    StateMgr --> |"If State=Listening"| Gate["Audio Gate"]
    
    %% (4) Encoding & Transmission
    AFE --> |"Clean Audio"| Gate
    Gate --> |"Open"| Encoder["Opus Encoder"]
    Encoder --> |"Encoded Packets"| Queue["Send Queue"]
    Queue --> |"UDP Transport"| Cloud((Cloud))
```

## 3. Full Interaction Loop (Workflow)

This sequence diagram illustrates the lifecycle of a complete voice interaction: from waking up to receiving a response.

```mermaid
sequenceDiagram
    participant User
    participant ESP as ESP32 (Local)
    participant MQTT as Cloud (MQTT)
    participant UDP as Cloud (UDP - Audio)

    Note over User, ESP: 1. Wake Up
    User->>ESP: "Hi Xiaozhi" (Wake Word)
    ESP->>ESP: Local WakeNet Detects
    ESP->>ESP: State -> Listening
    ESP->>User: Play "Ding" Sound
    ESP->>MQTT: {"type": "hello"} (Open Session)
    MQTT-->>ESP: Session Ready (Server IP/Port)
    
    Note over User, ESP: 2. Request
    User->>ESP: "What is the weather?"
    activate ESP
    ESP->>UDP: Stream Opus Audio Packets >>
    ESP->>ESP: VAD detects silence (End of Speech)
    ESP->>MQTT: {"type": "stop"} (End of Speech)
    deactivate ESP
    ESP->>ESP: State -> Processing

    Note over UDP, MQTT: 3. Cloud Processing (ASR + LLM)
    MQTT-->>ESP: {"type": "stt", "text": "What is the weather?"}
    ESP->>User: Display User Text

    Note over UDP, MQTT: 4. Response
    MQTT-->>ESP: {"type": "llm", "emotion": "happy"}
    ESP->>User: Update Face Expression
    
    MQTT-->>ESP: {"type": "tts", "state": "start"}
    ESP->>ESP: State -> Speaking
    
    udp-->>ESP: << Stream Opus Audio (TTS) <<
    ESP->>User: Play Audio Response
    MQTT-->>ESP: {"type": "tts", "text": "It is sunny today."}
    ESP->>User: Display Assistant Text
    
    UDP-->>ESP: Audio Stream End
    MQTT-->>ESP: {"type": "tts", "state": "stop"}
    ESP->>ESP: State -> Idle (or Listening if Continuous)
```
