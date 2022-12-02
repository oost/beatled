#!/bin/sh

set -x
set -e

docker buildx build --platform linux/arm64 --builder=mybuilder --output out -f Dockerfile . --progress=plain

utils/copy-tar-files.sh