#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

HZ6_CROSS128_TOY2_SPLIT_CFLAGS=()
hz6_preload_profile_owner_inbox_toy2_split_cflags \
  HZ6_CROSS128_TOY2_SPLIT_CFLAGS 1

hz6_preload_profile_build \
  HZ6_CROSS128_TOY2_SPLIT_CFLAGS \
  "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_cross128_toy2_split"
