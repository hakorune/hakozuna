#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

HZ6_MIDPAGE_TRUSTED_CLASS_TARGET_CFLAGS=()
hz6_preload_effective_selected_cflags \
  HZ6_MIDPAGE_TRUSTED_CLASS_TARGET_CFLAGS 1
hz6_preload_replace_define HZ6_MIDPAGE_TRUSTED_CLASS_TARGET_CFLAGS \
  HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 1

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_midpage_trusted_class_target}" \
HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags \
  "${HZ6_MIDPAGE_TRUSTED_CLASS_TARGET_CFLAGS[@]}")" \
  "${HZ6_DIR}/linux/build_hz6_preload.sh"
