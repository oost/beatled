# Beatled

## Prerequisites

- A Raspberry Pi 4 or 5
- One or more Raspberry Pico
- A Unix/Linux based machine (not strictly necessary but it may be easier to use your laptop to cross compile the code)

## Repos

- This repo contains the C++ server code and the React frontend.
- The [Beatled Pico](https://github.com/oost/beatled-pico) repo contains the Pico C code to be flashed on your Pico devices.
- The [Beatled Beat Tracker](https://github.com/oost/beatled-beat-tracker) repo contains a fork from [BTrack](https://github.com/adamstark/BTrack) that is used for live beat tracking.

## Local Development

A single script handles building and running all components locally:

```
utils/beatled.sh <command> [options]
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
| `beatled.sh test all` | Run all tests (server + client + pico) |
| `beatled.sh test server` | Run server tests only |
| `beatled.sh build all` | Build everything without running |
| `beatled.sh certs localhost` | Generate self-signed TLS certificates |

**Broadcasting locally:** The default broadcast address (`192.168.86.255`) targets the LAN and won't reach processes on localhost. When running the Pico firmware locally with `beatled.sh pico`, use `-c 127.0.0.1` to send unicast tempo messages to the local Pico process instead.

The server is configured with CMake on first run. Subsequent builds are incremental.

For local development with hot-reload, run the server and client in separate terminals:

```
# Terminal 1 - beat server
utils/beatled.sh server --start-http --cors-origin "https://localhost:5173"

# Terminal 2 - Vite dev server with HMR
utils/beatled.sh client
```

## Cross Compilation

In this step, we cross-compile the server for the Raspberry Pi and build the React frontend, packaged in a tarball.

1. Initialize and download submodule dependencies:

   ```
   git submodule update --init --recursive
   ```

2. Build the Docker builder image and the ARM64 server executable:

   ```
   utils/beatled.sh build rpi
   ```

3. Deploy to the Raspberry Pi:

   ```
   utils/beatled.sh deploy ${RPI_USERNAME} ${RPI_HOST}
   ```

## Requirements

- On MacOS
  - `brew install pkg-config cmake libtool automake autoconf autoconf-archive`
