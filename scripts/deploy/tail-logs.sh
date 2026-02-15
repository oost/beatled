#!/bin/bash
#
# Follow the beat-server service logs.
#

set -euo pipefail

exec journalctl -u beat-server.service -f
