#!/bin/bash
#
# Publish an extra mDNS .local name for this host so it answers to more
# than just <hostname>.local. Runs in the foreground (avahi-publish holds
# the registration for as long as it lives), so the systemd unit uses
# Type=simple with Restart=always.
#
# usage: avahi-alias.sh <alias.local>
#
set -uo pipefail

ALIAS="${1:?usage: avahi-alias.sh <alias.local>}"

# avahi-publish -a registers an A record, so it needs the current IP. Take
# the first non-loopback address (wlan0 / eth0). If the lease changes, the
# unit is restarted (Restart=always) and re-resolves.
IP="$(hostname -I | awk '{print $1}')"
if [[ -z "$IP" ]]; then
  echo "avahi-alias: no IP address yet for $ALIAS" >&2
  exit 1
fi

# -R / --no-reverse: don't publish a reverse PTR for the address. The
# host's primary name (<hostname>.local) already owns the PTR for this IP,
# so publishing another would collide ("Local name collision"). We only
# need the forward name -> address record for the alias.
echo "Publishing mDNS alias $ALIAS -> $IP"
exec avahi-publish -a -R "$ALIAS" "$IP"
