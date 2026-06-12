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
- For the Wi-Fi roaming / fallback-hotspot features: NetworkManager (default on
  Bookworm) with the connection profiles created once — see
  [Wi-Fi roaming, fallback hotspot, and mDNS alias](#wi-fi-roaming-fallback-hotspot-and-mdns-alias).
  A plain wired or single-network deploy needs none of this.

## Quick Start

```bash
# 1. Generate TLS certificates for the Pi's hostname
./beatled.sh certs <pi-hostname>

# 2. Cross-compile the server binary for ARM64
./beatled.sh build rpi

# 3. Deploy to the Pi
./beatled.sh server deploy <username> <pi-hostname>
```

The deploy command packages the server binary, web client, and deploy scripts into a tarball, copies it to the Pi, extracts it, and starts (or restarts) the systemd service.

## Step by Step

### 1. Generate Certificates

Certificates are generated on your dev machine using mkcert, then copied to the Pi during deployment. Generate them for the Pi's hostname (and any other names you want to use):

```bash
# For a hostname
./beatled.sh certs beatled.local

# For multiple names
./beatled.sh certs beatled.local 192.168.1.100
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
./beatled.sh build rpi
```

This builds a Docker image with the ARM64 cross-compilation toolchain and compiles the server. The output is written to `./out/` and includes:

- `server/beat_server` -- the server binary
- `client/` -- the built React web client
- `scripts/deploy/` -- deployment and service management scripts

### 3. Deploy

```bash
./beatled.sh server deploy <username> <host>
```

The deploy command:

1. Copies certificates to the Pi (if not already present)
2. Bundles `controller/.env.wifi` (if present) and packages `./out/` into a tarball
3. Transfers it via SCP to the Pi
4. Backs up the existing installation (if any)
5. Extracts the new version to `~/beat-server/`
6. Installs/restarts the services (`beat-server`, plus `wifi-fallback` and
   `avahi-alias` when configured), the polkit rule, and the NetworkManager
   dispatcher — all idempotent, re-run on every deploy
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

### Broadcasting Tempo to Controllers

Per-beat tempo broadcasts to the Pico / ESP32 controllers are an *optional*
service. They're disabled by default — the systemd unit (and the
`./beatled.sh server start` shorthand for local development) need
the `--start-broadcast` flag for the broadcaster to come up:

```sh
beat_server ... --start-udp --start-broadcast
```

`--broadcast-mode` controls how each NEXT_BEAT reaches the fleet:

| Mode                | Destination                                    | Notes                                                                                                            |
| ------------------- | ---------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| `unicast` (default) | each registered client's last-known endpoint   | per-client one-way-delay compensation applied; preferred for ≤10 controllers                                     |
| `subnet`            | `--broadcasting-address` (e.g. `192.168.1.255`)| one packet per beat; AP-friendly but no per-client compensation                                                  |
| `limited`           | `255.255.255.255`                              | one packet per beat; **frequently dropped by consumer Wi-Fi APs** — use only if you've verified yours forwards it |

The unicast default also drives the `PROGRAM` push (server-side state
change → instant fan-out + 1 Hz refresh) and is what the per-client OWD
correction in protocol v2 relies on. See
[Controller Sync](controller-sync.html) for the full handshake.

### Running Without systemd

To start the server directly (useful for debugging):

```bash
~/beat-server/scripts/deploy/start-server.sh
```

## Wi-Fi roaming, fallback hotspot, and mDNS alias

Beyond the server itself, the deploy installs three helper services so a Pi can
run untethered:

| Service | What it does |
|---------|--------------|
| `wifi-fallback` | On boot, brings up each upstream Wi-Fi in turn; if none connect, starts the Pi's own hotspot so the rig still works off-grid. Installed *enabled but not started* during deploy — starting it would drop the deploy's own Wi-Fi link. |
| `avahi-alias` | Publishes an extra mDNS name (`MDNS_ALIAS`, default `beatled.local`) so the Pi answers to it in addition to `<hostname>.local`. A NetworkManager dispatcher restarts it on every IP change so the name never goes stale. |
| `ap-mode` (on demand) | `scripts/deploy/ap-mode.sh` and the `POST /api/ap` endpoint toggle the radio between client and hotspot mode, with an optional auto-revert timer so a bad switch can't strand a headless Pi. |

All of these read the **single shared Wi-Fi config**, `controller/.env.wifi` —
the same file the controller firmware builds use. See
[the CLI reference](cli.html) for every field. The deploy bundles `.env.wifi`
into the tarball so the host services can read it.

The Pi has a single radio, so client and AP modes are mutually exclusive:
switching to the hotspot tears down upstream Wi-Fi (and any SSH/HTTP session
riding it). Clients then rejoin the hotspot SSID and reach the server at
`https://192.168.4.1:8443/`.

### NetworkManager profiles (required, one-time)

`wifi-fallback` and `ap-mode` don't store credentials — they **activate
NetworkManager connection profiles by name**. Create these on the Pi once,
before the first deploy (or before relying on the fallback). The names must
match `controller/.env.wifi`:

- one profile per upstream `WIFI_SSID[_2..4]`, named after the SSID;
- one AP-mode profile named `HOTSPOT_CON` (default `beatled-hotspot`) that
  broadcasts `HOTSPOT_SSID`.

> Bookworm-based Raspberry Pi OS uses NetworkManager by default. Confirm with
> `nmcli general status`; if it isn't the active backend these steps don't apply.
{: .note }

**Upstream Wi-Fi** — connecting once saves a profile named after the SSID,
which is exactly what `wifi-fallback` looks for:

```bash
nmcli device wifi connect "Livebox-98D4" password "<wifi-password>"
```

Repeat for each additional `WIFI_SSID_2..4` you configured.

**Fallback hotspot** — an access-point profile. The connection **name** must
equal `HOTSPOT_CON`, the broadcast **SSID** must equal `HOTSPOT_SSID`, and the
password must match `HOTSPOT_PASSWORD` in `.env.wifi` (so the controllers can
join it):

```bash
nmcli connection add type wifi ifname wlan0 con-name beatled-hotspot ssid "Beatled"
nmcli connection modify beatled-hotspot \
  connection.autoconnect no \
  802-11-wireless.mode ap \
  ipv4.method shared \
  ipv4.addresses 192.168.4.1/24 \
  802-11-wireless-security.key-mgmt wpa-psk \
  802-11-wireless-security.psk "<hotspot-password>"
```

- `autoconnect no` keeps the Pi on upstream Wi-Fi by default; the hotspot only
  comes up when `wifi-fallback` (no upstream reachable) or `ap-mode on`
  activates it.
- `ipv4.method shared` makes the Pi the gateway and runs DHCP for joined
  clients; `ipv4.addresses 192.168.4.1/24` fixes that gateway at `192.168.4.1`,
  so in AP mode the server is always at `https://192.168.4.1:8443/`.

Verify the profiles exist (names must match `.env.wifi`):

```bash
nmcli -t -f NAME,TYPE connection show
```

### Privileges (handled by the deploy)

Activating these connections from a headless, session-less service would
normally be denied by polkit. The deploy installs a scoped polkit rule
(`/etc/polkit-1/rules.d/50-beatled-network.rules`) granting the service user
the `network-control` and `wifi.share.protected`/`open` actions — the latter
are required because the hotspot uses `ipv4.method=shared`; without them AP
activation fails with *"Not authorized to share connections via wifi."* No
manual setup is needed.

### Toggling AP mode

```bash
# On the Pi (or via POST /api/ap from the web/iOS clients):
~/beat-server/scripts/deploy/ap-mode.sh on 10   # hotspot, auto-revert in 10 min
~/beat-server/scripts/deploy/ap-mode.sh off     # back to upstream Wi-Fi
~/beat-server/scripts/deploy/ap-mode.sh status  # "on" or "off"
```

The optional `<minutes>` argument schedules an automatic switch back to Wi-Fi —
a safety net so a bad switch can't strand the Pi headless.

## Troubleshooting

**Deploy fails with "No certificates found locally"**
: Run `./beatled.sh certs <hostname>` on your dev machine first.

**Service fails to start -- permission denied**
: The service runs as your deploy user. Ensure the user has read access to `~/certs/` and read/write access to `~/beat-server/`.

**Health check fails after deploy**
: The server may still be starting up. Check the logs with `journalctl -u beat-server.service -f`. Common causes: missing certificates, port already in use, or no audio device connected.

**Browser shows certificate warning**
: Certificates are locally-trusted via mkcert. Install the mkcert root CA on the device accessing the dashboard. On macOS, `mkcert -install` handles this. Safari does not accept mkcert certificates -- use Chrome instead.

**AP mode fails: "Not authorized to share connections via wifi"**
: The polkit rule isn't installed (or predates the `wifi.share` grant). Re-run a deploy, or copy `scripts/deploy/beatled-network.rules.template` to `/etc/polkit-1/rules.d/50-beatled-network.rules` (substituting your username). polkitd reloads `rules.d` automatically.

**`wifi-fallback` shows `failed` / the hotspot won't start**
: The required NetworkManager profiles don't exist or don't match `controller/.env.wifi`. Check `nmcli -t -f NAME connection show` — there must be a profile per `WIFI_SSID[_2..4]` and one named `HOTSPOT_CON`. See [NetworkManager profiles](#networkmanager-profiles-required-one-time).

**`beatled.local` resolves to a stale IP after switching to the hotspot**
: The mDNS alias should re-publish automatically via the NetworkManager dispatcher. If it's stale, restart it: `sudo systemctl restart avahi-alias.service` (and flush the client's cache, e.g. `sudo killall -HUP mDNSResponder` on macOS). The host's own `<hostname>.local` always tracks the current IP.

## Deploy Scripts Reference

All scripts live in `scripts/deploy/` and are copied to the Pi during deployment.

| Script | Purpose |
|--------|---------|
| `reload-service.sh` | Entry point each deploy runs; (re)installs and restarts the services via `install-service.sh` |
| `install-service.sh` | Generates each systemd unit from its template, installs the polkit rule and the NetworkManager dispatcher, and enables/starts the services |
| `create-certs.sh` | Generates TLS certificates using mkcert (run on dev machine) |
| `start-server.sh` | Starts the server directly without systemd |
| `tail-logs.sh` | Follows the `beat-server` service logs |
| `wifi-fallback.sh` | Run by the `wifi-fallback` unit at boot: try each upstream Wi-Fi profile in order, else start the hotspot |
| `ap-mode.sh` | Toggle client ⇄ hotspot mode on demand (`on [minutes]` / `off` / `status`); also driven by `POST /api/ap` |
| `avahi-alias.sh` | Run by the `avahi-alias` unit: publish the extra mDNS name (`MDNS_ALIAS`) |
| `avahi-alias-dispatcher.sh` | Installed as a NetworkManager dispatcher; restarts `avahi-alias` on every IP change so the name tracks the current address |

Templates and the polkit rule alongside them — `*.template.service`,
`beatled-network.rules.template` — are rendered with `envsubst` at install time.
