# Beatled

Real-time beat-synchronized LED control using a Raspberry Pi server, Pico W / ESP32 microcontrollers, and a React web dashboard.

## Prerequisites

- A Raspberry Pi 4 or 5 (server)
- One or more Pico W or ESP32 devices with WS2812 LED strips
- macOS or Linux for local development

## Repos

- **This repo** — C++ beat server, React frontend, iOS app, and project documentation
- [**beatled-pico**](https://github.com/oost/beatled-pico) — Embedded C firmware for Pico W and ESP32 (5 ports: `pico`, `pico_freertos`, `posix`, `posix_freertos`, `esp32`)
- [**beatled-beat-tracker**](https://github.com/oost/beatled-beat-tracker) — Fork of [BTrack](https://github.com/adamstark/BTrack) for live beat tracking

## Documentation

Full documentation: [oost.github.io/beatled](https://oost.github.io/beatled/)

## Local Development

A single script handles building and running all components locally:

```
scripts/beatled.sh <command> [options]
```

| Command | Description |
|---------|-------------|
| `beatled.sh server --start-http` | Build and start the HTTPS server with the built client |
| `beatled.sh server --start-http --start-udp` | Also start the UDP server for Pico devices |
| `beatled.sh server --start-http --start-udp --start-broadcast -c 127.0.0.1` | Also broadcast tempo to local Pico processes (see note below) |
| `beatled.sh server --start-http --cors-origin "https://localhost:5173"` | Run with CORS for the Vite dev server |
| `beatled.sh server --start-http --api-token "secret"` | Run with Bearer token authentication |
| `beatled.sh client` | Start the Vite dev server (proxies `/api` to the beat server) |
| `beatled.sh client-build` | Production build of the React client |
| `beatled.sh pico` | Build and run the Pico firmware locally (posix port) |
| `beatled.sh pico-freertos` | Build and run the Pico firmware locally (posix_freertos port) |
| `beatled.sh test all` | Run all tests (server + client + pico) |
| `beatled.sh test server` | Run server tests only |
| `beatled.sh build all` | Build everything without running |
| `beatled.sh certs localhost` | Generate self-signed TLS certificates |

**Broadcasting locally:** The default broadcast address (`192.168.86.255`) targets the LAN and won't reach processes on localhost. When running the Pico firmware locally with `beatled.sh pico`, use `-c 127.0.0.1` to send unicast tempo messages to the local Pico process instead.

The server is configured with CMake on first run. Subsequent builds are incremental.

For local development with hot-reload, run the server and client in separate terminals:

```
# Terminal 1 - beat server
scripts/beatled.sh server --start-http --cors-origin "https://localhost:5173"

# Terminal 2 - Vite dev server with HMR
scripts/beatled.sh client
```

## Cross Compilation

In this step, we cross-compile the server for the Raspberry Pi and build the React frontend, packaged in a tarball.

1. Initialize and download submodule dependencies:

   ```
   git submodule update --init --recursive
   ```

2. Build the Docker builder image and the ARM64 server executable:

   ```
   scripts/beatled.sh build rpi
   ```

3. Deploy to the Raspberry Pi:

   ```
   scripts/beatled.sh deploy ${RPI_USERNAME} ${RPI_HOST}
   ```

## Requirements

- On MacOS
  - `brew install pkg-config cmake libtool automake autoconf autoconf-archive`
