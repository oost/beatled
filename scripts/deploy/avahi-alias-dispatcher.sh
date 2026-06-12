#!/bin/bash
#
# NetworkManager dispatcher: re-publish the mDNS alias after the host's IP
# changes. avahi-alias.service holds a *static* A record (avahi-publish -a
# <name> <ip>), captured once at startup — so when the address changes (boot,
# ap-mode on/off, wifi-fallback, a DHCP renewal) it keeps advertising the old
# IP until restarted. NM runs this as root on every connectivity transition,
# so restarting the unit here needs no sudo/polkit.
#
# NetworkManager invokes dispatcher scripts as:  <script> <interface> <action>
#
# Installed (by install-service.sh) to
# /etc/NetworkManager/dispatcher.d/90-beatled-avahi-alias, root-owned and not
# group/world-writable — NM silently ignores scripts that aren't.
set -u

action="${2:-}"

case "$action" in
  up | dhcp4-change | dhcp6-change | connectivity-change)
    # --no-block so we don't stall NM's dispatcher run; restart (not
    # try-restart) so the alias comes back even if it had failed/stopped.
    # Restarting a unit isn't a network event, so this can't loop.
    systemctl --no-block restart avahi-alias.service
    ;;
esac
