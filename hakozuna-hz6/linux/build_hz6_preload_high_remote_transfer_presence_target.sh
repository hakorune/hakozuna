#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

HZ6_HIGH_REMOTE_TRANSFER_PRESENCE_CFLAGS=()
hz6_preload_profile_high_remote_transfer_presence_cflags \
  HZ6_HIGH_REMOTE_TRANSFER_PRESENCE_CFLAGS 1

hz6_preload_profile_build \
  HZ6_HIGH_REMOTE_TRANSFER_PRESENCE_CFLAGS \
  "${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_high_remote_transfer_presence"
