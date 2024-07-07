#!/bin/sh

set -e 
set -x

RPIZ_USERNAME=$1
RPIZ_HOST=$2
RPIZ_BEATSERVER_TGZ=beat-server.tar.gz

cd out
rm -f $RPIZ_BEATSERVER_TGZ
tar -czf $RPIZ_BEATSERVER_TGZ *

scp $RPIZ_BEATSERVER_TGZ ${RPIZ_USERNAME}@${RPIZ_HOST}:/home/${RPIZ_USERNAME}

ssh ${RPIZ_USERNAME}@${RPIZ_HOST} << EOF
  cd ~
  rm -rf beat-server 
  mkdir beat-server
  tar -xf $RPIZ_BEATSERVER_TGZ -C beat-server
  rm $RPIZ_BEATSERVER_TGZ
  ./beat-server/scripts/reload-service.sh
  systemctl status beat-server.service
EOF

rm $RPIZ_BEATSERVER_TGZ