#!/bin/sh

docker buildx build --builder=container --output out -f Dockerfile.arm64 .

utils/copy-tar-files.sh