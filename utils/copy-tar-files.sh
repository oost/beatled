#!/bin/bash

set -e 
# set -x

RPIZ_USERNAME=$1
RPIZ_HOST=$2
RPIZ_BEATSERVER_TGZ=beat-server.tar.gz

PAD="%-50s"

printf $PAD "Generating tar file"
cd out
rm -f $RPIZ_BEATSERVER_TGZ
tar -czf $RPIZ_BEATSERVER_TGZ *
echo " ✅"

printf $PAD "Copying to files to ${RPIZ_HOST}"
scp $RPIZ_BEATSERVER_TGZ ${RPIZ_USERNAME}@${RPIZ_HOST}:/home/${RPIZ_USERNAME} > /dev/null
echo " ✅"

printf $PAD "Deleting local tar file"
rm $RPIZ_BEATSERVER_TGZ
echo " ✅"

printf $PAD "Running script to launch server"

REMOTE_SCRIPT="cd ~
rm -rf beat-server 
mkdir beat-server
tar -xf $RPIZ_BEATSERVER_TGZ -C beat-server
rm $RPIZ_BEATSERVER_TGZ
./beat-server/scripts/reload-service.sh $RPIZ_HOST
systemctl status beat-server.service"

out=$(ssh ${RPIZ_USERNAME}@${RPIZ_HOST} "$REMOTE_SCRIPT")

if [[ $? == 0 ]]; then 
  echo " ✅"
else
  echo " ⛔️"
  echo "Error while starting the service on ${RPIZ_HOST}:"
  echo -e "$out"
  exit 1
fi

printf $PAD "We're done!!"
echo " 🚀"

printf $PAD "Let's go to https://${RPIZ_HOST}:8080/"
echo " 🌎"
if [[ $(uname -s) == "Darwin" ]]; then 
  echo ""
  echo "🟡 Safari won't accept self-signed certificates..."
  echo "Please try with Chrome instead"
fi

python3 -m webbrowser "https://${RPIZ_HOST}:8080/"
