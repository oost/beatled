---
title: Beatled Server
layout: default
parent: Components
nav_order: 1
---

# Beatled Server

Source code: [github.com/oost/beatled](https://github.com/oost/beatled) (`server/`)

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
scripts/beatled.sh server --start-http

# With UDP server for Pico devices
scripts/beatled.sh server --start-http --start-udp

# With CORS for the Vite dev server
scripts/beatled.sh server --start-http --cors-origin "https://localhost:5173"

# With Bearer token authentication
scripts/beatled.sh server --start-http --api-token "secret"

# Run server tests
scripts/beatled.sh test server
```

The server is built with CMake and uses vcpkg for dependency management. The first build will configure CMake and install dependencies automatically.

### Prerequisites

- **macOS**: `brew install pkg-config cmake libtool automake autoconf autoconf-archive`
- **vcpkg**: Must be installed and `VCPKG_ROOT` set in your environment
- A **USB microphone** for audio capture (or use a virtual audio device for testing)

## Custom Test Domain

The default development domain is `beatled.test`. To use it locally, add an entry to `/etc/hosts`:

```bash
sudo sh -c 'echo "127.0.0.1 beatled.test" >> /etc/hosts'
```

Or open `/etc/hosts` in an editor and add the line manually:

```
127.0.0.1 beatled.test
```

On a Raspberry Pi, point the domain to the Pi's IP instead:

```
192.168.1.100 beatled.test
```

The `.test` TLD is [reserved by IANA](https://datatracker.ietf.org/doc/html/rfc6761) for testing and will never resolve publicly, making it safe for local development.
{: .note }

## TLS Certificates

The server requires TLS certificates for HTTPS. Generate self-signed certificates for local development:

```bash
# Default domain (beatled.test)
scripts/beatled.sh certs

# Or specify one or more domains
scripts/beatled.sh certs beatled.test localhost 127.0.0.1
```

This creates certificates in `server/certs/` using [mkcert](https://github.com/FiloSotto/mkcert). The server expects `cert.pem`, `key.pem`, and `dh_param.pem`.

For iOS Simulator, install the mkcert root CA so the app trusts self-signed certs:

```bash
xcrun simctl keychain booted add-root-cert "$(mkcert -CAROOT)/rootCA.pem"
```

## API Authentication

When started with `--api-token`, the server requires a Bearer token for all API requests:

```
Authorization: Bearer <token>
```

## Cross-Compilation and Deployment

Cross-compile for Raspberry Pi (ARM64) using Docker:

```bash
git submodule update --init --recursive   # first time only
scripts/beatled.sh build rpi
scripts/beatled.sh deploy <username> <host>
```

See the [Deployment](deployment.html) page for the full guide covering certificates, service management, and troubleshooting.

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
