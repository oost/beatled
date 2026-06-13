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

# Real-time tuning headroom. The server locks its memory (mlockall) and runs
# the beat-detector thread at SCHED_FIFO; both need rlimit headroom the default
# login shell doesn't grant. Raise RLIMIT_MEMLOCK and RLIMIT_RTPRIO here so a
# non-root run still gets them. Each is best-effort — if the shell can't raise
# the limit (no privilege / capped in limits.conf) we warn and carry on, and
# the server falls back to pageable memory / SCHED_OTHER with its own warning.
ulimit -l unlimited 2>/dev/null \
  || echo "==> WARN: could not raise RLIMIT_MEMLOCK; mlockall may fail" >&2
ulimit -r 80 2>/dev/null \
  || echo "==> WARN: could not raise RLIMIT_RTPRIO; SCHED_FIFO may be denied" >&2

exec "$SERVER_BIN" \
  -a "$IP4_ADDRESS" \
  --root-dir "$SCRIPT_DIR/../../client/" \
  --certs-dir "$HOME/certs/" \
  --start-http \
  --start-udp
