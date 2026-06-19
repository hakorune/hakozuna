#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

HZ6_TRANSFER_CLASS_SHARD_CFLAGS=()
hz6_preload_profile_selected_cflags HZ6_TRANSFER_CLASS_SHARD_CFLAGS 1
hz6_preload_replace_define \
  HZ6_TRANSFER_CLASS_SHARD_CFLAGS \
  HZ6_PROFILE_TRANSFER_SHARD_CLASS_L1 \
  1

hz6_preload_profile_build \
  HZ6_TRANSFER_CLASS_SHARD_CFLAGS \
  "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_transfer_class_shard"
