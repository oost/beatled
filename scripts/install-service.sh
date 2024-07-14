#!/bin/bash

set -e
set -x

export USERNAME=$(whoami)

SCRIPT_PATH="${BASH_SOURCE}"
while [ -L "${SCRIPT_PATH}" ]; do
  SCRIPT_DIR="$(cd -P "$(dirname "${SCRIPT_PATH}")" >/dev/null 2>&1 && pwd)"
  SCRIPT_PATH="$(readlink "${SCRIPT_PATH}")"
  [[ ${SCRIPT_PATH} != /* ]] && SCRIPT_PATH="${SCRIPT_DIR}/${SCRIPT_PATH}"
done
SCRIPT_PATH="$(readlink -f "${SCRIPT_PATH}")"
SCRIPT_DIR="$(cd -P "$(dirname -- "${SCRIPT_PATH}")" >/dev/null 2>&1 && pwd)"

echo "SCRIPT_DIR=${SCRIPT_DIR}"
export ROOT_DIR=$(dirname ${SCRIPT_DIR})

cat ${SCRIPT_DIR}/beat-server.template.service | envsubst > ${SCRIPT_DIR}/beat-server.service

sudo cp ${SCRIPT_DIR}/beat-server.service /etc/systemd/system
sudo chmod 644 /etc/systemd/system/beat-server.service 
sudo systemctl daemon-reload
sudo systemctl start beat-server.service
systemctl status --no-pager  beat-server.service
