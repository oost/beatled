#!/bin/sh

set -x
set -e

echo "Building dockcross builder image..."
docker build -f docker/Dockerfile.dockcross . --progress=plain -t dockcross-builder

echo "Building beatled with dockcross..."
rm -rf out
docker build -f docker/Dockerfile.beatled-dockcross . --progress=plain -o out

echo "Build complete. Output in ./out/"
