---
title: Getting Started
layout: default
nav_order: 2
---

# Getting Started

This guide gets the server, web client, and Pico simulator running on your Mac for local development. No hardware required.

## Prerequisites

- **macOS** (Apple Silicon or Intel)
- **Homebrew**: [brew.sh](https://brew.sh)
- **vcpkg**: [vcpkg.io](https://vcpkg.io/en/getting-started), installed with `VCPKG_ROOT` set in your shell profile
- **Node.js** (for the web client)

Install the build tools:

```bash
brew install pkg-config cmake libtool automake autoconf autoconf-archive
```

## Clone

```bash
git clone https://github.com/oost/beatled.git
cd beatled
git submodule update --init --recursive
```

## Set Up the Test Domain

Add `beatled.test` to your `/etc/hosts` so it resolves to localhost:

```bash
sudo sh -c 'echo "127.0.0.1 beatled.test" >> /etc/hosts'
```

See [Beatled Server — Custom Test Domain](server.html#custom-test-domain) for more details.

## 1. Start the Server

Build and start the beat server with the HTTP API and UDP server for Pico devices:

```bash
scripts/beatled.sh server --start-http --start-udp --no-tls
```

The server listens on `localhost:8443` (HTTP) and `localhost:9090` (UDP). Add `--start-broadcast` to also broadcast beat timing.

## 2. Start the Web Client

In a second terminal, start the Vite dev server:

```bash
scripts/beatled.sh client
```

Open [https://localhost:5173](https://localhost:5173) in your browser. The dev server proxies API requests to the beat server.

## 3. Start the Pico Simulator

In a third terminal, build and run the POSIX port with the Metal LED visualizer:

```bash
scripts/beatled.sh pico
```

A window opens rendering a 3D LED ring. The simulator connects to `localhost`, registers with the server, and starts receiving beat timing.

## Running Tests

```bash
# All tests
scripts/beatled.sh test all

# Individual components
scripts/beatled.sh test server
scripts/beatled.sh test client
scripts/beatled.sh test pico
```

## Next Steps

- [Components](components.html) -- detailed build and configuration for each component
- [Architecture](architecture.html) -- system diagrams and synchronization sequence
- [Beatled Protocol](protocol.html) -- binary UDP wire format
- [HTTP API Reference](api.html) -- REST endpoints
