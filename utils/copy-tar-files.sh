#!/bin/sh

RPIZ_HOST=raspberrypiz.local
RPIZ_BEATSERVER_TGZ=beat-server.tar.gz
cd out 
tar -czvf $RPIZ_BEATSERVER_TGZ .

scp $RPIZ_BEATSERVER_TGZ ${RPIZ_HOST}:/home/pi

ssh ${RPIZ_HOST} << EOF
  cd /home/pi
  rm -rf beat-server 
  mkdir beat-server
  tar -xf $RPIZ_BEATSERVER_TGZ -C beat-server
  rm $RPIZ_BEATSERVER_TGZ
EOF

rm $RPIZ_BEATSERVER_TGZ