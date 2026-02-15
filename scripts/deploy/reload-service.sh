#!/bin/bash
#
# Reload or install the beat-server service on a Raspberry Pi.
# Called during deployment to ensure the service is running.
#

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly CERTS_DIR="$SCRIPT_DIR/../../../certs"

info()  { echo "==> $1"; }
error() { echo "==> ERROR: $1" >&2; }

if [[ $# -lt 1 ]]; then
  error "Usage: $0 <domain>"
  exit 1
fi

readonly DOMAIN="$1"

# Ensure certificates exist
info "Checking certificates..."
if [ ! -d "$CERTS_DIR" ]; then
  info "No certificates found, generating..."
  "$SCRIPT_DIR/create-certs.sh" "$DOMAIN" "$CERTS_DIR"
fi
info "Certificates OK"

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
