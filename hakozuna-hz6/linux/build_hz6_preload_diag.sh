#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_diag}" \
HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-} -DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1" \
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
