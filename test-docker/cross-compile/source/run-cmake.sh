#!/bin/sh

cmake --version

echo "\n\n --- Build 0\n"
cmake -G Ninja -B ../build -S .

echo "\n\n --- Build 1\n"
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake -B ../build1 -S .

echo "\n\n --- Build 2\n"
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET}  -B ../build2 -S .

echo "\n\n --- Build 3\n"
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET} -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=${VCPKG_HOME}/scripts/toolchains/linux.cmake -B ../build3 -S .

echo "\n\n --- Build with docker-toolchain\n"
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=/home/source/cmake/docker-toolchain.cmake -DVCPKG_DIR=${VCPKG_HOME} -DVCPKG_TARGET_TRIPLET=${TARGET_TRIPLET}  -B ../build4 -S .