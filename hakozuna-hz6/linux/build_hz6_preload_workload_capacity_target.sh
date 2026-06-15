#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS=()
hz6_preload_profile_selected_cflags HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS 1
HZ6_WORKLOAD_CAPACITY_OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_capacity_target"

case "${HZ6_WORKLOAD_CAPACITY_LEVEL:-full}" in
  lite)
    HZ6_WORKLOAD_CAPACITY_OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_capacity_lite_target"
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_ROUTE_TABLE_CAPACITY 65536
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_OBJECT_DESCRIPTOR_CAPACITY 16384
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_SOURCE_BLOCK_CAPACITY 2048
    ;;
  mid)
    HZ6_WORKLOAD_CAPACITY_OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_capacity_mid_target"
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_ROUTE_TABLE_CAPACITY 98304
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_OBJECT_DESCRIPTOR_CAPACITY 24576
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_SOURCE_BLOCK_CAPACITY 3072
    ;;
  full)
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_ROUTE_TABLE_CAPACITY 131072
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_OBJECT_DESCRIPTOR_CAPACITY 32768
    hz6_preload_replace_define HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
      HZ6_SOURCE_BLOCK_CAPACITY 4096
    ;;
  *)
    echo "unknown HZ6_WORKLOAD_CAPACITY_LEVEL: ${HZ6_WORKLOAD_CAPACITY_LEVEL}" >&2
    exit 2
    ;;
esac

hz6_preload_profile_build HZ6_WORKLOAD_CAPACITY_TARGET_CFLAGS \
  "$HZ6_WORKLOAD_CAPACITY_OUT_DIR"
