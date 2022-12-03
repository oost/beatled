#!/bin/sh

rm -rf ../build

cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=/home/source/docker-toolchain.cmake -DVCPKG_DIR=${VCPKG_HOME}  -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET} -B ../build -S .

cmake --build ../build/