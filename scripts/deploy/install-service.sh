#!/bin/bash
#
# Install the beat-server, wifi-fallback, and avahi-alias systemd services
# on a Raspberry Pi. Each service file is generated from its template
# (envsubst) and then enabled and started.
#
# The avahi-alias service publishes an extra mDNS name so the Pi answers to
# <MDNS_ALIAS> in addition to <hostname>.local. Override the name via the
# environment (or controller/.env.wifi): MDNS_ALIAS (default beatled.local).
#
# The wifi-fallback service brings up known NetworkManager connections on
# boot — the WiFi networks defined once in controller/.env.wifi, tried in
# order — and falls back to a hotspot if none connect. Each network is
# activated by profile name, so a NetworkManager profile must already
# exist whose name matches the SSID (the default when created via
# `nmcli dev wifi connect "<SSID>"`).
#
#   controller/.env.wifi    WIFI_SSID[_2..4]  the networks to try, in order
#   HOTSPOT_CON (env var)   hotspot profile name   (default: beatled-hotspot)
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
# Pull the WiFi networks from the single shared source of truth, the same
# file the controller firmware builds read. Each non-empty SSID becomes a
# NetworkManager connection name the host tries in order. The file may also
# set HOTSPOT_CON to override the hotspot profile name.
WIFI_ENV="$ROOT_DIR/controller/.env.wifi"
if [[ -f "$WIFI_ENV" ]]; then
  # shellcheck disable=SC1090
  source "$WIFI_ENV"
fi
# Defaults only apply if neither the environment nor .env.wifi set them.
export HOTSPOT_CON="${HOTSPOT_CON:-beatled-hotspot}"
export MDNS_ALIAS="${MDNS_ALIAS:-beatled.local}"
: "${WIFI_SSID:=}"
: "${WIFI_SSID_2:=}"
: "${WIFI_SSID_3:=}"
: "${WIFI_SSID_4:=}"

# Build a systemd-quoted argument list (e.g. "Net A" "Net B") so SSIDs
# with spaces survive the ExecStart=, then template it into the unit.
export WIFI_CONNECTIONS=""
for ssid in "$WIFI_SSID" "$WIFI_SSID_2" "$WIFI_SSID_3" "$WIFI_SSID_4"; do
  [[ -n "$ssid" ]] && WIFI_CONNECTIONS+="\"$ssid\" "
done
WIFI_CONNECTIONS="${WIFI_CONNECTIONS% }"

if [[ -z "$WIFI_CONNECTIONS" ]]; then
  error "No WIFI_SSID found in $WIFI_ENV — wifi-fallback will start the hotspot immediately"
fi

# Generate a service file from its template and install it under
# /etc/systemd/system/. The second argument controls activation:
#   restart  (default) — enable and (re)start the service now
#   enable   — enable for next boot but DON'T start it now. Used for
#              wifi-fallback: starting it re-activates the WiFi link, which
#              would drop an SSH session deploying over that same link. It's
#              a boot-time connectivity guard, so next boot is the right time.
install_service() {
  local name="$1"
  local activation="${2:-restart}"
  local template="$SCRIPT_DIR/${name}.template.service"
  local generated="$SCRIPT_DIR/${name}.service"

  info "Installing ${name} service"
  envsubst < "$template" > "$generated"

  sudo cp "$generated" "/etc/systemd/system/${name}.service"
  sudo chmod 644 "/etc/systemd/system/${name}.service"

  sudo systemctl daemon-reload
  sudo systemctl enable "${name}.service"

  if [[ "$activation" == "restart" ]]; then
    sudo systemctl restart "${name}.service"
    info "${name} status:"
    systemctl status --no-pager "${name}.service" || true
  else
    info "${name} enabled (not started now); takes effect on next boot"
  fi
}

info "Installing services (ROOT_DIR=$ROOT_DIR, networks=[${WIFI_CONNECTIONS:-none}], HOTSPOT_CON=$HOTSPOT_CON, MDNS_ALIAS=$MDNS_ALIAS)"

# These are executed by their services / the API; make sure they're runnable.
chmod +x "$SCRIPT_DIR/wifi-fallback.sh" "$SCRIPT_DIR/avahi-alias.sh" "$SCRIPT_DIR/ap-mode.sh"

# polkit rule: lets the service user activate NetworkManager connections
# (wifi-fallback at boot, ap-mode on demand) without sudo or an interactive
# session. polkitd watches rules.d and reloads automatically.
info "Installing polkit rule for NetworkManager control (user $USERNAME)"
envsubst < "$SCRIPT_DIR/beatled-network.rules.template" > "$SCRIPT_DIR/beatled-network.rules"
sudo cp "$SCRIPT_DIR/beatled-network.rules" /etc/polkit-1/rules.d/50-beatled-network.rules
sudo chmod 644 /etc/polkit-1/rules.d/50-beatled-network.rules

install_service beat-server
install_service wifi-fallback enable
# Publishing an mDNS record doesn't touch the network link, so it's safe to
# start now (unlike wifi-fallback, which would bounce the deploy's WiFi).
install_service avahi-alias
