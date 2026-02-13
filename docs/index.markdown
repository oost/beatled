---
title: Home
layout: home
nav_order: 1
permalink: /
---

# Beatled

**Real-time beat-synchronized LED control for live music.**

Beatled listens to music through a USB microphone, detects the beat in real time, and synchronizes individually addressable LED strips across multiple wireless devices -- all with sub-millisecond precision.

Imagine a party where everyone's outfit lights up in sync with the music. Sew an LED strip into a jacket, wrap one around a hat, line a stage prop, or embed one in a costume -- each one driven by a tiny Pico W microcontroller connected to WiFi. A single Raspberry Pi in the corner picks up the music, finds the beat, and tells every device exactly when to flash. The result: dozens of people pulsing together to the same rhythm, with coordinated color patterns that react to every beat and tempo change.

The LED strips used are **WS2812** (also known as NeoPixel) -- addressable RGB LEDs where each pixel can be set to any color independently. The firmware ships with 8 built-in patterns (snakes, sparkles, color fades, and more) that are driven by the beat position, and new patterns can be added as simple C functions. The active pattern can be switched live from the web dashboard, and every connected device updates simultaneously.

## How It Works

```mermaid
graph LR
    MIC["USB Mic"] --> PI["Raspberry Pi<br/>(Beat Server)"]
    PI -->|UDP| P1["Pico W + LEDs"]
    PI -->|UDP| P2["Pico W + LEDs"]
    PI -->|UDP| P3["Pico W + LEDs"]
    PI -->|HTTPS| WEB["Phone / Browser"]
```

1. A **Raspberry Pi** captures audio from a USB microphone and runs real-time beat detection (BTrack algorithm)
2. Beat timing is broadcast over **UDP** to all Pico W devices on the local network
3. Each **Pico W** uses NTP-style time synchronization to align its clock with the server, then drives a WS2812 LED strip in sync with the beat
4. A **React web dashboard** lets you monitor tempo, switch LED programs, and control services from your phone

## Components

| Component | Technology | Description |
|-----------|-----------|-------------|
| [Beat Server](server.html) | C++ (CMake, ASIO, PortAudio) | Audio capture, beat detection, UDP broadcast, HTTPS API |
| [Web Client](client.html) | React 18, TypeScript, Vite | Dashboard for tempo monitoring and LED program control |
| [Pico Firmware](pico.html) | C (Pico SDK) | Dual-core: networking on Core 0, LED rendering on Core 1 |
| [Protocol](protocol.html) | Binary UDP | Packed structs for registration, time sync, and beat data |

See the [Architecture](architecture.html) page for detailed diagrams and the full synchronization sequence.

## Getting Started

### Prerequisites

- A **Raspberry Pi 4 or 5** (runs the beat server)
- One or more **Raspberry Pi Pico W** (each drives an LED strip)
- A **USB microphone** ([example](https://www.amazon.com/gp/product/B0138HETXU/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1))
- WS2812 / NeoPixel LED strips
- A development machine with Docker (for cross-compilation) or macOS/Linux (for local builds)

### Quick Start

Clone the repo and use the unified dev script:

```bash
# Build and run the server locally
utils/beatled.sh server --start-http

# In another terminal, start the React dev server
utils/beatled.sh client

# Build and run the Pico firmware locally (posix simulator)
utils/beatled.sh pico

# Run all tests
utils/beatled.sh test all
```

For full build and deployment instructions, see the [Server](server.html) and [Pico](pico.html) guides.

## Repositories

| Repo | Description |
|------|-------------|
| [beatled](https://github.com/oost/beatled) | C++ server, React client, Docker build, and documentation |
| [beatled-pico](https://github.com/oost/beatled-pico) | Pico W firmware (C) with posix test port |
| [beatled-beat-tracker](https://github.com/oost/beatled-beat-tracker) | Fork of [BTrack](https://github.com/adamstark/BTrack) for real-time beat tracking |
