---
title: Components
layout: default
nav_order: 4
has_children: true
---

# Components

Beatled consists of three main components that work together to deliver real-time beat-synchronized LED control.

| Component | Technology | Description |
|-----------|-----------|-------------|
| [Beat Server](server.html) | C++ (CMake, ASIO, PortAudio) | Audio capture, beat detection, UDP broadcast, HTTPS API |
| [LED Controller](controller.html) | C (Pico SDK, ESP-IDF, POSIX) | Multi-platform firmware: Pico W, ESP32, macOS/Linux sim |
| [Clients](clients.html) | React / SwiftUI | Web, iOS, and macOS apps for monitoring and LED control |

See the [Architecture](architecture.html) page for system diagrams, the [HTTP API Reference](api.html) for REST endpoints, or [Server ↔ Controller Communication](server-controller-communication.html) for the binary UDP wire format.
