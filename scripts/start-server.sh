#!/bin/sh

set -x

SCRIPT_PATH=$(dirname "$0")
# IP4_ADDRESS=$(ip -o -4 addr list wlan0 | awk '{print $4}' | cut -d/ -f1)
IP4_ADDRESS="0.0.0.0"

$SCRIPT_PATH/../server/beat_server -a $IP4_ADDRESS --root-dir $SCRIPT_PATH/../client/ --certs-dir /home/${USER}/certs/ --start-http --start-udp

