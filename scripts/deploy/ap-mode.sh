#!/bin/bash
#
# Toggle the Pi's WiFi between client mode and access-point (hotspot) mode.
# The Pi has a single radio, so AP and client are mutually exclusive:
# turning the AP on disconnects upstream WiFi, and vice versa.
#
#   ap-mode.sh on [minutes]   Activate the hotspot AP. With an optional
#                             integer <minutes>, schedule an automatic switch
#                             back to WiFi after that long — handy for testing
#                             so a bad switch can't strand the Pi headless.
#   ap-mode.sh off            Reconnect to the primary upstream WiFi
#                             (also cancels any pending auto-revert).
#   ap-mode.sh status         Print "on" or "off".
#
# Profile names come from the shared controller/.env.wifi (HOTSPOT_CON,
# WIFI_SSID). Activating connections uses NetworkManager's network-control
# polkit action, granted to the service user by the deploy
# (scripts/deploy/beatled-network.rules.template), so no sudo is needed.
#
set -uo pipefail

SCRIPT_DIR="$(cd -P "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"
SELF="$SCRIPT_DIR/$(basename -- "${BASH_SOURCE[0]}")"

WIFI_ENV="$ROOT_DIR/controller/.env.wifi"
if [[ -f "$WIFI_ENV" ]]; then
  # shellcheck disable=SC1090
  source "$WIFI_ENV"
fi
HOTSPOT_CON="${HOTSPOT_CON:-beatled-hotspot}"
WIFI_SSID="${WIFI_SSID:-}"

# Pending auto-revert is tracked here. ROOT_DIR is the one writable path under
# the server's systemd sandbox (ReadWritePaths=$ROOT_DIR), so /tmp is not an
# option when ap-mode is invoked via the server.
REVERT_MARKER="$ROOT_DIR/.ap-revert.pid"

is_ap_active() {
  nmcli -t -f NAME con show --active 2>/dev/null | grep -qxF "$HOTSPOT_CON"
}

cancel_revert() {
  [[ -f "$REVERT_MARKER" ]] || return 0
  local pid
  pid="$(cat "$REVERT_MARKER" 2>/dev/null || true)"
  if [[ "$pid" =~ ^[0-9]+$ ]]; then
    # setsid made the sleeper a process-group leader; signal the whole group
    # (negative pid) so its sleep child dies with it.
    kill -TERM -- "-${pid}" 2>/dev/null || true
  fi
  rm -f "$REVERT_MARKER"
}

schedule_revert() {
  local minutes="$1"
  cancel_revert
  # Detach fully (setsid + closed/redirected fds) so the timer survives this
  # script exec'ing into nmcli, the SSH session dropping, and the HTTP
  # connection closing. It removes the marker *before* invoking off, so off's
  # own cancel_revert doesn't kill the revert run itself.
  REVERT_SECS="$((minutes * 60))" REVERT_MARKER="$REVERT_MARKER" SELF="$SELF" \
    setsid bash -c 'echo $$ > "$REVERT_MARKER"; sleep "$REVERT_SECS"; rm -f "$REVERT_MARKER"; exec "$SELF" off' \
    </dev/null >>"$ROOT_DIR/.ap-revert.log" 2>&1 &
  echo "Auto-revert to WiFi scheduled in ${minutes} min."
}

case "${1:-status}" in
  on)
    revert="${2:-0}"
    if [[ ! "$revert" =~ ^[0-9]+$ ]]; then
      echo "revert minutes must be a non-negative integer" >&2
      exit 1
    fi
    if ((revert > 0)); then
      schedule_revert "$revert"
    fi
    echo "Activating AP '$HOTSPOT_CON' (this drops upstream WiFi)..."
    exec nmcli con up "$HOTSPOT_CON"
    ;;
  off)
    cancel_revert
    if [[ -z "$WIFI_SSID" ]]; then
      echo "No WIFI_SSID configured; bringing the AP down instead." >&2
      exec nmcli con down "$HOTSPOT_CON"
    fi
    echo "Reconnecting to upstream WiFi '$WIFI_SSID'..."
    exec nmcli con up "$WIFI_SSID"
    ;;
  status)
    if is_ap_active; then echo "on"; else echo "off"; fi
    ;;
  *)
    echo "usage: ap-mode.sh <on [minutes]|off|status>" >&2
    exit 1
    ;;
esac
