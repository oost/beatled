#!/bin/bash

SSID="Livebox-98D4"
HOTSPOT_CON="hotspot"
TIMEOUT=30

echo "Trying to connect to $SSID..."
nmcli con up "$SSID" --timeout $TIMEOUT

if [ $? -eq 0 ]; then
    echo "Connected to home WiFi."
else
    echo "Failed. Starting hotspot..."
    nmcli con up "$HOTSPOT_CON"
fi