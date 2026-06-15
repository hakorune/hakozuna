#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

HZ6_REALLOC_BOUNDARY_8K_TARGET_CFLAGS=()
hz6_preload_profile_selected_cflags HZ6_REALLOC_BOUNDARY_8K_TARGET_CFLAGS 1
hz6_preload_replace_define HZ6_REALLOC_BOUNDARY_8K_TARGET_CFLAGS \
  HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1 1

hz6_preload_profile_build HZ6_REALLOC_BOUNDARY_8K_TARGET_CFLAGS \
  "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_realloc_boundary_8k_target"
