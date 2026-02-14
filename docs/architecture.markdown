---
title: Architecture
layout: default
nav_order: 2
---

# Architecture

Beatled is a distributed system for real-time beat-synchronized LED control. It consists of three main components that communicate over a local network.

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

## Components

### Beat Server (C++)

The central server running on a Raspberry Pi. Responsibilities:

- **Audio capture** via PortAudio from a USB microphone
- **Beat detection** using the BTrack algorithm (onset detection + tempo estimation)
- **HTTPS API** (RESTinio + OpenSSL) for the web dashboard
- **UDP broadcast** of tempo and beat timing to all Pico clients
- **UDP server** for Pico client registration and time synchronization

| Component | Description |
|-----------|-------------|
| `BeatDetector` | Wraps BTrack for real-time beat detection from audio input |
| `AudioBufferPool` | Pre-allocated pool of audio buffers (zero-allocation in audio path) |
| `UDPServer` | Handles device registration, time sync, and tempo distribution |
| `HTTPServer` | REST API for the web client + static file serving |
| `ServiceController` | Start/stop management for beat detector and other services |

### Web Dashboard (React)

A single-page application served by the beat server over HTTPS:

- Real-time tempo display with beat visualization (Chart.js)
- Service control (start/stop beat detection, UDP broadcast)
- LED program selection
- Server log viewer
- PWA-enabled for mobile access

| Layer | Technology |
|-------|-----------|
| UI Framework | React 18 + TypeScript |
| Routing | React Router v6 (data loaders) |
| Styling | Tailwind CSS 4 + shadcn/ui |
| Charts | Chart.js + react-chartjs-2 |
| Build | Vite 7 |
| PWA | vite-plugin-pwa (offline support) |

### Pico Firmware (C)

Firmware running on Raspberry Pi Pico W microcontrollers:

- **Dual-core architecture**: Core 0 handles networking, Core 1 drives LEDs
- **State machine**: STARTED -> INITIALIZED -> REGISTERED -> TIME_SYNCED -> TEMPO_SYNCED
- **NTP-style time sync** with the server for precise beat alignment
- **8 LED patterns**: snakes, random, sparkle, greys, drops, solid, fade, fade color
- **Inter-core communication** via shared registry with mutex protection

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

## HTTP API (HTTPS, port 8443)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/status` | GET | Service status, tempo, client list |
| `/api/service/control` | POST | Start/stop services |
| `/api/tempo` | GET | Current tempo and time reference |
| `/api/program` | GET/POST | Get/set LED program |
| `/api/log` | GET | Server log tail |
| `/api/devices` | GET | Connected device list with IPs and last seen time |

All POST endpoints validate request body size (max 4KB) and required JSON fields. When `--api-token` is set, all endpoints require `Authorization: Bearer <token>`.

## Build & Deployment

- **Server**: CMake + vcpkg, cross-compiled for ARM via Docker
- **Client**: Vite + React, bundled as static assets served by the server
- **Pico**: CMake + Pico SDK, compiled to UF2 firmware
- **Deployment**: systemd service on Raspberry Pi, UF2 flash for Picos
