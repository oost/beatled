#!/bin/sh

set -x

journalctl -u beat-server.service -f
