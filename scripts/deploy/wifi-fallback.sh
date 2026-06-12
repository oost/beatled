#!/bin/bash
#
# Bring up a known NetworkManager connection profile; if it can't connect
# within the timeout, activate a hotspot profile instead. Both profiles
# must already exist (see `nmcli con show`).
#
set -uo pipefail

CONNECTION="${1:?usage: wifi-fallback.sh <wifi-connection> <hotspot-connection>}"
HOTSPOT_CON="${2:?usage: wifi-fallback.sh <wifi-connection> <hotspot-connection>}"
TIMEOUT="${TIMEOUT:-30}"

echo "Trying to connect to '$CONNECTION' (timeout ${TIMEOUT}s)..."
if nmcli --wait "$TIMEOUT" con up "$CONNECTION"; then
    echo "Connected to '$CONNECTION'."
else
    echo "Failed. Starting hotspot '$HOTSPOT_CON'..."
    nmcli con up "$HOTSPOT_CON"
fi
