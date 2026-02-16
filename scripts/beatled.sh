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

usage() {
  cat <<EOF
Usage: $(basename "$0") <command> [options]

Commands:
  server [options]    Build (if needed) and start the beat server
  client              Start the Vite dev server (with proxy to beat server)
  client-build        Build the client for production
  pico                Build (if needed) and run the Pico firmware (posix port)
  test [component]    Run tests (server, client, pico, or all)
  build [component]   Build without running (server, client, pico, rpi, or all)
  clean [component]   Clean build artifacts (server, client, pico, or all)
  deploy <user> <host> Deploy the RPi build to a Raspberry Pi via SSH
  ios [subcommand]    iOS/macOS commands (open, build, sim, mac). Default: open
  docs                Start the Jekyll docs site locally
  certs [domain ...]  Generate locally-trusted development certificates for the given domains (default: oost.test)

Server options (passed through to beat_server):
  --start-http        Start the HTTP/S server
  --start-udp         Start the UDP server
  --start-broadcast   Start the UDP broadcaster
  --cors-origin URL   Set CORS allowed origin
  --api-token TOKEN   Set Bearer token for API auth
  --http-port PORT    HTTP port (default: 8443)
  -a ADDRESS          Listen address (default: localhost)

Examples:
  $(basename "$0") server --start-http
  $(basename "$0") server --start-http --start-udp --cors-origin "https://localhost:5173"
  $(basename "$0") client
  $(basename "$0") test all
  $(basename "$0") build server
  $(basename "$0") build rpi
  $(basename "$0") deploy pi beatled.local
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

  if [ ! -d "$PICO_BUILD_DIR" ]; then
    info "Configuring pico build (posix port, first time)..."
    cmake \
      -DPORT=posix \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
      -DCMAKE_BUILD_TYPE=Debug \
      -B "$PICO_BUILD_DIR" \
      -S "$PICO_DIR"
  fi

  info "Building pico (posix)..."
  cmake --build "$PICO_BUILD_DIR"
  ok "Pico build complete"
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

# --- Commands ---

cmd_server() {
  build_server

  local BIN="$SERVER_BUILD_DIR/src/beat_server"
  local CERTS_DIR="$SERVER_DIR/certs"
  local CLIENT_DIST="$CLIENT_DIR/dist"

  # Default args
  local ARGS=("--certs-dir" "$CERTS_DIR" "-a" "0.0.0.0")

  # Serve built client if dist exists
  if [ -d "$CLIENT_DIST" ]; then
    ARGS+=("--root-dir" "$CLIENT_DIST")
  else
    warn "Client not built yet ($CLIENT_DIST not found)"
    warn "Run '$(basename "$0") client-build' first, or use '$(basename "$0") client' for dev mode"
  fi

  # Check certs exist
  if [ ! -f "$CERTS_DIR/cert.pem" ]; then
    warn "No TLS certificates found in $CERTS_DIR"
    warn "Run '$(basename "$0") certs' to generate them"
  fi

  info "Starting beat server..."
  echo -e "${CYAN}  $BIN ${ARGS[*]} $*${NC}"
  exec "$BIN" "${ARGS[@]}" "$@"
}

cmd_client() {
  info "Starting Vite dev server..."
  info "API requests proxy to https://127.0.0.1:8080 (configure in vite.config.ts)"
  (cd "$CLIENT_DIR" && npm install --silent && npm run dev)
}

cmd_client_build() {
  build_client
}

cmd_pico() {
  build_pico

  local BIN="$PICO_BUILD_DIR/src/pico_w_beatled.app/Contents/MacOS/pico_w_beatled"
  if [ ! -f "$BIN" ]; then
    # Fallback for non-macOS
    BIN="$PICO_BUILD_DIR/src/pico_w_beatled"
  fi

  info "Starting pico firmware (posix)..."
  exec "$BIN" "$@"
}

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
    server)  build_server ;;
    client)  build_client ;;
    pico)    build_pico ;;
    rpi)     build_rpi ;;
    all)
      build_server
      build_client
      build_pico
      ;;
    *)
      error "Unknown build component: $component (use server, client, pico, rpi, or all)"
      exit 1
      ;;
  esac
}

cmd_clean() {
  local component="${1:-all}"

  case "$component" in
    server)  clean_server ;;
    client)  clean_client ;;
    pico)    clean_pico ;;
    all)
      clean_server
      clean_client
      clean_pico
      ok "All build artifacts cleaned"
      ;;
    *)
      error "Unknown clean component: $component (use server, client, pico, or all)"
      exit 1
      ;;
  esac
}

cmd_deploy() {
  local username="${1:-}"
  local host="${2:-}"

  if [ -z "$username" ] || [ -z "$host" ]; then
    error "Usage: $(basename "$0") deploy <username> <host>"
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

  # Ensure certificates exist on remote host
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

  # Health check — verify the server is actually responding
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

cmd_ios() {
  local readonly XCODEPROJ="$PROJECT_DIR/ios/Beatled.xcodeproj"
  local subcommand="${1:-open}"
  shift 2>/dev/null || true

  if [ ! -d "$XCODEPROJ" ]; then
    error "iOS project not found: $XCODEPROJ"
    error "Run 'xcodegen generate --spec $PROJECT_DIR/ios/project.yml' to generate it"
    exit 1
  fi

  case "$subcommand" in
    open)
      info "Opening Beatled iOS project in Xcode..."
      open "$XCODEPROJ"
      ;;
    build)
      info "Building Beatled iOS app (Debug, iphonesimulator)..."
      xcodebuild build \
        -project "$XCODEPROJ" \
        -scheme Beatled \
        -configuration Debug \
        -destination "generic/platform=iOS Simulator" \
        -quiet
      ok "iOS build complete"
      ;;
    sim)
      cmd_ios build

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
      ;;
    mac)
      info "Building Beatled macOS app (Debug)..."
      xcodebuild build \
        -project "$XCODEPROJ" \
        -scheme Beatled \
        -configuration Debug \
        -destination "platform=macOS" \
        -quiet
      ok "macOS build complete"

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
      ;;
    *)
      error "Unknown ios subcommand: $subcommand (use open, build, sim, or mac)"
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

if [ $# -lt 1 ]; then
  usage
fi

COMMAND="$1"
shift

case "$COMMAND" in
  server)       cmd_server "$@" ;;
  client)       cmd_client "$@" ;;
  client-build) cmd_client_build "$@" ;;
  pico)         cmd_pico "$@" ;;
  test)         cmd_test "$@" ;;
  build)        cmd_build "$@" ;;
  clean)        cmd_clean "$@" ;;
  deploy)       cmd_deploy "$@" ;;
  ios)          cmd_ios "$@" ;;
  docs)         cmd_docs "$@" ;;
  certs)        cmd_certs "$@" ;;
  help|-h|--help) usage ;;
  *)
    error "Unknown command: $COMMAND"
    usage
    ;;
esac
