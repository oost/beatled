---
title: Beatled Server
layout: default
nav_order: 4
---

# Beatled Server

The beat server is a C++ application that captures audio, detects beats in real time, and distributes timing data to Pico W devices over UDP. It also serves an HTTPS REST API and the React web dashboard.

## Architecture

| Module | Description |
|--------|-------------|
| `beat_detector` | Audio capture (PortAudio) and real-time beat detection (BTrack) |
| `core` | State management, configuration, client registry |
| `server/http` | HTTPS REST API (RESTinio + OpenSSL) and static file serving |
| `server/udp_server` | UDP request handler for device registration and time sync |
| `server/tempo_broadcaster` | Periodic UDP broadcast of beat timing to registered devices |
| `server/logger` | Ring-buffer logger exposed via the `/api/log` endpoint |

## Local Development

```bash
# Build and start the HTTPS server with the web client
utils/beatled.sh server --start-http

# With UDP server for Pico devices
utils/beatled.sh server --start-http --start-udp

# With CORS for the Vite dev server
utils/beatled.sh server --start-http --cors-origin "https://localhost:5173"

# With Bearer token authentication
utils/beatled.sh server --start-http --api-token "secret"

# Run server tests
utils/beatled.sh test server
```

The server is built with CMake and uses vcpkg for dependency management. The first build will configure CMake and install dependencies automatically.

### Prerequisites

- **macOS**: `brew install pkg-config cmake libtool automake autoconf autoconf-archive`
- **vcpkg**: Must be installed and `VCPKG_ROOT` set in your environment
- A **USB microphone** for audio capture (or use a virtual audio device for testing)

## TLS Certificates

The server requires TLS certificates for HTTPS. Generate self-signed certificates for local development:

```bash
utils/beatled.sh certs localhost
```

This creates certificates in the `certs/` directory. The server expects `cert.pem`, `key.pem`, and `dh_param.pem`.

## API Authentication

When started with `--api-token`, the server requires a Bearer token for all API requests:

```
Authorization: Bearer <token>
```

## Cross-Compilation

Cross-compile for Raspberry Pi (ARM64) using Docker:

1. Initialize submodules:

   ```bash
   git submodule update --init --recursive
   ```

2. Build the Docker build environment:

   ```bash
   utils/build-docker-builder.sh
   ```

3. Build the ARM64 executable:

   ```bash
   utils/build-beatled-server.sh
   ```

4. Deploy to your Raspberry Pi:

   ```bash
   utils/copy-tar-files.sh ${RPI_USERNAME} ${RPI_HOST}
   ```

The deployment script copies the server binary, built client, and scripts, then restarts the systemd service on the Pi.

## Dependencies

Managed via vcpkg (pinned to a specific commit for reproducibility):

| Library | Purpose |
|---------|---------|
| asio | Async networking |
| restinio | HTTP/HTTPS server |
| nlohmann-json | JSON parsing |
| spdlog / fmt | Logging and formatting |
| portaudio | Audio capture |
| fftw3 | FFT for beat detection |
| openssl | TLS support |
| catch2 | Unit testing |
