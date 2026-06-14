#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_midpage_boundary_control}" \
HZ6_PRELOAD_ENABLE_MIDPAGE_BOUNDARY=0 \
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
