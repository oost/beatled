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
