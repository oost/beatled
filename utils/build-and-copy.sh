#!/bin/sh

docker buildx build --builder=container --output out -f Dockerfile .

utils/copy-tar-files.sh