#!/bin/sh

set -x

sudo cp beat-server/scripts/beat-server.service /etc/systemd/system
sudo chmod 644 /etc/systemd/system/beat-server.service 
sudo systemctl daemon-reload
sudo systemctl start beat-server.service