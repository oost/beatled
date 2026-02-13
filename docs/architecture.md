# Beatled Architecture

## System Overview

```mermaid
graph TB
    subgraph Server["Raspberry Pi 4 - Server"]
        AI[Audio Input<br/>PortAudio] --> BD[BeatDetector<br/>BTrack]
        BD --> TS[Tempo State]
        TS --> HTTP[HTTP/S Server<br/>:8443]
        TS --> UDP_S[UDP Server<br/>:9090]
        UDP_S --> BC[UDP Broadcast<br/>:8765]
    end

    subgraph Client["React Client"]
        UI[React SPA] <-->|HTTPS| HTTP
    end

    subgraph Pico["Pico W Devices"]
        P1[Pico W #1] <-->|UDP Unicast| UDP_S
        P1 -.->|Receives| BC
        P2[Pico W #2] <-->|UDP Unicast| UDP_S
        P2 -.->|Receives| BC
        P1 --> LED1[WS2812 LEDs]
        P2 --> LED2[WS2812 LEDs]
    end
```

## Pico W State Machine

Each Pico W device follows this state machine for synchronization:

```mermaid
stateDiagram-v2
    [*] --> STARTED
    STARTED --> INITIALIZED: Board init complete
    INITIALIZED --> REGISTERED: Hello response received
    REGISTERED --> TIME_SYNCED: Time sync complete
    TIME_SYNCED --> TEMPO_SYNCED: Tempo/NextBeat received
    TEMPO_SYNCED --> TEMPO_SYNCED: Re-sync (new tempo data)
```

| State | Description |
|-------|-------------|
| STARTED | Initial state after power-on |
| INITIALIZED | WiFi connected, peripherals ready |
| REGISTERED | Server acknowledged device (assigned client_id) |
| TIME_SYNCED | NTP-style clock offset established |
| TEMPO_SYNCED | Receiving beat data, LEDs active |

## Synchronization Sequence

```mermaid
sequenceDiagram
    participant P as Pico W
    participant S as Server

    Note over P: STATE: INITIALIZED
    P->>S: HELLO_REQUEST (board_id)
    S->>P: HELLO_RESPONSE (client_id)
    Note over P: STATE: REGISTERED

    loop Time Sync
        P->>S: TIME_REQUEST (orig_time)
        S->>P: TIME_RESPONSE (orig, recv, xmit)
        Note over P: Calculate clock offset
    end
    Note over P: STATE: TIME_SYNCED

    P->>S: TEMPO_REQUEST
    S->>P: TEMPO_RESPONSE (beat_ref, period, program)
    Note over P: STATE: TEMPO_SYNCED

    loop Steady State
        S->>P: NEXT_BEAT (beat_ref, period, count, program)
        Note over P: Schedule LED update at beat_ref
        S-->>P: BEAT (broadcast, informational)
    end
```

## Pico W Dual-Core Architecture

```mermaid
graph LR
    subgraph Core0["Core 0 - Network"]
        EL[Event Loop] --> CMD[Command Handler]
        CMD --> SM[State Manager]
        UDP[UDP Listener] --> EQ[Event Queue]
        EQ --> EL
    end

    subgraph IPC["Inter-core"]
        ICQ[Intercore Queue]
    end

    subgraph Core1["Core 1 - LEDs"]
        LP[LED Processor] --> WS[WS2812 Driver]
        REG[Registry<br/>Shared State] --> LP
    end

    CMD -->|tempo, program updates| ICQ
    ICQ --> LP
    CMD -->|mutex-protected| REG
```

## Server Components

| Component | Description |
|-----------|-------------|
| `BeatDetector` | Wraps BTrack for real-time beat detection from audio input |
| `AudioBufferPool` | Pre-allocated pool of audio buffers (zero-allocation in audio path) |
| `UDPServer` | Handles device registration, time sync, and tempo distribution |
| `HTTPServer` | REST API for the web client + static file serving |
| `ServiceController` | Start/stop management for beat detector and other services |

## Client Architecture

| Layer | Technology |
|-------|-----------|
| UI Framework | React 18 + TypeScript |
| Routing | React Router v6 (data loaders) |
| Styling | Tailwind CSS 4 + shadcn/ui |
| Charts | Chart.js + react-chartjs-2 |
| Build | Vite 7 |
| PWA | vite-plugin-pwa (offline support) |
