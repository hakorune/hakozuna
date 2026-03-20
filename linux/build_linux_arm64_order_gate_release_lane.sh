#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

exec "${ROOT_DIR}/linux/build_linux_release_lane.sh" \
  --arch arm64 \
  --hz4-target all_perf_lib \
  --hz4-extra '-DHZ4_FREE_ROUTE_ORDER_GATEBOX=1'
