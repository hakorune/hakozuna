#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_midpage_target}" \
HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-} -DHZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1 -DHZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=1" \
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
