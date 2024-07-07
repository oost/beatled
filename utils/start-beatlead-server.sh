#!/bin/sh

set -e
set -x

SCRIPT_PATH="${BASH_SOURCE}"
while [ -L "${SCRIPT_PATH}" ]; do
  SCRIPT_DIR="$(cd -P "$(dirname "${SCRIPT_PATH}")" >/dev/null 2>&1 && pwd)"
  SCRIPT_PATH="$(readlink "${SCRIPT_PATH}")"
  [[ ${SCRIPT_PATH} != /* ]] && SCRIPT_PATH="${SCRIPT_DIR}/${SCRIPT_PATH}"
done
SCRIPT_PATH="$(readlink -f "${SCRIPT_PATH}")"
SCRIPT_DIR="$(cd -P "$(dirname -- "${SCRIPT_PATH}")" >/dev/null 2>&1 && pwd)"

PROJECT_PATH=$(dirname "${SCRIPT_DIR}")

# BIN_PATH="${SCRIPT_DIR}/../server/out/build/Clang/src"
BIN_PATH="${PROJECT_PATH}/build/src"

${BIN_PATH}/beat_server --start-http --root-dir ${PROJECT_PATH}/client/dist --certs-dir ${PROJECT_PATH}/server/certs