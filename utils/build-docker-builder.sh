#!/bin/sh

set -x
set -e

docker build -f docker/Dockerfile.builder . --progress=plain -t docker-builder
