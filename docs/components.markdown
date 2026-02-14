---
title: Components
layout: default
nav_order: 3
has_children: true
---

# Components

Beatled consists of three main components that work together to deliver real-time beat-synchronized LED control.

| Component | Technology | Description |
|-----------|-----------|-------------|
| [Beat Server](server.html) | C++ (CMake, ASIO, PortAudio) | Audio capture, beat detection, UDP broadcast, HTTPS API |
| [Pico Firmware](pico.html) | C (Pico SDK) | Dual-core: networking on Core 0, LED rendering on Core 1 |
| [Web Client](client.html) | React 18, TypeScript, Vite | Dashboard for tempo monitoring and LED program control |

See the [Architecture](architecture.html) page for detailed diagrams and the full synchronization sequence, or the [Protocol](protocol.html) page for the binary UDP wire format.
