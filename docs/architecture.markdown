---
title: Architecture
layout: default
nav_order: 2
---

# Architecture

Beatled is a distributed system for real-time beat-synchronized LED control. It consists of three main components that communicate over a local network.

## System Overview

```
                      +---------------------------+
                      |     Raspberry Pi 4/5      |
                      |                           |
 +----------+        | +-------+    +-----------+ |        +----------------+
 |   USB    |------->| | Beat  |--->| Broadcast | |------->| Pico W #1      |
 |   Mic    | audio  | | Server|    | (UDP)     | | tempo  | (WS2812 LEDs)  |
 +----------+        | +---+---+    +-----------+ |        +----------------+
                      |     |                      |
                      |     | HTTPS                |        +----------------+
                      |     v                      |------->| Pico W #2      |
                      | +--------+                 | tempo  | (WS2812 LEDs)  |
                      | |  Web   |                 |        +----------------+
                      | |  UI    |                 |
                      | +--------+                 |        +----------------+
                      +---------------------------+  ------>| Pico W #N      |
                                                    tempo   | (WS2812 LEDs)  |
                                                            +----------------+
```

## Components

### Beat Server (C++)

The central server running on a Raspberry Pi. Responsibilities:

- **Audio capture** via PortAudio from a USB microphone
- **Beat detection** using the BTrack algorithm (onset detection + tempo estimation)
- **HTTPS API** (RESTinio + OpenSSL) for the web dashboard
- **UDP broadcast** of tempo and beat timing to all Pico clients
- **UDP server** for Pico client registration and time synchronization

### Web Dashboard (React)

A single-page application served by the beat server over HTTPS:

- Real-time tempo display with beat visualization (Chart.js)
- Service control (start/stop beat detection, UDP broadcast)
- LED program selection
- Server log viewer
- PWA-enabled for mobile access

### Pico Firmware (C)

Firmware running on Raspberry Pi Pico W microcontrollers:

- **Dual-core architecture**: Core 0 handles networking, Core 1 drives LEDs
- **State machine**: STARTED -> INITIALIZED -> REGISTERED -> TIME_SYNCED -> TEMPO_SYNCED
- **NTP-style time sync** with the server for precise beat alignment
- **8 LED patterns**: snakes, random, sparkle, greys, drops, solid, fade, fade color
- **Inter-core communication** via shared registry with mutex protection

## Communication Protocols

### HTTP API (HTTPS, port 8080)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/status` | GET | Service status, tempo, client list |
| `/api/service/control` | POST | Start/stop services |
| `/api/tempo` | GET | Current tempo and time reference |
| `/api/program` | GET/POST | Get/set LED program |
| `/api/log` | GET | Server log tail |

### UDP Synchronization Protocol

**Registration (port 9090):**
1. Pico sends `HELLO` request with board ID
2. Server responds with client acknowledgment
3. Pico requests time sync (NTP-style round-trip calculation)
4. Pico requests tempo sync

**Beat Broadcast (port 8765):**
- Server broadcasts `next_beat` messages with timing reference
- All registered Picos receive beat timing simultaneously
- Each Pico interpolates LED animations between beats

### Time Synchronization

The system uses an NTP-style protocol to synchronize clocks:

```
Pico                          Server
  |--- time_request (T1) ------->|
  |                               |  T2 = receive time
  |<-- time_response (T1,T2,T3) -|  T3 = transmit time
  |  T4 = receive time            |

  offset = ((T2 - T1) + (T3 - T4)) / 2
```

## Build & Deployment

- **Server**: CMake + vcpkg, cross-compiled for ARM via Docker
- **Client**: Vite + React, bundled as static assets served by the server
- **Pico**: CMake + Pico SDK, compiled to UF2 firmware
- **Deployment**: systemd service on Raspberry Pi, UF2 flash for Picos
