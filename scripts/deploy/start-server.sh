#!/bin/bash
#
# Start the beat server directly (without systemd).
#

set -euo pipefail

readonly SCRIPT_DIR="$(dirname "$0")"
readonly IP4_ADDRESS="0.0.0.0"

exec "$SCRIPT_DIR/../../server/beat_server" \
  -a "$IP4_ADDRESS" \
  --root-dir "$SCRIPT_DIR/../../client/" \
  --certs-dir "$HOME/certs/" \
  --start-http \
  --start-udp
