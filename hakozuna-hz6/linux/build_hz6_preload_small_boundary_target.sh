#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

HZ6_SMALL_BOUNDARY_TARGET_CFLAGS=()
hz6_preload_effective_selected_cflags HZ6_SMALL_BOUNDARY_TARGET_CFLAGS 1
hz6_preload_replace_define HZ6_SMALL_BOUNDARY_TARGET_CFLAGS \
  HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 1
hz6_preload_replace_define HZ6_SMALL_BOUNDARY_TARGET_CFLAGS \
  HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1 1
hz6_preload_replace_define HZ6_SMALL_BOUNDARY_TARGET_CFLAGS \
  HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES 4096
hz6_preload_replace_define HZ6_SMALL_BOUNDARY_TARGET_CFLAGS \
  HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_L1 1

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_small_boundary_target}" \
HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags \
  "${HZ6_SMALL_BOUNDARY_TARGET_CFLAGS[@]}")" \
  "${HZ6_DIR}/linux/build_hz6_preload.sh"
