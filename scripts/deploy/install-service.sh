#!/bin/bash
#
# Install the beat-server systemd service on a Raspberry Pi.
# Generates the service file from the template and enables it.
#

set -euo pipefail

info()  { echo "==> $1"; }
error() { echo "==> ERROR: $1" >&2; }

if ! command -v envsubst &>/dev/null; then
  error "envsubst is not installed (part of gettext)"
  exit 1
fi

# Resolve symlinks to find the real script directory
SCRIPT_PATH="${BASH_SOURCE[0]}"
while [ -L "$SCRIPT_PATH" ]; do
  SCRIPT_DIR="$(cd -P "$(dirname "$SCRIPT_PATH")" >/dev/null 2>&1 && pwd)"
  SCRIPT_PATH="$(readlink "$SCRIPT_PATH")"
  [[ $SCRIPT_PATH != /* ]] && SCRIPT_PATH="$SCRIPT_DIR/$SCRIPT_PATH"
done
SCRIPT_PATH="$(readlink -f "$SCRIPT_PATH")"
readonly SCRIPT_DIR="$(cd -P "$(dirname -- "$SCRIPT_PATH")" >/dev/null 2>&1 && pwd)"

export USERNAME
USERNAME="$(whoami)"
export ROOT_DIR
ROOT_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"

info "Installing beat-server service (ROOT_DIR=$ROOT_DIR)"

readonly SERVICE_FILE="$SCRIPT_DIR/beat-server.service"
envsubst < "$SCRIPT_DIR/beat-server.template.service" > "$SERVICE_FILE"

info "Copying service file to /etc/systemd/system/"
sudo cp "$SERVICE_FILE" /etc/systemd/system/beat-server.service
sudo chmod 644 /etc/systemd/system/beat-server.service

info "Reloading systemd, enabling and starting service..."
sudo systemctl daemon-reload
sudo systemctl enable beat-server.service
sudo systemctl start beat-server.service

info "Service status:"
systemctl status --no-pager beat-server.service
