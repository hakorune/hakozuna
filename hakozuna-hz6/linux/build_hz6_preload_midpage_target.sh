#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_midpage_target}" \
HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-} -DHZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1" \
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
