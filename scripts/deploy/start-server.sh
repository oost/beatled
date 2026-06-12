#!/bin/bash
#
# Start the beat server directly (without systemd).
#

set -euo pipefail

readonly SCRIPT_DIR="$(cd -P "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
readonly IP4_ADDRESS="0.0.0.0"

readonly SERVER_BIN="$SCRIPT_DIR/../../server/beat_server"
if [[ ! -x "$SERVER_BIN" ]]; then
  echo "==> ERROR: $SERVER_BIN not found or not executable" >&2
  echo "==> Build it first: ./beatled.sh server build" >&2
  exit 1
fi

exec "$SERVER_BIN" \
  -a "$IP4_ADDRESS" \
  --root-dir "$SCRIPT_DIR/../../client/" \
  --certs-dir "$HOME/certs/" \
  --start-http \
  --start-udp
