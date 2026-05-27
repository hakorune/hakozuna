#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux"
CC_BIN="${CC:-cc}"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

mkdir -p "$OUT_DIR"

HZ6_INCLUDES=(
  "${HZ6_DIR}/include"
  "${HZ6_DIR}/route"
  "${HZ6_DIR}/transfer"
  "${HZ6_DIR}/owner"
  "${HZ6_DIR}/source"
  "${HZ6_DIR}/frontcache"
  "${HZ6_DIR}/fronts"
  "${HZ6_DIR}/fronts/large"
  "${HZ6_DIR}/fronts/local2p"
  "${HZ6_DIR}/fronts/midpage"
  "${HZ6_DIR}/fronts/toy"
  "${HZ6_DIR}/policy"
  "${HZ6_DIR}/api"
)

HZ6_LIB_SOURCES=(
  "${HZ6_DIR}/api/hz6_allocator.c"
  "${HZ6_DIR}/frontcache/hz6_frontcache.c"
  "${HZ6_DIR}/frontcache/hz6_size_class.c"
  "${HZ6_DIR}/fronts/hz6_front.c"
  "${HZ6_DIR}/fronts/hz6_front_util.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front.c"
  "${HZ6_DIR}/fronts/local2p/hz6_local2p_front.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_front.c"
  "${HZ6_DIR}/fronts/toy/hz6_toy_front.c"
  "${HZ6_DIR}/policy/hz6_profiles.c"
  "${HZ6_DIR}/route/hz6_route_backend.c"
  "${HZ6_DIR}/route/hz6_route.c"
  "${HZ6_DIR}/source/linux_source_mmap.c"
  "${HZ6_DIR}/source/hz6_source.c"
  "${HZ6_DIR}/source/hz6_source_registry.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend.c"
  "${HZ6_DIR}/transfer/hz6_transfer.c"
)

HZ6_INCLUDE_FLAGS=()
for include_dir in "${HZ6_INCLUDES[@]}"; do
  HZ6_INCLUDE_FLAGS+=("-I${include_dir}")
done

build_smoke() {
  local test_source="$1"
  local output_name="$2"

  "$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 \
    "${HZ6_INCLUDE_FLAGS[@]}" \
    "${HZ6_LIB_SOURCES[@]}" \
    "$test_source" \
    -o "${OUT_DIR}/${output_name}"

  "${OUT_DIR}/${output_name}"
}

build_smoke "${HZ6_DIR}/tests/hz6_r1_contract_smoke.c" \
  "hz6_r1_contract_smoke"
build_smoke "${HZ6_DIR}/tests/hz6_r1_allocator_smoke.c" \
  "hz6_r1_allocator_smoke"
