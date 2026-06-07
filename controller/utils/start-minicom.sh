#!/bin/sh

DEVICE=$(ls /dev | grep tty.usbmodem | head -1)

if [ -z "$DEVICE" ]; then 
  echo "Device not found..."
  exit 1
fi

echo "Connecting to device: ${DEVICE}"

minicom -D /dev/${DEVICE} -b 115200
