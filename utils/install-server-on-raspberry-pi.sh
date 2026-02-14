#!/bin/bash
#
# Runs ON the Raspberry Pi to install and restart the beatled server.
# Copied and executed remotely by deploy-server-to-raspberry-pi.sh.
#

set -e

if [[ $# -ne 2 ]]; then
  echo "Usage: $(basename "$0") <host> <tarfile>"
  exit 1
fi

HOST="$1"
TARFILE="$2"
INSTALL_DIR="$HOME/beat-server"

echo "Extracting $TARFILE to $INSTALL_DIR"
rm -rf "$INSTALL_DIR"
mkdir "$INSTALL_DIR"
tar -xf "$HOME/$TARFILE" -C "$INSTALL_DIR"

echo "Removing $TARFILE"
rm "$HOME/$TARFILE"

echo "Reloading service"
"$INSTALL_DIR/scripts/reload-service.sh" "$HOST"

echo "Checking service status"
systemctl status beat-server.service
