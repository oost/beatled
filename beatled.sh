#!/bin/bash

set -euo pipefail

# The script now lives at the repo root; PROJECT_DIR resolves to the
# directory the script file itself sits in. SCRIPTS_DIR is the
# now-helper-only scripts/ directory (deploy/, git-hooks/, …).
readonly PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPTS_DIR="$PROJECT_DIR/scripts"

readonly VCPKG_DIR="${VCPKG_DIR:-$HOME/coding/external/vcpkg}"
readonly SERVER_DIR="$PROJECT_DIR/server"
readonly SERVER_BUILD_DIR="$SERVER_DIR/build"
readonly CLIENT_DIR="$PROJECT_DIR/client"
# Controller firmware now lives in-tree at controller/. PICO_DIR retains
# its env-var override so a developer can point at a sibling clone for
# experiments, but the default no longer relies on a parallel checkout.
readonly PICO_DIR="${PICO_DIR:-$PROJECT_DIR/controller}"
readonly PICO_BUILD_DIR="$PICO_DIR/build"
readonly PICO_FREERTOS_BUILD_DIR="$PICO_DIR/build_posix_freertos"
readonly PICO_HW_BUILD_DIR="$PICO_DIR/build-pico"
readonly PICO_HW_FREERTOS_BUILD_DIR="$PICO_DIR/build-pico-freertos"

# --- Colors ---
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[0;33m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m'

info()  { echo -e "${CYAN}==> $1${NC}"; }
ok()    { echo -e "${GREEN}==> $1${NC}"; }
warn()  { echo -e "${YELLOW}==> $1${NC}"; }
error() { echo -e "${RED}==> $1${NC}" >&2; }

# --- Usage ---

usage() {
  cat <<EOF
Usage: $(basename "$0") <group> <subcommand> [action] [options]

Groups:
  client      React web app, iOS app, macOS app
  controller  LED firmware (Pico W, ESP32, POSIX simulator)
  server      Beat detection server

Utilities:
  test [component]    Run tests (server, client, pico, or all)
  build [component]   Build without running (server, client, pico, pico-freertos, rpi, or all)
  clean [component]   Clean build artifacts (server, client, pico, pico-freertos, esp32, or all)
  docs                Start the Jekyll docs site locally on http://127.0.0.1:4000
  certs [domain ...]  Generate locally-trusted development certificates (uses mkcert)
  help                Show this message; '-h' / '--help' work too at every level

Examples:
  $(basename "$0") client react dev                       # Vite dev server (proxies /api to 127.0.0.1:8443)
  $(basename "$0") client ios sim                         # build + boot the iOS Simulator
  $(basename "$0") controller posix build                 # firmware POSIX simulator (no hardware)
  $(basename "$0") controller pico flash                  # cross-build + copy .uf2 to a BOOTSEL Pico W
  $(basename "$0") controller esp32-freertos flash        # esptool flash + idf.py monitor
  $(basename "$0") server start --start-http --start-udp --start-broadcast
  $(basename "$0") server deploy pi beatled.local         # tarball + scp + systemd reload on the Pi

Environment variables (all optional, sensible defaults shown):
  VCPKG_DIR              ~/coding/external/vcpkg   vcpkg checkout used by the server build
  PICO_DIR               <repo>/controller         firmware tree (set only to point at a sibling clone)
  WIFI_SSID              (none — required for hw)  baked into the firmware .uf2 at build time
  WIFI_PASSWORD          (none — required for hw)  baked into the firmware .uf2 at build time
  BEATLED_SERVER_NAME    (none — required for hw)  hostname or IP the controllers connect to
  NUM_PIXELS             (none — required)         LEDs on the strip
  WS2812_PIN             0                         GPIO data pin for the WS2812 strip
  ESP32_TARGET           esp32s3                   idf.py set-target value
  ESP32_PORT             /dev/cu.usbmodem*         serial device for esptool / monitor
  BEATLED_API_TOKEN      (none)                    fallback API token for 'server start --api-token'

Most of those live in $PICO_DIR/.env.pico, .env.posix, or .env.esp32.
Copy the .template alongside, fill in your values, and the relevant
subcommand sources it automatically.

Full reference: https://oost.github.io/beatled/cli.html
EOF
  exit 1
}

usage_client() {
  cat <<EOF
Usage: $(basename "$0") client <surface> <action>

Surfaces & actions:
  react   build    Production build of the React frontend (output: client/dist/)
  react   dev      Vite dev server with HMR; proxies /api to https://127.0.0.1:8443
  ios     build    xcodebuild iOS Simulator binary
  ios     sim      Build + boot the iOS Simulator and launch the app
  macos   build    xcodebuild Beatled.app for macOS
  macos   start    Build and launch the macOS app

Notes:
  - The iOS / macOS targets share a single Xcode project at ios/Beatled.xcodeproj
    and use the same Swift sources (the ios/ folder name is historical).
  - 'react dev' requires the server to be running locally; start it with
    '$(basename "$0") server start --start-http'.
EOF
  exit 1
}

usage_client_react() {
  cat <<EOF
Usage: $(basename "$0") client react <action>

Actions:
  build   Build the React frontend for production (output: client/dist)
  dev     Start the Vite dev server (proxies API to https://127.0.0.1:8443)
EOF
  exit 1
}

usage_client_ios() {
  cat <<EOF
Usage: $(basename "$0") client ios <action>

Actions:
  build   Build the iOS app for the simulator
  sim     Build, boot simulator, install, and launch the app
EOF
  exit 1
}

usage_client_macos() {
  cat <<EOF
Usage: $(basename "$0") client macos <action>

Actions:
  build   Build the macOS app
  start   Build and launch the macOS app
EOF
  exit 1
}

usage_controller() {
  cat <<EOF
Usage: $(basename "$0") controller <port> <action>

Ports & actions:
  pico            build    Cross-build Pico W bare-metal .uf2
  pico            flash    Build + copy .uf2 to a BOOTSEL-mounted Pico W
  pico-freertos   build    Cross-build Pico W FreeRTOS SMP .uf2
  pico-freertos   flash    Build + copy .uf2 to a BOOTSEL-mounted Pico W
  posix           build    Native simulator (Metal LED renderer on macOS)
  freertos-sim    build    Native simulator + FreeRTOS POSIX kernel
  esp32-freertos  build    Cross-build ESP32 firmware (ESP-IDF)
  esp32-freertos  flash    Build + esptool flash + idf.py monitor
  esp32-freertos  monitor  Open the ESP32 serial monitor without re-flashing

Flashing notes:
  Pico W:  hold the BOOTSEL button while plugging the USB cable in so the
           board mounts as /Volumes/RPI-RP2/ before running 'pico flash'.
  ESP32:   the board must already be in download mode and ESP32_PORT must
           point at the serial device (usually /dev/cu.usbmodem*).

Config:
  Copy $PICO_DIR/.env.pico.template     to .env.pico    (Pico W hardware)
  Copy $PICO_DIR/.env.posix.template    to .env.posix   (POSIX simulator)
  Copy $PICO_DIR/.env.esp32.template    to .env.esp32   (ESP32)
  Fill in WIFI_SSID / WIFI_PASSWORD / BEATLED_SERVER_NAME / NUM_PIXELS /
  WS2812_PIN. These values are baked into the firmware at build time. The
  POSIX simulator only needs BEATLED_SERVER_NAME + NUM_PIXELS (no Wi-Fi or
  GPIO), so .env.posix can stay minimal.
EOF
  exit 1
}

usage_server() {
  cat <<EOF
Usage: $(basename "$0") server <subcommand> [options]

Subcommands:
  build                      Build the beat server (cmake + ninja, vcpkg toolchain)
  start [options]            Build (if needed) and start the beat server in the foreground
  deploy <user> <host>       Build, tarball, scp, and systemd-reload the RPi build via SSH

Service flags (which sub-services to start — none default on):
  --start-http         HTTPS API + static client serving on --http-port (default 8443)
  --start-udp          UDP server on --udp-port (default 9090) for HELLO / TIME / TEMPO
  --start-broadcast    Per-beat tempo dispatcher + PROGRAM push (see --broadcast-mode)

Script flags (handled by $(basename "$0"), not passed to the binary):
  --no-client          Don't treat the React bundle as a dependency. By default
                       'server start' builds client/dist if it's missing and
                       serves it as the static root; pass this to skip that
                       (e.g. when you're running 'client react dev' separately).

Server config:
  -a ADDRESS                Listen address (default: localhost — script overrides to 0.0.0.0)
  -p, --http-port PORT      HTTP listening port (default: 8443)
  -u, --udp-port PORT       UDP listening port (default: 9090)
  -n, --thread-pool-size N  asio worker threads (default: 2)
  -r, --root-dir PATH       Static-file root (default: client/dist)
  --certs-dir PATH          TLS cert / key / DH parameter directory
  --no-tls                  Serve plain HTTP (development only — logs a WARN)
  --cors-origin URL         Allow CORS from a single origin (default: disabled)
  --api-token TOKEN         Require 'Authorization: Bearer <token>' on every state-changing call
  --log-level LEVEL         spdlog level (default: info). Valid values:
                            trace, debug, info, warn, err, critical, off.
                            Falls back to the BEATLED_LOG_LEVEL env var
                            when the flag is not on the CLI.

Broadcaster config (only relevant when --start-broadcast is on):
  --broadcast-mode MODE     unicast (default) | subnet | limited
  -c, --m_broadcasting-address ADDR   destination for subnet/limited mode
  -b, --broadcasting-port PORT        UDP destination port (default 8765)
  --program-refresh-ms MS   PROGRAM background refresh period in ms
                            (default 200). On-change pushes are also
                            sent twice ~50 ms apart for Wi-Fi loss
                            insurance; lower this if you want even
                            faster catch-up for controllers that
                            missed both copies.

QoS / diagnostics config (protocol v4):
  --status-probe-ms MS      STATUS probe period in ms (default 5000;
                            0 disables). The server unicasts a
                            STATUS_REQUEST to each registered client
                            at this cadence; the response carries a
                            fresh RTT measurement plus the same
                            diagnostic block that piggy-backs on
                            TEMPO_REQUEST. Surfaced on /api/qos.
  --qos-skew-warn-us US     Fleet skew (max-min controller offset in
                            us) at which the QoS pip turns amber
                            (default 5000).
  --qos-skew-fail-us US     Fleet skew at which the QoS pip turns red
                            (default 20000). A non-zero intercore
                            drop or TIME-sync outlier total anywhere
                            in the fleet also forces red.

  unicast (default): one packet per registered client, with per-client OWD
                     compensation. Best on Wi-Fi for <=10 controllers.
  subnet:            broadcast to --m_broadcasting-address (e.g. 192.168.1.255).
  limited:           broadcast to 255.255.255.255 (often dropped by Wi-Fi APs).

Examples:
  $(basename "$0") server start --start-http --start-udp --start-broadcast
  $(basename "$0") server start --start-http --api-token "\$BEATLED_API_TOKEN"
  $(basename "$0") server start --start-http --no-tls --cors-origin "https://localhost:5173"
  $(basename "$0") server start --start-http --no-client   # UI served by 'client react dev'
  $(basename "$0") server deploy pi beatled.local
EOF
  exit 1
}

# --- Build helpers ---

build_server() {
  if [ ! -d "$SERVER_BUILD_DIR" ]; then
    info "Configuring server build (first time)..."
    cmake -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -B "$SERVER_BUILD_DIR" \
      -S "$SERVER_DIR"
  fi

  info "Building server..."
  cmake --build "$SERVER_BUILD_DIR"
  ok "Server build complete"
}

build_client() {
  info "Building client..."
  (cd "$CLIENT_DIR" && npm install --silent && npm run build)
  ok "Client build complete (output: $CLIENT_DIR/dist)"
}

build_rpi() {
  info "Building Docker builder image..."
  docker build -f "$PROJECT_DIR/docker/Dockerfile.builder" "$PROJECT_DIR" --progress=plain -t docker-builder

  info "Building beatled server (ARM64)..."
  rm -rf "$PROJECT_DIR/out"
  docker build -f "$PROJECT_DIR/docker/Dockerfile.beatled" "$PROJECT_DIR" --progress=plain -o "$PROJECT_DIR/out"

  ok "RPi build complete (output: $PROJECT_DIR/out/)"
}

build_pico() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi

  load_posix_env

  if [ ! -d "$PICO_BUILD_DIR" ]; then
    info "Configuring pico build (posix port, first time)..."
    cmake \
      -DPORT=posix \
      -DNUM_PIXELS="$NUM_PIXELS" \
      -DWS2812_PIN="$WS2812_PIN" \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
      -DCMAKE_BUILD_TYPE=Debug \
      -B "$PICO_BUILD_DIR" \
      -S "$PICO_DIR"
  fi

  info "Building pico (posix)..."
  cmake --build "$PICO_BUILD_DIR"
  ok "Pico build complete"
}

build_pico_freertos() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi

  load_posix_env

  if [ ! -d "$PICO_FREERTOS_BUILD_DIR" ]; then
    info "Configuring pico build (posix_freertos port, first time)..."
    cmake \
      -DPORT=posix_freertos \
      -DNUM_PIXELS="$NUM_PIXELS" \
      -DWS2812_PIN="$WS2812_PIN" \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
      -DCMAKE_BUILD_TYPE=Debug \
      -B "$PICO_FREERTOS_BUILD_DIR" \
      -S "$PICO_DIR"
  fi

  info "Building pico (posix_freertos)..."
  cmake --build "$PICO_FREERTOS_BUILD_DIR"
  ok "Pico FreeRTOS build complete"
}

# Reconfigure on every build so changes to .env.pico (WIFI_SSID, server
# name, etc.) propagate. CMake's incremental reconfigure is fast and the
# explicit -D values override any CACHE INTERNAL variables that would
# otherwise stick to the first value seen.
configure_pico_hw_cmake() {
  local port="$1"
  local build_dir="$2"
  cmake \
    -DPORT="$port" \
    -DPICO_BOARD=pico_w \
    -DPICO_SDK_PATH="$PICO_DIR/lib/pico-sdk" \
    -DWIFI_SSID="$WIFI_SSID" \
    -DWIFI_PASSWORD="$WIFI_PASSWORD" \
    -DWIFI_SSID_2="$WIFI_SSID_2" \
    -DWIFI_PASSWORD_2="$WIFI_PASSWORD_2" \
    -DWIFI_SSID_3="$WIFI_SSID_3" \
    -DWIFI_PASSWORD_3="$WIFI_PASSWORD_3" \
    -DWIFI_SSID_4="$WIFI_SSID_4" \
    -DWIFI_PASSWORD_4="$WIFI_PASSWORD_4" \
    -DBEATLED_SERVER_NAME="$BEATLED_SERVER_NAME" \
    -DNUM_PIXELS="$NUM_PIXELS" \
    -DWS2812_PIN="$WS2812_PIN" \
    -B "$build_dir" \
    -S "$PICO_DIR"
}

build_pico_hw() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi

  load_pico_env

  info "Configuring pico hardware build (pico port)..."
  configure_pico_hw_cmake pico "$PICO_HW_BUILD_DIR"

  info "Building pico hardware firmware (pico)..."
  cmake --build "$PICO_HW_BUILD_DIR"
  ok "Pico hardware build complete (UF2: $PICO_HW_BUILD_DIR/src/pico_w_beatled.uf2)"
}

build_pico_freertos_hw() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi

  load_pico_env

  info "Configuring pico hardware build (pico_freertos port)..."
  configure_pico_hw_cmake pico_freertos "$PICO_HW_FREERTOS_BUILD_DIR"

  info "Building pico hardware firmware (pico_freertos)..."
  cmake --build "$PICO_HW_FREERTOS_BUILD_DIR"
  ok "Pico FreeRTOS hardware build complete (UF2: $PICO_HW_FREERTOS_BUILD_DIR/src/pico_w_beatled.uf2)"
}

build_esp32() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi
  load_esp32_env
  info "Building ESP32 firmware (target: $ESP32_TARGET)..."
  (cd "$PICO_DIR/esp32" && \
    idf.py set-target "$ESP32_TARGET" && \
    WIFI_SSID="$WIFI_SSID" \
    WIFI_PASSWORD="$WIFI_PASSWORD" \
    WIFI_SSID_2="$WIFI_SSID_2" \
    WIFI_PASSWORD_2="$WIFI_PASSWORD_2" \
    WIFI_SSID_3="$WIFI_SSID_3" \
    WIFI_PASSWORD_3="$WIFI_PASSWORD_3" \
    WIFI_SSID_4="$WIFI_SSID_4" \
    WIFI_PASSWORD_4="$WIFI_PASSWORD_4" \
    BEATLED_SERVER_NAME="$BEATLED_SERVER_NAME" \
    WS2812_PIN="$WS2812_PIN" \
      idf.py build)
  ok "ESP32 build complete"
}

# --- Clean helpers ---

clean_server() {
  if [ -d "$SERVER_BUILD_DIR" ]; then
    info "Cleaning server build..."
    rm -rf "$SERVER_BUILD_DIR"
    ok "Server build cleaned"
  else
    info "Server build directory does not exist (already clean)"
  fi
}

clean_client() {
  info "Cleaning client build..."
  (cd "$CLIENT_DIR" && rm -rf dist node_modules/.vite)
  ok "Client build cleaned"
}

clean_pico() {
  local cleaned=0
  for dir in "$PICO_BUILD_DIR" "$PICO_HW_BUILD_DIR"; do
    if [ -d "$dir" ]; then
      info "Cleaning $dir..."
      rm -rf "$dir"
      cleaned=1
    fi
  done
  if [ "$cleaned" -eq 1 ]; then
    ok "Pico build cleaned"
  else
    info "Pico build directories do not exist (already clean)"
  fi
}

clean_pico_freertos() {
  local cleaned=0
  for dir in "$PICO_FREERTOS_BUILD_DIR" "$PICO_HW_FREERTOS_BUILD_DIR"; do
    if [ -d "$dir" ]; then
      info "Cleaning $dir..."
      rm -rf "$dir"
      cleaned=1
    fi
  done
  if [ "$cleaned" -eq 1 ]; then
    ok "Pico FreeRTOS build cleaned"
  else
    info "Pico FreeRTOS build directories do not exist (already clean)"
  fi
}

clean_esp32() {
  local dir="$PICO_DIR/esp32/build"
  if [ -d "$dir" ]; then
    info "Cleaning $dir..."
    rm -rf "$dir"
    ok "ESP32 build cleaned"
  else
    info "ESP32 build directory does not exist (already clean)"
  fi
}

# --- Env loaders ---

load_pico_env() {
  local env_file="$PICO_DIR/.env.pico"
  if [[ ! -f "$env_file" ]]; then
    error "Pico env file not found: $env_file"
    error "Copy .env.pico.template to .env.pico and fill in your values"
    exit 1
  fi
  set -a
  # shellcheck disable=SC1090
  source "$env_file"
  # Fallback networks are optional; default empty so unset slots don't trip
  # `set -u` when forwarded as -D flags.
  : "${WIFI_SSID_2:=}"; : "${WIFI_PASSWORD_2:=}"
  : "${WIFI_SSID_3:=}"; : "${WIFI_PASSWORD_3:=}"
  : "${WIFI_SSID_4:=}"; : "${WIFI_PASSWORD_4:=}"
  set +a
}

load_posix_env() {
  local env_file="$PICO_DIR/.env.posix"
  if [[ ! -f "$env_file" ]]; then
    error "POSIX env file not found: $env_file"
    error "Copy .env.posix.template to .env.posix and fill in your values"
    exit 1
  fi
  # The native simulator has no Wi-Fi and no real GPIO, so .env.posix only needs
  # to carry what actually differs from the hardware build (server + pixel
  # count). Seed harmless defaults for the rest so the shared CMake/source still
  # compiles; anything set in .env.posix below overrides these.
  set -a
  : "${WIFI_SSID:=}"
  : "${WIFI_PASSWORD:=}"
  : "${WIFI_SSID_2:=}"; : "${WIFI_PASSWORD_2:=}"
  : "${WIFI_SSID_3:=}"; : "${WIFI_PASSWORD_3:=}"
  : "${WIFI_SSID_4:=}"; : "${WIFI_PASSWORD_4:=}"
  : "${WS2812_PIN:=0}"
  : "${NUM_PIXELS:=30}"
  # shellcheck disable=SC1090
  source "$env_file"
  set +a
}

load_esp32_env() {
  local env_file="$PICO_DIR/.env.esp32"
  if [[ ! -f "$env_file" ]]; then
    error "ESP32 env file not found: $env_file"
    error "Create it with WIFI_SSID, WIFI_PASSWORD, BEATLED_SERVER_NAME, ESP32_TARGET, ESP32_PORT"
    exit 1
  fi
  set -a
  # shellcheck disable=SC1090
  source "$env_file"
  # Fallback networks are optional; default empty so unset slots don't trip
  # `set -u` when forwarded to idf.py.
  : "${WIFI_SSID_2:=}"; : "${WIFI_PASSWORD_2:=}"
  : "${WIFI_SSID_3:=}"; : "${WIFI_PASSWORD_3:=}"
  : "${WIFI_SSID_4:=}"; : "${WIFI_PASSWORD_4:=}"
  set +a
}

# --- Server commands ---

cmd_server_build() {
  build_server
}

cmd_server_start() {
  build_server

  local BIN="$SERVER_BUILD_DIR/src/beat_server"
  local CERTS_DIR="$SERVER_DIR/certs"
  local CLIENT_DIST="$CLIENT_DIR/dist"

  # Pull our own --no-client flag out of the args before they reach the binary
  # (beat_server doesn't understand it); everything else passes through.
  local SERVER_NO_CLIENT=0
  local PASSTHRU=()
  local arg
  for arg in "$@"; do
    if [ "$arg" = "--no-client" ]; then
      SERVER_NO_CLIENT=1
    else
      PASSTHRU+=("$arg")
    fi
  done
  if [ "${#PASSTHRU[@]}" -gt 0 ]; then set -- "${PASSTHRU[@]}"; else set --; fi

  local ARGS=("--certs-dir" "$CERTS_DIR" "-a" "0.0.0.0")

  # The client bundle is a dependency of the server: the server serves it as
  # its static root, so without it GET / 404s (Chrome shows ERR_EMPTY_RESPONSE).
  # Build it on demand if it's missing rather than starting a server that can't
  # serve the UI. Pass --no-client to skip (e.g. when using the Vite dev server).
  if [ "$SERVER_NO_CLIENT" = "1" ]; then
    info "Skipping client dependency (--no-client); UI must be served elsewhere (e.g. 'client react dev')"
  elif [ ! -d "$CLIENT_DIST" ]; then
    warn "Client not built yet ($CLIENT_DIST not found) — building it now"
    build_client
  fi

  if [ "$SERVER_NO_CLIENT" != "1" ] && [ -d "$CLIENT_DIST" ]; then
    ARGS+=("--root-dir" "$CLIENT_DIST")
  fi

  if [ ! -f "$CERTS_DIR/cert.pem" ]; then
    warn "No TLS certificates found in $CERTS_DIR"
    warn "Run '$(basename "$0") certs' to generate them"
  fi

  info "Starting beat server..."
  echo -e "${CYAN}  $BIN ${ARGS[*]} $*${NC}"
  exec "$BIN" "${ARGS[@]}" "$@"
}

cmd_server_deploy() {
  local username="${1:-}"
  local host="${2:-}"

  if [ -z "$username" ] || [ -z "$host" ]; then
    error "Usage: $(basename "$0") server deploy <username> <host>"
    exit 1
  fi

  local out_dir="$PROJECT_DIR/out"
  local tgz="beat-server.tar.gz"
  local remote_home="/home/${username}"
  local install_dir="\$HOME/beat-server"
  local certs_dir="$HOME/certs"

  if [ ! -d "$out_dir" ] || [ -z "$(ls -A "$out_dir" 2>/dev/null)" ]; then
    error "Build output not found in $out_dir"
    error "Run '$(basename "$0") build rpi' first"
    exit 1
  fi

  info "Deploying beatled server to ${username}@${host}"

  if ! ssh "${username}@${host}" "test -f ~/certs/cert.pem" 2>/dev/null; then
    if [ ! -d "$certs_dir" ] || [ ! -f "$certs_dir/cert.pem" ]; then
      error "No certificates found locally at $certs_dir"
      error "Generate them first: scripts/deploy/create-certs.sh ${host}"
      exit 1
    fi
    info "Copying certificates to ${host}..."
    scp -r "$certs_dir" "${username}@${host}:${remote_home}/certs" > /dev/null
  fi

  info "Packaging build output..."
  (cd "$out_dir" && rm -f "$tgz" && tar -czf "$tgz" ./*)

  info "Copying tarball to ${host}..."
  scp "$out_dir/$tgz" "${username}@${host}:${remote_home}/" > /dev/null

  rm "$out_dir/$tgz"

  info "Installing and restarting service on ${host}..."
  local remote_out
  if remote_out=$(ssh "${username}@${host}" "set -e; \
    mv ${install_dir} ${install_dir}.bak 2>/dev/null || true; \
    mkdir -p ${install_dir}; \
    tar -xf ${remote_home}/${tgz} -C ${install_dir}; \
    rm ${remote_home}/${tgz}; \
    ${install_dir}/scripts/deploy/reload-service.sh; \
    systemctl status beat-server.service" 2>&1); then
    ssh "${username}@${host}" "rm -rf ${install_dir}.bak" 2>/dev/null || true
    ok "Deploy complete — server running at https://${host}:8443/"
  else
    error "Deploy failed, restoring previous version..."
    ssh "${username}@${host}" "rm -rf ${install_dir}; \
      mv ${install_dir}.bak ${install_dir} 2>/dev/null || true" 2>/dev/null
    error "Service output:"
    echo -e "$remote_out"
    exit 1
  fi

  info "Verifying server is responding..."
  if curl -sk --retry 3 --retry-delay 2 --max-time 10 \
    "https://${host}:8443/api/health" > /dev/null 2>&1; then
    ok "Health check passed"
  else
    warn "Health check failed — server may still be starting up"
  fi

  if [ "$(uname -s)" = "Darwin" ]; then
    warn "Safari does not accept self-signed certificates — use Chrome instead"
    python3 -m webbrowser "https://${host}:8443/"
  fi
}

# --- Client commands ---

cmd_client_react_dev() {
  info "Starting Vite dev server..."
  info "API requests proxy to https://127.0.0.1:8443 (configure in vite.config.ts)"
  (cd "$CLIENT_DIR" && npm install --silent && npm run dev)
}

cmd_client_react_build() {
  build_client
}

_xcodeproj_check() {
  local XCODEPROJ="$PROJECT_DIR/ios/Beatled.xcodeproj"
  if [ ! -d "$XCODEPROJ" ]; then
    error "iOS project not found: $XCODEPROJ"
    error "Run 'xcodegen generate --spec $PROJECT_DIR/ios/project.yml' to generate it"
    exit 1
  fi
  echo "$XCODEPROJ"
}

cmd_client_ios_build() {
  local XCODEPROJ
  XCODEPROJ=$(_xcodeproj_check)
  info "Building Beatled iOS app (Debug, iphonesimulator)..."
  xcodebuild build \
    -project "$XCODEPROJ" \
    -scheme Beatled \
    -configuration Debug \
    -destination "generic/platform=iOS Simulator" \
    -quiet
  ok "iOS build complete"
}

cmd_client_ios_sim() {
  local XCODEPROJ
  XCODEPROJ=$(_xcodeproj_check)

  cmd_client_ios_build

  info "Booting iOS Simulator..."
  local sim_id
  sim_id=$(xcrun simctl list devices available iPhone -j \
    | python3 -c "
import sys, json
data = json.load(sys.stdin)
for runtime, devices in data['devices'].items():
    for d in devices:
        if d['isAvailable'] and 'iPhone' in d['name']:
            print(d['udid'])
            sys.exit(0)
sys.exit(1)
")

  if [ -z "$sim_id" ]; then
    error "No available iPhone simulator found"
    exit 1
  fi

  xcrun simctl boot "$sim_id" 2>/dev/null || true
  open -a Simulator

  info "Installing and launching Beatled..."
  local app_path
  app_path=$(xcodebuild build \
    -project "$XCODEPROJ" \
    -scheme Beatled \
    -configuration Debug \
    -destination "id=$sim_id" \
    -showBuildSettings 2>/dev/null \
    | grep -m1 "BUILT_PRODUCTS_DIR" | awk '{print $3}')

  xcrun simctl install "$sim_id" "$app_path/Beatled.app"
  xcrun simctl launch "$sim_id" com.beatled.app
  ok "Beatled running in simulator"
}

cmd_client_macos_build() {
  local XCODEPROJ
  XCODEPROJ=$(_xcodeproj_check)
  info "Building Beatled macOS app (Debug)..."
  xcodebuild build \
    -project "$XCODEPROJ" \
    -scheme Beatled \
    -configuration Debug \
    -destination "platform=macOS" \
    -quiet
  ok "macOS build complete"
}

cmd_client_macos_start() {
  local XCODEPROJ
  XCODEPROJ=$(_xcodeproj_check)

  cmd_client_macos_build

  local app_path
  app_path=$(xcodebuild build \
    -project "$XCODEPROJ" \
    -scheme Beatled \
    -configuration Debug \
    -destination "platform=macOS" \
    -showBuildSettings 2>/dev/null \
    | grep -m1 "BUILT_PRODUCTS_DIR" | awk '{print $3}')

  info "Launching Beatled.app..."
  open "$app_path/Beatled.app"
  ok "Beatled running"
}

# --- Controller commands ---

cmd_controller_pico_build() {
  build_pico_hw
}

cmd_controller_pico_freertos_build() {
  build_pico_freertos_hw
}

# Copy a .uf2 onto a Pico mounted in BOOTSEL mode at /Volumes/RPI-RP2.
flash_pico_uf2() {
  local uf2="$1"
  local mount="/Volumes/RPI-RP2"

  if [ ! -f "$uf2" ]; then
    error ".uf2 not found: $uf2"
    error "Run the matching build subcommand first"
    exit 1
  fi

  if [ ! -d "$mount" ]; then
    warn "Pico not in BOOTSEL mode ($mount not mounted)"
    warn "Hold BOOTSEL while plugging the Pico in, then re-run this command"
    exit 1
  fi

  info "Flashing $(basename "$uf2") -> $mount"
  cp "$uf2" "$mount/"
  # macOS unmounts RPI-RP2 the moment cp finishes — that's the Pico
  # rebooting into the new firmware, not an error.
  ok "Flash complete (Pico is rebooting)"
}

cmd_controller_pico_flash() {
  build_pico_hw
  flash_pico_uf2 "$PICO_HW_BUILD_DIR/src/pico_w_beatled.uf2"
}

cmd_controller_pico_freertos_flash() {
  build_pico_freertos_hw
  flash_pico_uf2 "$PICO_HW_FREERTOS_BUILD_DIR/src/pico_w_beatled.uf2"
}

cmd_controller_posix_build() {
  build_pico

  local BIN="$PICO_BUILD_DIR/src/pico_w_beatled.app/Contents/MacOS/pico_w_beatled"
  if [ ! -f "$BIN" ]; then
    BIN="$PICO_BUILD_DIR/src/pico_w_beatled"
  fi

  info "Starting controller firmware (posix)..."
  exec "$BIN"
}

cmd_controller_freertos_sim_build() {
  build_pico_freertos

  local BIN="$PICO_FREERTOS_BUILD_DIR/src/pico_w_beatled.app/Contents/MacOS/pico_w_beatled"
  if [ ! -f "$BIN" ]; then
    BIN="$PICO_FREERTOS_BUILD_DIR/src/pico_w_beatled"
  fi

  info "Starting controller firmware (freertos-sim)..."
  exec "$BIN"
}

cmd_controller_esp32_build() {
  build_esp32
}

cmd_controller_esp32_flash() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi
  load_esp32_env
  info "Building and flashing ESP32 (target: $ESP32_TARGET, port: $ESP32_PORT)..."
  (cd "$PICO_DIR/esp32" && \
    idf.py set-target "$ESP32_TARGET" && \
    WIFI_SSID="$WIFI_SSID" \
    WIFI_PASSWORD="$WIFI_PASSWORD" \
    BEATLED_SERVER_NAME="$BEATLED_SERVER_NAME" \
    WS2812_PIN="$WS2812_PIN" \
      idf.py build flash monitor -p "$ESP32_PORT")
}

cmd_controller_esp32_monitor() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to the controller/ directory (defaults to \$PROJECT_DIR/controller)"
    exit 1
  fi
  load_esp32_env
  info "Opening ESP32 serial monitor (port: $ESP32_PORT)..."
  (cd "$PICO_DIR/esp32" && idf.py monitor -p "$ESP32_PORT")
}

# --- Utility commands ---

usage_test() {
  cat <<EOF
Usage: $(basename "$0") test [component]

Components:
  server   Catch2 unit + integration tests against the C++ beat server
  client   Vitest run for the React client (lib + view tests)
  pico     POSIX-port firmware tests (build + ctest)
  all      All of the above (default if omitted)
EOF
  exit 1
}

cmd_test() {
  local component="${1:-all}"

  case "$component" in
    help|-h|--help) usage_test ;;
    server)
      build_server
      info "Running server tests..."
      "$SERVER_BUILD_DIR/tests/api_handler/test_api_handler"
      "$SERVER_BUILD_DIR/tests/state_manager/test_state_manager"
      "$SERVER_BUILD_DIR/tests/test_audio_buffer_pool"
      "$SERVER_BUILD_DIR/tests/udp/test_udp_protocol"
      "$SERVER_BUILD_DIR/tests/udp/test_udp_request_handler"
      "$SERVER_BUILD_DIR/tests/http/test_mime_types"
      "$SERVER_BUILD_DIR/tests/http/test_api_gates"
      ;;
    client)
      info "Running client tests..."
      (cd "$CLIENT_DIR" && npm install --silent && npm test -- --run)
      ;;
    pico)
      build_pico
      info "Running pico tests..."
      ctest --test-dir "$PICO_BUILD_DIR" --output-on-failure
      ;;
    all)
      cmd_test server
      echo
      cmd_test client
      echo
      cmd_test pico
      ;;
    *)
      error "Unknown test component: $component (use server, client, pico, or all)"
      exit 1
      ;;
  esac

  ok "Tests passed ($component)"
}

usage_build() {
  cat <<EOF
Usage: $(basename "$0") build [component]

Builds the named component(s) without running anything. Tests are not run.

Components:
  server          C++ beat_server (cmake + ninja, vcpkg toolchain)
  client          React frontend (Vite production build → client/dist/)
  pico            Pico W firmware POSIX simulator
  pico-freertos   Pico W firmware POSIX-FreeRTOS simulator
  rpi             Cross-compiled aarch64 server tarball under out/ (Docker)
  all             server + client + pico + pico-freertos (default)
EOF
  exit 1
}

cmd_build() {
  local component="${1:-all}"

  case "$component" in
    help|-h|--help) usage_build ;;
    server)         build_server ;;
    client)         build_client ;;
    pico)           build_pico ;;
    pico-freertos)  build_pico_freertos ;;
    rpi)            build_rpi ;;
    all)
      build_server
      build_client
      build_pico
      build_pico_freertos
      ;;
    *)
      error "Unknown build component: $component (use server, client, pico, pico-freertos, rpi, or all)"
      exit 1
      ;;
  esac
}

usage_clean() {
  cat <<EOF
Usage: $(basename "$0") clean [component]

Removes the named component's build artifacts.

Components:
  server          Wipes server/build/
  client          Wipes client/dist/ and client/node_modules/.vite
  pico            Wipes controller/build/ (POSIX sim) and controller/build-pico/ (uf2)
  pico-freertos   Wipes controller/build_posix_freertos/ and controller/build-pico-freertos/
  esp32           Wipes controller/esp32/build/
  all             All of the above (default)
EOF
  exit 1
}

cmd_clean() {
  local component="${1:-all}"

  case "$component" in
    help|-h|--help) usage_clean ;;
    server)         clean_server ;;
    client)         clean_client ;;
    pico)           clean_pico ;;
    pico-freertos)  clean_pico_freertos ;;
    esp32)          clean_esp32 ;;
    all)
      clean_server
      clean_client
      clean_pico
      clean_pico_freertos
      clean_esp32
      ok "All build artifacts cleaned"
      ;;
    *)
      error "Unknown clean component: $component (use server, client, pico, pico-freertos, esp32, or all)"
      exit 1
      ;;
  esac
}

usage_docs() {
  cat <<EOF
Usage: $(basename "$0") docs [-- <jekyll options>]

Builds and serves the Jekyll docs site (theme: just-the-docs) at
http://127.0.0.1:4000/beatled/. Any options after '--' are passed
through to 'bundle exec jekyll serve' (e.g. --livereload, --port).
EOF
  exit 1
}

cmd_docs() {
  if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ] || [ "${1:-}" = "help" ]; then
    usage_docs
  fi
  local DOCS_DIR="$PROJECT_DIR/docs"
  info "Starting Jekyll docs site..."
  info "Serving at http://127.0.0.1:4000/beatled/"
  (cd "$DOCS_DIR" && bundle exec jekyll serve "$@")
}

usage_certs() {
  cat <<EOF
Usage: $(basename "$0") certs [domain ...]

Generates locally-trusted TLS certificates with mkcert and writes them
to server/certs/ (cert.pem, key.pem, dh_param.pem). The certs are
copied to the Pi by 'server deploy' on first deploy.

Examples:
  $(basename "$0") certs                                 # default: beatled.test
  $(basename "$0") certs beatled.local
  $(basename "$0") certs beatled.local 192.168.1.100
EOF
  exit 1
}

cmd_certs() {
  if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ] || [ "${1:-}" = "help" ]; then
    usage_certs
  fi
  local certs_dir="$SERVER_DIR/certs"
  local domains=("${@:-beatled.test}")
  info "Generating certificates for '${domains[*]}' in $certs_dir"
  "$SCRIPTS_DIR/deploy/create-certs.sh" --output "$certs_dir" "${domains[@]}"
  ok "Certificates created in $certs_dir"
}

# --- Main ---

CMD1="${1:-}"
CMD2="${2:-}"
CMD3="${3:-}"

case "$CMD1" in
  client)
    case "$CMD2" in
      react)
        case "$CMD3" in
          build)              cmd_client_react_build ;;
          dev)                cmd_client_react_dev ;;
          help|-h|--help|"")  usage_client_react ;;
          *)                  usage_client_react ;;
        esac ;;
      ios)
        case "$CMD3" in
          build)              cmd_client_ios_build ;;
          sim)                cmd_client_ios_sim ;;
          help|-h|--help|"")  usage_client_ios ;;
          *)                  usage_client_ios ;;
        esac ;;
      macos)
        case "$CMD3" in
          build)              cmd_client_macos_build ;;
          start)              cmd_client_macos_start ;;
          help|-h|--help|"")  usage_client_macos ;;
          *)                  usage_client_macos ;;
        esac ;;
      help|-h|--help|"")      usage_client ;;
      *)                      usage_client ;;
    esac ;;

  controller)
    case "$CMD2" in
      pico)
        case "$CMD3" in
          build)              cmd_controller_pico_build ;;
          flash)              cmd_controller_pico_flash ;;
          help|-h|--help|"")  usage_controller ;;
          *)                  usage_controller ;;
        esac ;;
      pico-freertos)
        case "$CMD3" in
          build)              cmd_controller_pico_freertos_build ;;
          flash)              cmd_controller_pico_freertos_flash ;;
          help|-h|--help|"")  usage_controller ;;
          *)                  usage_controller ;;
        esac ;;
      posix)
        case "$CMD3" in
          build)              cmd_controller_posix_build ;;
          help|-h|--help|"")  usage_controller ;;
          *)                  usage_controller ;;
        esac ;;
      freertos-sim)
        case "$CMD3" in
          build)              cmd_controller_freertos_sim_build ;;
          help|-h|--help|"")  usage_controller ;;
          *)                  usage_controller ;;
        esac ;;
      esp32-freertos)
        case "$CMD3" in
          build)              cmd_controller_esp32_build ;;
          flash)              cmd_controller_esp32_flash ;;
          monitor)            cmd_controller_esp32_monitor ;;
          help|-h|--help|"")  usage_controller ;;
          *)                  usage_controller ;;
        esac ;;
      help|-h|--help|"")      usage_controller ;;
      *)                      usage_controller ;;
    esac ;;

  server)
    case "$CMD2" in
      build)              cmd_server_build ;;
      start)              shift 2; cmd_server_start "$@" ;;
      deploy)             shift 2; cmd_server_deploy "$@" ;;
      help|-h|--help|"")  usage_server ;;
      *)                  usage_server ;;
    esac ;;

  test)   shift; cmd_test "$@" ;;
  build)  shift; cmd_build "$@" ;;
  clean)  shift; cmd_clean "$@" ;;
  docs)   shift; cmd_docs "$@" ;;
  certs)  shift; cmd_certs "$@" ;;
  help|-h|--help|"") usage ;;
  *)
    error "Unknown command: $CMD1"
    usage
    ;;
esac
