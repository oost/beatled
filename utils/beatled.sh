#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

VCPKG_DIR="${VCPKG_DIR:-$HOME/coding/external/vcpkg}"
SERVER_DIR="$PROJECT_DIR/server"
SERVER_BUILD_DIR="$SERVER_DIR/build"
CLIENT_DIR="$PROJECT_DIR/client"
PICO_DIR="${PICO_DIR:-$HOME/coding/projects/beatled-pico}"
PICO_BUILD_DIR="$PICO_DIR/build"

# --- Colors ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m'

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
  build [component]   Build without running (server, client, pico, or all)
  docs                Start the Jekyll docs site locally
  certs <domain>      Generate self-signed TLS certificates

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

# --- Commands ---

cmd_server() {
  build_server

  local BIN="$SERVER_BUILD_DIR/src/beat_server"
  local CERTS_DIR="$SERVER_DIR/certs"
  local CLIENT_DIST="$CLIENT_DIR/dist"

  # Default args
  local ARGS=("--certs-dir" "$CERTS_DIR")

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
    warn "Run '$(basename "$0") certs localhost' to generate them"
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
    all)
      build_server
      build_client
      build_pico
      ;;
    *)
      error "Unknown build component: $component (use server, client, pico, or all)"
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
  local domain="${1:-localhost}"
  local certs_dir="$SERVER_DIR/certs"
  info "Generating certificates for '$domain' in $certs_dir"
  "$PROJECT_DIR/scripts/create-certs.sh" "$domain" "$certs_dir"
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
  docs)         cmd_docs "$@" ;;
  certs)        cmd_certs "$@" ;;
  help|-h|--help) usage ;;
  *)
    error "Unknown command: $COMMAND"
    usage
    ;;
esac
