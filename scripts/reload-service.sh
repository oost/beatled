#!/bin/sh

set -x

sudo systemctl daemon-reload
sudo systemctl restart beat-server.service
systemctl status beat-server.service