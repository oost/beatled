#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

RPIZ_BEATSERVER_TGZ="beat-server.tar.gz"
OUT_DIR="$PROJECT_DIR/out"

# --- Logging helpers ---
info()  { printf "  %-50s" "$1"; }
ok()    { echo " ✅"; }
fail()  { echo " ⛔️"; }

# --- Input validation ---
usage() {
  echo "Usage: $(basename "$0") <username> <host>"
  echo ""
  echo "Deploy the beatled server to a Raspberry Pi."
  echo ""
  echo "Arguments:"
  echo "  username   SSH username on the Raspberry Pi"
  echo "  host       Hostname or IP address of the Raspberry Pi"
  echo ""
  echo "The build output must exist in ./out/ (run utils/build-docker.sh first)."
  exit 1
}

if [[ $# -ne 2 ]]; then
  echo "Error: expected 2 arguments, got $#."
  usage
fi

RPIZ_USERNAME="$1"
RPIZ_HOST="$2"

if [[ -z "$RPIZ_USERNAME" ]]; then
  echo "Error: username cannot be empty."
  usage
fi

if [[ -z "$RPIZ_HOST" ]]; then
  echo "Error: host cannot be empty."
  usage
fi

if [[ ! -d "$OUT_DIR" ]]; then
  echo "Error: build output directory not found at $OUT_DIR"
  echo "Run utils/build-docker.sh first to produce the build output."
  exit 1
fi

# Check that out/ contains files
if [[ -z "$(ls -A "$OUT_DIR" 2>/dev/null)" ]]; then
  echo "Error: build output directory $OUT_DIR is empty."
  echo "Run utils/build-docker.sh first to produce the build output."
  exit 1
fi

echo "==> Deploying beatled server to ${RPIZ_USERNAME}@${RPIZ_HOST}"
echo ""

# --- Package ---
info "Generating tar file from build output"
cd "$OUT_DIR"
rm -f "$RPIZ_BEATSERVER_TGZ"
tar -czf "$RPIZ_BEATSERVER_TGZ" ./*
ok

# --- Transfer ---
REMOTE_HOME="/home/${RPIZ_USERNAME}"
INSTALL_SCRIPT="$SCRIPT_DIR/install-server-on-raspberry-pi.sh"

info "Copying files to ${RPIZ_HOST}"
scp "$RPIZ_BEATSERVER_TGZ" "$INSTALL_SCRIPT" "${RPIZ_USERNAME}@${RPIZ_HOST}:${REMOTE_HOME}" > /dev/null
ok

# --- Clean up local tar ---
info "Deleting local tar file"
rm "$RPIZ_BEATSERVER_TGZ"
ok

# --- Remote install ---
info "Installing and restarting service on ${RPIZ_HOST}"

if out=$(ssh "${RPIZ_USERNAME}@${RPIZ_HOST}" "bash ${REMOTE_HOME}/install-server-on-raspberry-pi.sh ${RPIZ_HOST} ${RPIZ_BEATSERVER_TGZ}" 2>&1); then
  ok
else
  fail
  echo ""
  echo "Error while starting the service on ${RPIZ_HOST}:"
  echo -e "$out"
  exit 1
fi

echo ""
echo "==> Deploy complete!"
echo "    Server running at https://${RPIZ_HOST}:8443/"

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo ""
  echo "Note: Safari does not accept self-signed certificates — use Chrome instead."
  python3 -m webbrowser "https://${RPIZ_HOST}:8443/"
fi
