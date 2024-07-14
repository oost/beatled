#!/bin/bash

# set -x
# set -e

DOMAIN=$1

echo -n "- Checking certificates"
if [ ! -d $(dirname "$0")/../../certs ]; then
  echo " ⛔️"
  echo -n "- Creating certificates"
  $(dirname "$0")/create-certs.sh $DOMAIN
fi
echo " ✅"

echo -n "- Checking service status"
systemctl status --no-pager beat-server.service > /dev/null
if [[ $? == 4 ]]; then 
  echo " ⛔️"
  echo " - Beat server service was not found. Installing..."
  . $(dirname "$0")/install-service.sh
else
  echo " ✅"
  echo -n "- Reloading service daemons"
  sudo systemctl daemon-reload
  echo " ✅"
  echo -n " - Restarting beat-service"
  sudo systemctl restart beat-server.service
  echo " ✅"
  systemctl status --no-pager beat-server.service
fi