---
title: Components
layout: default
nav_order: 4
has_children: true
---

# Components

Beatled consists of four main components that work together to deliver real-time beat-synchronized LED control.

| Component | Technology | Description |
|-----------|-----------|-------------|
| [Beat Server](server.html) | C++ (CMake, ASIO, PortAudio) | Audio capture, beat detection, UDP broadcast, HTTPS API |
| [Pico Firmware](pico.html) | C (Pico SDK, ESP-IDF, POSIX) | Multi-platform: Pico W, ESP32, macOS simulator. 5 ports with shared HAL |
| [Web Controller](client.html) | React 18, TypeScript, Vite | PWA dashboard for tempo monitoring and LED program control |
| [iOS Controller](ios.html) | SwiftUI, Swift Charts | Native iOS app for monitoring and control |

See the [Architecture](architecture.html) page for detailed diagrams and the full synchronization sequence, or the [Protocol](protocol.html) page for the binary UDP wire format.
