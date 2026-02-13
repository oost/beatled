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
| `beatled.sh server --start-http --cors-origin "https://localhost:5173"` | Run with CORS for the Vite dev server |
| `beatled.sh server --start-http --api-token "secret"` | Run with Bearer token authentication |
| `beatled.sh client` | Start the Vite dev server (proxies `/api` to the beat server) |
| `beatled.sh client-build` | Production build of the React client |
| `beatled.sh pico` | Build and run the Pico firmware locally (posix port) |
| `beatled.sh test all` | Run all tests (server + client + pico) |
| `beatled.sh test server` | Run server tests only |
| `beatled.sh build all` | Build everything without running |
| `beatled.sh certs localhost` | Generate self-signed TLS certificates |

The server is configured with CMake on first run. Subsequent builds are incremental.

For local development with hot-reload, run the server and client in separate terminals:

```
# Terminal 1 - beat server
utils/beatled.sh server --start-http --cors-origin "https://localhost:5173"

# Terminal 2 - Vite dev server with HMR
utils/beatled.sh client
```

## Cross Compilation

In this step, we cross-compile the various server for the Raspberry Pi. We also build the React front and package it in a tarball.

1. Download submodule dependencies.

   ```
   git sumbodule update
   ```

2. Build builder image:

   ```
   utils/build-docker-builder.sh
   ```

3. Build Raspberry Pi executable:

   ```
   utils/build-beatled-server.sh
   ```

4. Copy your files to the raspberry pi:

   ```
   utils/copy-tar-files.sh ${RPI_USERNAME} ${RPI_HOST}
   ```

## Requirements

- On MacOS
  - `brew install pkg-config cmake libtool automake autoconf autoconf-archive`
