#!/bin/bash
#
# Generate locally-trusted TLS certificates using mkcert.
# Requires: mkcert (https://github.com/FiloSotto/mkcert)
#   brew install mkcert && mkcert -install
#

set -euo pipefail

CERTS_FOLDER="$HOME/certs"
DOMAINS=()

# Parse arguments: --output <folder> sets the output directory, everything else is a domain
while [[ $# -gt 0 ]]; do
  case "$1" in
    --output)
      CERTS_FOLDER="$2"
      shift 2
      ;;
    *)
      DOMAINS+=("$1")
      shift
      ;;
  esac
done

if [[ ${#DOMAINS[@]} -eq 0 ]]; then
  echo "Error: No domain names provided"
  echo "Usage: $0 [--output <certs_folder>] <domain> [domain2 ...]"
  exit 1
fi

if ! command -v mkcert &>/dev/null; then
  echo "Error: mkcert is not installed"
  echo "Install with: brew install mkcert && mkcert -install"
  exit 1
fi

readonly CERTS_FOLDER
readonly DOMAINS

echo "- Creating certs folder $CERTS_FOLDER"
mkdir -p "$CERTS_FOLDER"

cd "$CERTS_FOLDER"

# Generate cert + key with mkcert (trusted by local system/browsers)
echo "- Generating certificate and key for: ${DOMAINS[*]}"
mkcert -cert-file cert.pem -key-file key.pem "${DOMAINS[@]}"

# Generate DH params (mkcert doesn't do this)
if [ ! -f dh_param.pem ]; then
  echo "- Generating DH params (this takes a moment)"
  openssl dhparam -out dh_param.pem 2048
else
  echo "- DH params already exist, skipping"
fi

echo "- Done! Files in $CERTS_FOLDER:"
ls -la ./*.pem
