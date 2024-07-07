#!/bin/sh

set -x
set -e

rm -rf out
docker build -f docker/Dockerfile.beatled . --progress=plain -o out
