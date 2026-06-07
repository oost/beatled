#!/bin/sh

REBUILD=${1:-""}

if [ ! -z "$REBUILD"]; then 
  rm -rf build-pico
  mkdir build-pico
fi

cd build-pico 

export PICO_TOOLCHAIN_PATH="/Applications/ArmGNUToolchain/12.2.rel1/arm-none-eabi"
export BEATLED_SERVER_NAME="raspberrypi1.local"

cmake .. -DPORT=pico -DPICO_PROBE=OFF -DPICO_SDK_PATH=../lib/pico-sdk -DPICO_BOARD=pico_w 

cmake --build .