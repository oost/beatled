#!/bin/bash
#
# Reload or install the beat-server service on a Raspberry Pi.
# Called during deployment to ensure the service is running.
# Certificates must already exist (handled by cmd_deploy).
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

# Check if service is installed
info "Checking service status..."
if ! systemctl status --no-pager beat-server.service > /dev/null 2>&1; then
  info "Beat server service not found, installing..."
  # shellcheck disable=SC1091
  . "$SCRIPT_DIR/install-service.sh"
else
  info "Reloading systemd daemon..."
  sudo systemctl daemon-reload
  info "Restarting beat-server..."
  sudo systemctl restart beat-server.service
  info "Service status:"
  systemctl status --no-pager beat-server.service
fi
