#!/bin/sh

set -x
set -e

docker buildx build --builder=container --output out -f Dockerfile .

utils/copy-tar-files.sh