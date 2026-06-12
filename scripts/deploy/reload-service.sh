#!/bin/bash
#
# Reload/install the beat-server and wifi-fallback services on a
# Raspberry Pi. Called during deployment to ensure both services are
# installed, up to date, and running. Certificates must already exist
# (handled by cmd_deploy).
#

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

info()  { echo "==> $1"; }
error() { echo "==> ERROR: $1" >&2; }

# Verify certificates are present
if [ ! -f "$HOME/certs/cert.pem" ]; then
  error "Certificates not found at $HOME/certs/"
  error "Certificates should be copied during deployment"
  exit 1
fi

# install-service.sh is idempotent: it regenerates each unit file from its
# template, reloads systemd, and enables + restarts beat-server and
# wifi-fallback. Running it on every deploy keeps both services current
# (the old "only install when missing" path never refreshed wifi-fallback).
# Run as a subprocess, not `.` — install-service.sh is self-contained and
# also declares `readonly SCRIPT_DIR`, which would collide if sourced.
info "Installing/refreshing beat-server and wifi-fallback services..."
bash "$SCRIPT_DIR/install-service.sh"
