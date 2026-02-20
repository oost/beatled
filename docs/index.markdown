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
    PI -->|UDP| C1["Pico W"] --> LED1["WS2812 LEDs"]
    PI -->|UDP| C2["ESP32"] --> LED2["WS2812 LEDs"]
    PI -->|HTTPS| WEB["React (Web)"]
    PI -->|HTTPS| IOS["iOS"]
    PI -->|HTTPS| MACOS["macOS"]
```

1. A **Raspberry Pi** captures audio from a USB microphone and runs real-time beat detection (BTrack algorithm)
2. Beat timing is broadcast over **UDP** to all LED controllers on the local network
3. Each **controller** (Pico W, ESP32, or POSIX simulator) uses NTP-style time synchronization to align its clock with the server, then drives a WS2812 LED strip in sync with the beat
4. A **web, iOS, or macOS client** lets you monitor tempo, switch LED programs, and control services from your phone or computer

## Components

| Component                         | Technology                   | Description                                               |
| --------------------------------- | ---------------------------- | --------------------------------------------------------- |
| [Beat Server](server.html)        | C++ (CMake, ASIO, PortAudio) | Audio capture, beat detection, UDP broadcast, HTTPS API   |
| [LED Controller](controller.html) | C (Pico SDK, ESP-IDF, POSIX) | Multi-platform firmware: Pico W, ESP32, macOS/Linux sim   |
| [Clients](clients.html)           | React / SwiftUI              | Web, iOS, and macOS apps for monitoring and LED control   |
| [Protocol](protocol.html)         | Binary UDP                   | Packed structs for registration, time sync, and beat data |

See the [Architecture](architecture.html) page for detailed diagrams.

## Getting Started

Get the server, web client, and Pico simulator running locally in minutes -- no hardware required.

**[Getting Started Guide](getting-started.html)**

## Repositories

| Repo                                                                 | Description                                                                       |
| -------------------------------------------------------------------- | --------------------------------------------------------------------------------- |
| [beatled](https://github.com/oost/beatled)                           | C++ server, React client, Docker build, and documentation                         |
| [beatled-pico](https://github.com/oost/beatled-pico)                 | Pico W firmware (C) with posix test port                                          |
| [beatled-beat-tracker](https://github.com/oost/beatled-beat-tracker) | Fork of [BTrack](https://github.com/adamstark/BTrack) for real-time beat tracking |
