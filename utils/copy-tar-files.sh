#!/bin/sh

set -e 
set -x

RPIZ_HOST=raspberrypi1.local
RPIZ_BEATSERVER_TGZ=beat-server.tar.gz

cd out
tar -czf $RPIZ_BEATSERVER_TGZ *

scp $RPIZ_BEATSERVER_TGZ ${RPIZ_HOST}:/home/pi

ssh ${RPIZ_HOST} << EOF
  cd /home/pi
  rm -rf beat-server 
  mkdir beat-server
  tar -xf $RPIZ_BEATSERVER_TGZ -C beat-server
  rm $RPIZ_BEATSERVER_TGZ
  ./beat-server/scripts/reload-service.sh
  systemctl status beat-server.service
EOF

rm $RPIZ_BEATSERVER_TGZ