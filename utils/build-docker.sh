#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "==> Building Docker builder image..."
docker build -f docker/Dockerfile.builder . --progress=plain -t docker-builder

echo "==> Building beatled server (ARM64)..."
rm -rf out
docker build -f docker/Dockerfile.beatled . --progress=plain -o out

echo "==> Build complete. Output in ./out/"
