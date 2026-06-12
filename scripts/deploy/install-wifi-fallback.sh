#!/bin/sh
# sudo chmod +x /usr/local/bin/wifi-fallback.sh
cp ./scripts/deploy/wifi-fallback.service /etc/systemd/system/wifi-fallback.service
sudo systemctl enable wifi-fallback.service
sudo systemctl start wifi-fallback.service