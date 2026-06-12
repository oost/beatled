#!/bin/bash
#
# Install the beat-server and wifi-fallback systemd services on a
# Raspberry Pi. Each service file is generated from its template
# (envsubst) and then enabled and started.
#
# The wifi-fallback service brings up a known NetworkManager connection
# on boot and falls back to a hotspot if it can't. Override the
# connection names via the environment:
#
#   WIFI_SSID    NetworkManager connection to bring up   (default: beatled-wifi)
#   HOTSPOT_CON  NetworkManager hotspot connection name  (default: beatled-hotspot)
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
export WIFI_SSID="${WIFI_SSID:-beatled-wifi}"
export HOTSPOT_CON="${HOTSPOT_CON:-beatled-hotspot}"

# Generate a service file from its template, install it under
# /etc/systemd/system/, then enable and start it.
install_service() {
  local name="$1"
  local template="$SCRIPT_DIR/${name}.template.service"
  local generated="$SCRIPT_DIR/${name}.service"

  info "Installing ${name} service"
  envsubst < "$template" > "$generated"

  sudo cp "$generated" "/etc/systemd/system/${name}.service"
  sudo chmod 644 "/etc/systemd/system/${name}.service"

  sudo systemctl daemon-reload
  sudo systemctl enable "${name}.service"
  sudo systemctl restart "${name}.service"

  info "${name} status:"
  systemctl status --no-pager "${name}.service" || true
}

info "Installing services (ROOT_DIR=$ROOT_DIR, WIFI_SSID=$WIFI_SSID, HOTSPOT_CON=$HOTSPOT_CON)"

# wifi-fallback.sh is executed by the service; make sure it's runnable.
chmod +x "$SCRIPT_DIR/wifi-fallback.sh"

install_service beat-server
install_service wifi-fallback
