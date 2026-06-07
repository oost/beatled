---
title: CLI Reference
layout: default
nav_order: 4
---

# `./beatled.sh` — CLI Reference

`beatled.sh` is the single entry point for building, running, testing,
flashing, and deploying every piece of the system. It wraps cmake,
ninja, npm, xcodebuild, the Pico SDK, esptool, mkcert, Docker, and ssh
behind a small group/subcommand grammar so day-to-day work doesn't need
a cheat sheet for each underlying tool.

```sh
./beatled.sh <group> <subcommand> [action] [options]
```

Every command level accepts `-h` / `--help` (or `help`). Run e.g.
`./beatled.sh server -h` to see the same content as below
without leaving the terminal.

## Quick orientation

| Group        | What it touches                                                                                                                              |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------------------- |
| `client`     | React frontend (`client/`) and the SwiftUI app shared by iOS + macOS (`ios/`)                                                                |
| `controller` | Embedded firmware at `controller/` (5 ports: `pico`, `pico_freertos`, `posix`, `posix_freertos`, `esp32`)                                     |
| `server`     | C++ beat server (`server/`)                                                                                                                  |
| `test`       | Catch2 / Vitest test runs across the components                                                                                              |
| `build`      | Cross-component builds without running anything                                                                                              |
| `clean`      | Wipe build artefacts                                                                                                                         |
| `docs`       | Serve this Jekyll site locally                                                                                                               |
| `certs`      | Generate locally-trusted dev certificates                                                                                                    |

## `client`

```sh
./beatled.sh client <surface> <action>
```

| Surface | Action  | Behaviour |
|---------|---------|-----------|
| `react` | `dev`   | Vite dev server with HMR; proxies `/api` to `https://127.0.0.1:8443` |
| `react` | `build` | Production build → `client/dist/` |
| `ios`   | `build` | `xcodebuild` for the iOS Simulator |
| `ios`   | `sim`   | Build + boot the iOS Simulator and launch the app |
| `macos` | `build` | `xcodebuild` `Beatled.app` for macOS |
| `macos` | `start` | Build and launch the macOS app |

The iOS and macOS targets share a single Xcode project at
`ios/Beatled.xcodeproj` and the same Swift sources (the `ios/`
directory name is historical). `react dev` needs the server running —
start it with `server start --start-http` in another terminal.

## `controller`

```sh
./beatled.sh controller <port> <action>
```

| Port             | Actions                          | Output                                |
| ---------------- | -------------------------------- | ------------------------------------- |
| `pico`           | `build`, `flash`                 | `controller/build-pico/.../pico_w_beatled.uf2` |
| `pico-freertos`  | `build`, `flash`                 | `controller/build-pico-freertos/.../pico_w_beatled.uf2` |
| `posix`          | `build`                          | Native simulator (Metal LED renderer on macOS); execs the binary |
| `freertos-sim`   | `build`                          | Native simulator + FreeRTOS POSIX kernel; execs the binary |
| `esp32-freertos` | `build`, `flash`, `monitor`      | ESP-IDF binary; `flash` chains esptool + `idf.py monitor` |

The `flash` action on the Pico ports copies the `.uf2` to
`/Volumes/RPI-RP2/` after the build, so the board needs to be in
BOOTSEL mode (hold the button while plugging USB in). The ESP32
`flash` uses the existing serial connection from `ESP32_PORT`.

### Firmware config (`.env.pico` / `.env.esp32`)

The hardware ports bake several values into the firmware at build time
— Wi-Fi credentials, the server's hostname or IP, the pixel count, and
the GPIO pin. They live in `.env.*` files inside `controller/`:

```sh
cp controller/.env.pico.template  controller/.env.pico
cp controller/.env.esp32.template controller/.env.esp32
$EDITOR controller/.env.pico
```

The relevant subcommand sources the file automatically. Both
`.env.*` shapes are gitignored at the repo root.

## `server`

```sh
./beatled.sh server <subcommand> [options]
```

### Subcommands

| Subcommand             | Behaviour |
| ---------------------- | --------- |
| `build`                | cmake + ninja under `server/build/` (vcpkg toolchain) |
| `start [opts]`         | Build if needed, then run `beat_server` in the foreground |
| `deploy <user> <host>` | Build → tarball → scp → systemd reload over SSH; rollback on failure |

### Service flags

`start` builds nothing on its own — service flags pick which sub-services
the binary brings up.

| Flag                 | Default | Notes |
| -------------------- | ------- | ----- |
| `--start-http`       | off     | HTTPS API + static client on `--http-port` (default 8443) |
| `--start-udp`        | off     | UDP server on `--udp-port` (default 9090) — HELLO / TIME / TEMPO requests |
| `--start-broadcast`  | off     | Per-beat tempo dispatcher + PROGRAM push (see `--broadcast-mode`) |

### Server config

| Flag                                                | Default                                | Description |
| --------------------------------------------------- | -------------------------------------- | ----------- |
| `-a ADDRESS`                                        | `localhost` (script overrides to `0.0.0.0`) | Listen address |
| `-p, --http-port PORT`                              | `8443`                                 | HTTP(S) port |
| `-u, --udp-port PORT`                               | `9090`                                 | UDP request port |
| `-n, --thread-pool-size N`                          | `2`                                    | asio worker threads |
| `-r, --root-dir PATH`                               | `client/dist`                          | Static-file root |
| `--certs-dir PATH`                                  | `server/certs`                         | TLS cert / key / DH parameters |
| `--no-tls`                                          | off                                    | Serve plain HTTP — development only (emits `SPDLOG_WARN`) |
| `--cors-origin URL`                                 | disabled                               | Single-origin CORS allowance |
| `--api-token TOKEN`                                 | disabled                               | Require `Authorization: Bearer <token>` on state-changing calls |
| `--verbose`                                         | off                                    | DEBUG-level logging (per-beat lines etc.) |

### Broadcaster config (only with `--start-broadcast`)

| Flag                                  | Default          | Description |
| ------------------------------------- | ---------------- | ----------- |
| `--broadcast-mode {unicast,subnet,limited}` | `unicast`  | See below |
| `-c, --m_broadcasting-address ADDR`   | `255.255.255.255`| Destination for `subnet` / `limited` modes |
| `-b, --broadcasting-port PORT`        | `8765`           | UDP destination port |

| Mode      | Destination                              | Notes |
| --------- | ---------------------------------------- | ----- |
| `unicast` | each registered client's last-known endpoint | Default — per-client OWD compensation, best for ≤10 controllers on Wi-Fi |
| `subnet`  | `--m_broadcasting-address` (e.g. `192.168.1.255`) | One packet per beat, no per-client compensation |
| `limited` | `255.255.255.255`                        | Frequently dropped by consumer Wi-Fi APs |

### Deployment

`server deploy <user> <host>` tarballs the cross-compiled aarch64 build
(`build rpi` artefacts), copies it to the Pi via `scp`, installs or
reloads the systemd unit, and runs an `/api/health` probe; on failure
it restores the previous backup. See the
[Deployment runbook](deployment.html).

## `test`, `build`, `clean`

```sh
./beatled.sh test  [server|client|pico|all]
./beatled.sh build [server|client|pico|pico-freertos|rpi|all]
./beatled.sh clean [server|client|pico|pico-freertos|esp32|all]
```

- `test all` is the default; it runs the server's Catch2 binaries
  (including `test_api_gates` + `test_state_manager`), then `npm test`
  in `client/`, then the POSIX-port firmware tests under
  `controller/build/tests/posix/`.
- `build all` is the default; it does the server, client, pico, and
  pico-freertos targets. The `rpi` target is opt-in (it requires
  Docker for the aarch64 cross-compile).
- `clean all` wipes every component's build directory. Pico / FreeRTOS
  variants each have two build directories (POSIX simulator vs.
  hardware .uf2); both are cleaned.

## `docs`

```sh
./beatled.sh docs [-- jekyll options]
```

Serves the Jekyll site at <http://127.0.0.1:4000/beatled/>. Any
options after `--` are passed through to `bundle exec jekyll serve`;
common examples: `--livereload`, `--port 4001`.

## `certs`

```sh
./beatled.sh certs [domain ...]
```

Generates a locally-trusted TLS cert (cert + key + DH parameters)
under `server/certs/` using `mkcert`. The cert chains to the local
mkcert root CA, which the server iOS + Mac clients also need installed
(`mkcert -install`).

Examples:

```sh
./beatled.sh certs                              # default: beatled.test
./beatled.sh certs beatled.local
./beatled.sh certs beatled.local 192.168.1.100  # multi-SAN
```

## Environment variables

Most callers don't need to set these — the defaults are wired for the
in-tree layout — but they're useful when the standard layout doesn't
match (sibling clones, alternate vcpkg root, different ESP32 board).

| Variable              | Default                              | Used by |
| --------------------- | ------------------------------------ | ------- |
| `VCPKG_DIR`           | `~/coding/external/vcpkg`            | Server build |
| `PICO_DIR`            | `<repo>/controller`                  | All controller subcommands |
| `WIFI_SSID`           | *(required for hardware ports)*      | Pico W / ESP32 build (baked into `.uf2`) |
| `WIFI_PASSWORD`       | *(required for hardware ports)*      | Pico W / ESP32 build |
| `BEATLED_SERVER_NAME` | *(required for hardware ports)*      | Pico W / ESP32 build (hostname or IP) |
| `NUM_PIXELS`          | *(required)*                         | Pixel count baked into firmware |
| `WS2812_PIN`          | `0`                                  | GPIO data pin |
| `ESP32_TARGET`        | `esp32s3`                            | `idf.py set-target` value |
| `ESP32_PORT`          | `/dev/cu.usbmodem*`                  | esptool / monitor serial device |
| `BEATLED_API_TOKEN`   | *(none)*                             | Fallback API token when `--api-token` is set on the CLI |

`WIFI_SSID`, `WIFI_PASSWORD`, `BEATLED_SERVER_NAME`, `NUM_PIXELS`, and
`WS2812_PIN` are typically set via `controller/.env.pico` and friends
rather than exported manually. The build wrapper sources the right
`.env` file for each port automatically.

## Common workflows

### First-time setup (dev machine, macOS)

```sh
git clone https://github.com/oost/beatled.git
cd beatled
git submodule update --init --recursive
scripts/git-hooks/install.sh              # pre-commit hooks (clang-format, shellcheck)
./beatled.sh certs                  # local TLS for beatled.test / mkcert root
./beatled.sh server start --start-http --start-udp --start-broadcast
# in another terminal:
./beatled.sh client react dev
```

### Iterate on the firmware (POSIX simulator, no hardware)

```sh
cp controller/.env.pico.template controller/.env.pico
$EDITOR controller/.env.pico                # WIFI_SSID, BEATLED_SERVER_NAME, NUM_PIXELS
./beatled.sh controller posix build   # builds + execs the simulator
./beatled.sh test pico                # POSIX-port unit + integration tests
```

### Flash a real Pico W

```sh
# Once: fill .env.pico (WIFI_SSID / WIFI_PASSWORD / BEATLED_SERVER_NAME)
# Hold BOOTSEL while plugging USB so /Volumes/RPI-RP2/ mounts.
./beatled.sh controller pico flash
# or the FreeRTOS port:
./beatled.sh controller pico-freertos flash
```

### Deploy to a Raspberry Pi

```sh
./beatled.sh certs beatled.local           # first time only
./beatled.sh build rpi                     # Docker cross-compile to out/
./beatled.sh server deploy pi beatled.local
```

The deploy script copies certs to the Pi, scps the tarball, installs
or reloads `beat-server.service`, and probes `/api/health`. On failure
it restores the previous backup. See
[Deployment](deployment.html#step-by-step) for the long version.

### Cut a release commit

```sh
./beatled.sh test all                      # server + client + firmware POSIX
./beatled.sh build all                     # check every component still links
git status                                       # spot-check
git commit …
```

## Escape hatches

- **Skip the pre-commit hooks for a single commit**:
  `BEATLED_SKIP_HOOKS=1 git commit …`. Don't make a habit of it.
- **Point at a sibling firmware checkout**:
  `PICO_DIR=/path/to/other/firmware ./beatled.sh controller posix build`.
- **Pass through to the underlying Jekyll server**: anything after `--`
  reaches `jekyll serve` directly, e.g. `./beatled.sh docs --
  --livereload --port 4001`.
