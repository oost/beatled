#!/bin/bash

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

readonly VCPKG_DIR="${VCPKG_DIR:-$HOME/coding/external/vcpkg}"
readonly SERVER_DIR="$PROJECT_DIR/server"
readonly SERVER_BUILD_DIR="$SERVER_DIR/build"
readonly CLIENT_DIR="$PROJECT_DIR/client"
readonly PICO_DIR="${PICO_DIR:-$HOME/coding/projects/beatled-pico}"
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
  clean [component]   Clean build artifacts (server, client, pico, pico-freertos, or all)
  docs                Start the Jekyll docs site locally
  certs [domain ...]  Generate locally-trusted development certificates

Examples:
  $(basename "$0") client react dev
  $(basename "$0") client ios sim
  $(basename "$0") controller posix build
  $(basename "$0") controller esp32-freertos flash
  $(basename "$0") server start --start-http --start-udp
  $(basename "$0") server deploy pi beatled.local
EOF
  exit 1
}

usage_client() {
  cat <<EOF
Usage: $(basename "$0") client <subcommand> [action]

Subcommands:
  react   build    Build the React frontend for production
  react   dev      Start the Vite dev server
  ios     build    Build the iOS app (simulator)
  ios     sim      Build and run in iOS Simulator
  macos   build    Build the macOS app
  macos   start    Build and launch the macOS app
EOF
  exit 1
}

usage_client_react() {
  cat <<EOF
Usage: $(basename "$0") client react <action>

Actions:
  build   Build the React frontend for production (output: client/dist)
  dev     Start the Vite dev server (proxies API to https://127.0.0.1:8080)
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
Usage: $(basename "$0") controller <subcommand> [action]

Subcommands:
  pico            build    Build Pico W firmware (bare-metal, outputs .uf2)
  pico-freertos   build    Build Pico W firmware (FreeRTOS SMP, outputs .uf2)
  posix           build    Build and run POSIX simulator (Metal LED renderer)
  freertos-sim    build    Build and run FreeRTOS POSIX simulator
  esp32-freertos  build    Build ESP32 firmware
  esp32-freertos  flash    Build, flash, and monitor ESP32
  esp32-freertos  monitor  Open ESP32 serial monitor

Pico W flashing: copy the .uf2 from build-pico/src/ to /Volumes/RPI-RP2/
ESP32 config:    copy $PICO_DIR/.env.esp32.template to .env.esp32 and fill in values
Pico config:     copy $PICO_DIR/.env.pico.template to .env.pico and fill in values
EOF
  exit 1
}

usage_server() {
  cat <<EOF
Usage: $(basename "$0") server <subcommand> [options]

Subcommands:
  build                      Build the beat server
  start [options]            Build (if needed) and start the beat server
  deploy <user> <host>       Deploy the RPi build via SSH

Server options (passed to beat_server):
  --start-http        Start the HTTP/S server
  --start-udp         Start the UDP server
  --start-broadcast   Start the UDP broadcaster
  --cors-origin URL   Set CORS allowed origin
  --api-token TOKEN   Set Bearer token for API auth
  --http-port PORT    HTTP port (default: 8443)
  -a ADDRESS          Listen address (default: localhost)
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
    error "Set PICO_DIR to your beatled-pico checkout"
    exit 1
  fi

  load_pico_env

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
    error "Set PICO_DIR to your beatled-pico checkout"
    exit 1
  fi

  load_pico_env

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

build_pico_hw() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to your beatled-pico checkout"
    exit 1
  fi

  load_pico_env

  if [ ! -d "$PICO_HW_BUILD_DIR" ]; then
    info "Configuring pico hardware build (pico port, first time)..."
    cmake \
      -DPORT=pico \
      -DPICO_BOARD=pico_w \
      -DPICO_SDK_PATH="$PICO_DIR/lib/pico-sdk" \
      -DNUM_PIXELS="$NUM_PIXELS" \
      -DWS2812_PIN="$WS2812_PIN" \
      -B "$PICO_HW_BUILD_DIR" \
      -S "$PICO_DIR"
  fi

  info "Building pico hardware firmware (pico)..."
  cmake --build "$PICO_HW_BUILD_DIR"
  ok "Pico hardware build complete (UF2: $PICO_HW_BUILD_DIR/src/pico_w_beatled.uf2)"
}

build_pico_freertos_hw() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to your beatled-pico checkout"
    exit 1
  fi

  load_pico_env

  if [ ! -d "$PICO_HW_FREERTOS_BUILD_DIR" ]; then
    info "Configuring pico hardware build (pico_freertos port, first time)..."
    cmake \
      -DPORT=pico_freertos \
      -DPICO_BOARD=pico_w \
      -DPICO_SDK_PATH="$PICO_DIR/lib/pico-sdk" \
      -DNUM_PIXELS="$NUM_PIXELS" \
      -DWS2812_PIN="$WS2812_PIN" \
      -B "$PICO_HW_FREERTOS_BUILD_DIR" \
      -S "$PICO_DIR"
  fi

  info "Building pico hardware firmware (pico_freertos)..."
  cmake --build "$PICO_HW_FREERTOS_BUILD_DIR"
  ok "Pico FreeRTOS hardware build complete (UF2: $PICO_HW_FREERTOS_BUILD_DIR/src/pico_w_beatled.uf2)"
}

build_esp32() {
  if [ ! -d "$PICO_DIR" ]; then
    error "Pico directory not found: $PICO_DIR"
    error "Set PICO_DIR to your beatled-pico checkout"
    exit 1
  fi
  load_esp32_env
  info "Building ESP32 firmware (target: $ESP32_TARGET)..."
  (cd "$PICO_DIR/esp32" && \
    idf.py set-target "$ESP32_TARGET" && \
    WIFI_SSID="$WIFI_SSID" \
    WIFI_PASSWORD="$WIFI_PASSWORD" \
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
  if [ -d "$PICO_BUILD_DIR" ]; then
    info "Cleaning pico build..."
    rm -rf "$PICO_BUILD_DIR"
    ok "Pico build cleaned"
  else
    info "Pico build directory does not exist (already clean)"
  fi
}

clean_pico_freertos() {
  if [ -d "$PICO_FREERTOS_BUILD_DIR" ]; then
    info "Cleaning pico FreeRTOS build..."
    rm -rf "$PICO_FREERTOS_BUILD_DIR"
    ok "Pico FreeRTOS build cleaned"
  else
    info "Pico FreeRTOS build directory does not exist (already clean)"
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
  # shellcheck disable=SC1090
  set -a; source "$env_file"; set +a
}

load_esp32_env() {
  local env_file="$PICO_DIR/.env.esp32"
  if [[ ! -f "$env_file" ]]; then
    error "ESP32 env file not found: $env_file"
    error "Create it with WIFI_SSID, WIFI_PASSWORD, BEATLED_SERVER_NAME, ESP32_TARGET, ESP32_PORT"
    exit 1
  fi
  # shellcheck disable=SC1090
  set -a; source "$env_file"; set +a
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

  local ARGS=("--certs-dir" "$CERTS_DIR" "-a" "0.0.0.0")

  if [ -d "$CLIENT_DIST" ]; then
    ARGS+=("--root-dir" "$CLIENT_DIST")
  else
    warn "Client not built yet ($CLIENT_DIST not found)"
    warn "Run '$(basename "$0") client react build' first, or use '$(basename "$0") client react dev' for dev mode"
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
  info "API requests proxy to https://127.0.0.1:8080 (configure in vite.config.ts)"
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
    error "Set PICO_DIR to your beatled-pico checkout"
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
    error "Set PICO_DIR to your beatled-pico checkout"
    exit 1
  fi
  load_esp32_env
  info "Opening ESP32 serial monitor (port: $ESP32_PORT)..."
  (cd "$PICO_DIR/esp32" && idf.py monitor -p "$ESP32_PORT")
}

# --- Utility commands ---

cmd_test() {
  local component="${1:-all}"

  case "$component" in
    server)
      build_server
      info "Running server tests..."
      "$SERVER_BUILD_DIR/tests/api_handler/test_api_handler"
      "$SERVER_BUILD_DIR/tests/state_manager/test_state_manager"
      "$SERVER_BUILD_DIR/tests/test_audio_buffer_pool"
      "$SERVER_BUILD_DIR/tests/udp/test_udp_protocol"
      "$SERVER_BUILD_DIR/tests/udp/test_udp_request_handler"
      "$SERVER_BUILD_DIR/tests/http/test_mime_types"
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

cmd_build() {
  local component="${1:-all}"

  case "$component" in
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

cmd_clean() {
  local component="${1:-all}"

  case "$component" in
    server)         clean_server ;;
    client)         clean_client ;;
    pico)           clean_pico ;;
    pico-freertos)  clean_pico_freertos ;;
    all)
      clean_server
      clean_client
      clean_pico
      clean_pico_freertos
      ok "All build artifacts cleaned"
      ;;
    *)
      error "Unknown clean component: $component (use server, client, pico, pico-freertos, or all)"
      exit 1
      ;;
  esac
}

cmd_docs() {
  local DOCS_DIR="$PROJECT_DIR/docs"
  info "Starting Jekyll docs site..."
  info "Serving at http://127.0.0.1:4000/beatled/"
  (cd "$DOCS_DIR" && bundle exec jekyll serve "$@")
}

cmd_certs() {
  local certs_dir="$SERVER_DIR/certs"
  local domains=("${@:-beatled.test}")
  info "Generating certificates for '${domains[*]}' in $certs_dir"
  "$SCRIPT_DIR/deploy/create-certs.sh" --output "$certs_dir" "${domains[@]}"
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
          build)   cmd_client_react_build ;;
          dev)     cmd_client_react_dev ;;
          *)       usage_client_react ;;
        esac ;;
      ios)
        case "$CMD3" in
          build)   cmd_client_ios_build ;;
          sim)     cmd_client_ios_sim ;;
          *)       usage_client_ios ;;
        esac ;;
      macos)
        case "$CMD3" in
          build)   cmd_client_macos_build ;;
          start)   cmd_client_macos_start ;;
          *)       usage_client_macos ;;
        esac ;;
      *)           usage_client ;;
    esac ;;

  controller)
    case "$CMD2" in
      pico)
        case "$CMD3" in
          build)   cmd_controller_pico_build ;;
          *)       usage_controller ;;
        esac ;;
      pico-freertos)
        case "$CMD3" in
          build)   cmd_controller_pico_freertos_build ;;
          *)       usage_controller ;;
        esac ;;
      posix)
        case "$CMD3" in
          build)   cmd_controller_posix_build ;;
          *)       usage_controller ;;
        esac ;;
      freertos-sim)
        case "$CMD3" in
          build)   cmd_controller_freertos_sim_build ;;
          *)       usage_controller ;;
        esac ;;
      esp32-freertos)
        case "$CMD3" in
          build)   cmd_controller_esp32_build ;;
          flash)   cmd_controller_esp32_flash ;;
          monitor) cmd_controller_esp32_monitor ;;
          *)       usage_controller ;;
        esac ;;
      *)           usage_controller ;;
    esac ;;

  server)
    case "$CMD2" in
      build)   cmd_server_build ;;
      start)   shift 2; cmd_server_start "$@" ;;
      deploy)  shift 2; cmd_server_deploy "$@" ;;
      *)       usage_server ;;
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
