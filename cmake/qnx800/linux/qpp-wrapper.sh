#!/usr/bin/env bash
set -euo pipefail
# Ensure QNX SDP environment is loaded for CLion toolchain checks.
source /home/lance/qnx800/qnxsdp-env.sh >/dev/null
exec /home/lance/qnx800/host/linux/x86_64/usr/bin/q++ "$@"

