---
title: Deployment
layout: default
nav_order: 3
---

# Deploying to Raspberry Pi

This guide covers cross-compiling the server for ARM64 and deploying it to a Raspberry Pi over SSH.

## Prerequisites

On your **development machine** (macOS):

- Docker (for cross-compilation)
- [mkcert](https://github.com/FiloSotto/mkcert) for TLS certificates: `brew install mkcert && mkcert -install`
- SSH access to the Raspberry Pi

On the **Raspberry Pi**:

- Debian/Raspbian (64-bit ARM)
- `envsubst` (part of `gettext`): `sudo apt install gettext-base`
- A USB microphone for audio capture

## Quick Start

```bash
# 1. Generate TLS certificates for the Pi's hostname
scripts/beatled.sh certs <pi-hostname>

# 2. Cross-compile the server binary for ARM64
scripts/beatled.sh build rpi

# 3. Deploy to the Pi
scripts/beatled.sh deploy <username> <pi-hostname>
```

The deploy command packages the server binary, web client, and deploy scripts into a tarball, copies it to the Pi, extracts it, and starts (or restarts) the systemd service.

## Step by Step

### 1. Generate Certificates

Certificates are generated on your dev machine using mkcert, then copied to the Pi during deployment. Generate them for the Pi's hostname (and any other names you want to use):

```bash
# For a hostname
scripts/beatled.sh certs beatled.local

# For multiple names
scripts/beatled.sh certs beatled.local 192.168.1.100
```

This creates `cert.pem`, `key.pem`, and `dh_param.pem` in `server/certs/`. The deploy command copies these to `~/certs/` on the Pi if they don't already exist there.

Certificates only need to be generated once per hostname. Subsequent deploys will skip the copy if `~/certs/cert.pem` already exists on the Pi.
{: .note }

### 2. Cross-Compile

Build the ARM64 server binary using Docker:

```bash
# Initialize submodules (first time only)
git submodule update --init --recursive

# Build
scripts/beatled.sh build rpi
```

This builds a Docker image with the ARM64 cross-compilation toolchain and compiles the server. The output is written to `./out/` and includes:

- `server/beat_server` -- the server binary
- `client/` -- the built React web client
- `scripts/deploy/` -- deployment and service management scripts

### 3. Deploy

```bash
scripts/beatled.sh deploy <username> <host>
```

The deploy command:

1. Copies certificates to the Pi (if not already present)
2. Packages `./out/` into a tarball
3. Transfers it via SCP to the Pi
4. Backs up the existing installation (if any)
5. Extracts the new version to `~/beat-server/`
6. Installs or restarts the systemd service
7. Runs a health check against `https://<host>:8443/api/health`

If the service fails to start, the previous version is automatically restored from the backup.

## Service Management

The server runs as a systemd service called `beat-server.service`. On the Pi:

```bash
# Check status
systemctl status beat-server.service

# View logs (follow)
~/beat-server/scripts/deploy/tail-logs.sh
# or
journalctl -u beat-server.service -f

# Restart
sudo systemctl restart beat-server.service

# Stop
sudo systemctl stop beat-server.service
```

The service is enabled on install and will start automatically on boot.

### Service Configuration

The systemd service file is generated from a template at install time. The server listens on:

- **HTTPS**: `0.0.0.0:8443` (API and web client)
- **UDP**: `0.0.0.0:9090` (device communication)

The service runs as the deploy user (not root) with these security restrictions:

- `NoNewPrivileges` -- cannot gain additional privileges
- `ProtectSystem=strict` -- filesystem is read-only except for the app directory
- `ProtectHome=read-only` -- home directory is read-only (certificates are only read)

### Running Without systemd

To start the server directly (useful for debugging):

```bash
~/beat-server/scripts/deploy/start-server.sh
```

## Troubleshooting

**Deploy fails with "No certificates found locally"**
: Run `scripts/beatled.sh certs <hostname>` on your dev machine first.

**Service fails to start -- permission denied**
: The service runs as your deploy user. Ensure the user has read access to `~/certs/` and read/write access to `~/beat-server/`.

**Health check fails after deploy**
: The server may still be starting up. Check the logs with `journalctl -u beat-server.service -f`. Common causes: missing certificates, port already in use, or no audio device connected.

**Browser shows certificate warning**
: Certificates are locally-trusted via mkcert. Install the mkcert root CA on the device accessing the dashboard. On macOS, `mkcert -install` handles this. Safari does not accept mkcert certificates -- use Chrome instead.

## Deploy Scripts Reference

All scripts live in `scripts/deploy/` and are copied to the Pi during deployment.

| Script | Purpose |
|--------|---------|
| `reload-service.sh` | Restarts the service, or installs it on first deploy |
| `install-service.sh` | Generates the systemd service file from the template and enables it |
| `create-certs.sh` | Generates TLS certificates using mkcert (run on dev machine) |
| `start-server.sh` | Starts the server directly without systemd |
| `tail-logs.sh` | Follows the systemd service logs |
