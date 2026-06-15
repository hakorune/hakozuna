#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

HZ6_CALLOC_REAL_TARGET_CFLAGS=()
hz6_preload_profile_selected_cflags HZ6_CALLOC_REAL_TARGET_CFLAGS 1
hz6_preload_replace_define HZ6_CALLOC_REAL_TARGET_CFLAGS \
  HZ6_PRELOAD_CALLOC_REAL_FALLBACK_L1 1
hz6_preload_replace_define HZ6_CALLOC_REAL_TARGET_CFLAGS \
  HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1 1

hz6_preload_profile_build HZ6_CALLOC_REAL_TARGET_CFLAGS \
  "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_calloc_real_target"
