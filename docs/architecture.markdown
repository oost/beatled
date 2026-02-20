---
title: Architecture
layout: default
nav_order: 3
---

# Architecture

Beatled is a distributed system for real-time beat-synchronized LED control. It consists of three main components that communicate over a local network.

## System Overview

```mermaid
graph TB
    subgraph Server["Raspberry Pi 4 - Server"]
        AI[Audio Input<br/>PortAudio] --> BD[BeatDetector<br/>BTrack]
        BD --> TS[Tempo State]
        TS --> UDP_S[UDP Server<br/>:9090]
        UDP_S --> BC[UDP Broadcast<br/>:8765]
        TS --> HTTPS[HTTPS Server<br/>:8443]
    end


    subgraph Devices["LED Controllers"]
        P1[Pico W] <-->|UDP Unicast| UDP_S
        P1 -.->|Receives| BC
        P2[ESP32] <-->|UDP Unicast| UDP_S
        P2 -.->|Receives| BC
        P1 --> LED1[WS2812 LEDs]
        P2 --> LED2[WS2812 LEDs]
    end

    subgraph Client["Clients (React / iOS / macOS)"]
        UI[React SPA/iOS/macOS] <--> |HTTPS| HTTPS
    end

```

## Components

### Beat Server (C++)

The central server running on a Raspberry Pi. Responsibilities:

- **Audio capture** via PortAudio from a USB microphone
- **Beat detection** using the BTrack algorithm (onset detection + tempo estimation)
- **HTTPS API** (RESTinio + OpenSSL) for the web dashboard
- **UDP broadcast** of tempo and beat timing to all controllers
- **UDP server** for controller registration and time synchronization

| Component           | Description                                                         |
| ------------------- | ------------------------------------------------------------------- |
| `BeatDetector`      | Wraps BTrack for real-time beat detection from audio input          |
| `AudioBufferPool`   | Pre-allocated pool of audio buffers (zero-allocation in audio path) |
| `UDPServer`         | Handles device registration, time sync, and tempo distribution      |
| `HTTPServer`        | REST API for the web client + static file serving                   |
| `ServiceController` | Start/stop management for beat detector and other services          |

### Clients

Three client apps connect to the server over HTTPS:

- **React (Web)**: Single-page application served by the beat server. Real-time tempo display, service control, LED program selection, PWA-enabled for mobile.
- **iOS**: Native SwiftUI app. Same functionality as the web app in a native iOS experience.
- **macOS**: Native SwiftUI app. Sidebar-based layout sharing the same SwiftUI views as the iOS app.

| Layer        | Technology                        |
| ------------ | --------------------------------- |
| UI Framework | React 18 + TypeScript             |
| Routing      | React Router v6 (data loaders)    |
| Styling      | Tailwind CSS 4 + shadcn/ui        |
| Charts       | Chart.js + react-chartjs-2        |
| Build        | Vite 7                            |
| PWA          | vite-plugin-pwa (offline support) |

### LED Controller Firmware (C)

Firmware running on Raspberry Pi Pico W and ESP32 microcontrollers. A 10-module HAL allows the same application code to compile for 5 targets: `pico`, `pico_freertos`, `posix`, `posix_freertos`, and `esp32`.

- **Dual-core architecture**: Core 0 handles networking, Core 1 drives LEDs (single-core chips share via FreeRTOS scheduling)
- **State machine**: STARTED → INITIALIZED → REGISTERED → TIME_SYNCED → TEMPO_SYNCED
- **NTP-style time sync** with the server for precise beat alignment
- **8 LED patterns**: snakes, random, sparkle, greys, drops, solid, fade, fade color
- **Inter-core communication** via shared registry with mutex protection
- **Platform support**: Pico W (PIO+DMA), ESP32-S3/C3 (RMT), macOS (Metal simulation)

## Build & Deployment

- **Server**: CMake + vcpkg, cross-compiled for ARM via Docker
- **Clients**: Vite + React (web), Xcode (iOS/macOS)
- **Pico W**: CMake + Pico SDK, compiled to UF2 firmware
- **ESP32**: ESP-IDF (`idf.py build`), flashed via USB serial
- **POSIX**: CMake native build for macOS/Linux development
- **Deployment**: systemd service on Raspberry Pi, UF2 flash for Pico W, serial flash for ESP32
