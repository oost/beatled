#!/bin/bash
#
# Try a list of NetworkManager connection profiles in order; if none come
# up within the timeout, activate a hotspot profile instead. All profiles
# must already exist (see `nmcli con show`); the WiFi profile names are
# the SSIDs from controller/.env.wifi, wired in by install-service.sh.
#
# usage: wifi-fallback.sh <hotspot-connection> [<wifi-connection> ...]
#
set -uo pipefail

HOTSPOT_CON="${1:?usage: wifi-fallback.sh <hotspot-connection> [<wifi-connection> ...]}"
shift
TIMEOUT="${TIMEOUT:-30}"

# On a cold boot the Wi-Fi radio (wlan0) is often not enumerated/managed yet
# when this unit runs — ordering After=NetworkManager.service only guarantees
# the daemon is up, not that the device exists. `nmcli con up` then fails fast
# with "no suitable device found (device eth0 not available ...)" because the
# only managed device is eth0, and --wait does NOT cover this case (it waits
# for an *existing* device to activate, not for one to appear). Without this
# guard we'd skip every Wi-Fi profile and drop straight to the hotspot. Wait
# for a Wi-Fi device to leave the 'unavailable' state before trying profiles.
echo "Waiting for a Wi-Fi device to become available (timeout ${TIMEOUT}s)..."
for _ in $(seq 1 "$TIMEOUT"); do
    wifi_state="$(nmcli -t -f TYPE,STATE dev status | awk -F: '$1=="wifi"{print $2; exit}')"
    [[ -n "$wifi_state" && "$wifi_state" != "unavailable" ]] && break
    sleep 1
done
if [[ -z "${wifi_state:-}" || "$wifi_state" == "unavailable" ]]; then
    echo "No Wi-Fi device ready after ${TIMEOUT}s; proceeding anyway." >&2
fi

for con in "$@"; do
    echo "Trying to connect to '$con' (timeout ${TIMEOUT}s)..."
    if nmcli --wait "$TIMEOUT" con up "$con"; then
        echo "Connected to '$con'."
        exit 0
    fi
    echo "Failed to connect to '$con'."
done

echo "No WiFi network connected. Starting hotspot '$HOTSPOT_CON'..."
exec nmcli con up "$HOTSPOT_CON"
