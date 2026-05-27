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
  "${HZ6_DIR}/fronts/toy"
  "${HZ6_DIR}/policy"
  "${HZ6_DIR}/api"
)

HZ6_SOURCES=(
  "${HZ6_DIR}/api/hz6_allocator.c"
  "${HZ6_DIR}/frontcache/hz6_frontcache.c"
  "${HZ6_DIR}/frontcache/hz6_size_class.c"
  "${HZ6_DIR}/fronts/hz6_front.c"
  "${HZ6_DIR}/fronts/hz6_front_util.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front.c"
  "${HZ6_DIR}/fronts/toy/hz6_toy_front.c"
  "${HZ6_DIR}/policy/hz6_profiles.c"
  "${HZ6_DIR}/route/hz6_route.c"
  "${HZ6_DIR}/source/hz6_source.c"
  "${HZ6_DIR}/transfer/hz6_transfer.c"
  "${HZ6_DIR}/tests/hz6_r1_contract_smoke.c"
)

HZ6_INCLUDE_FLAGS=()
for include_dir in "${HZ6_INCLUDES[@]}"; do
  HZ6_INCLUDE_FLAGS+=("-I${include_dir}")
done

"$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 \
  "${HZ6_INCLUDE_FLAGS[@]}" \
  "${HZ6_SOURCES[@]}" \
  -o "${OUT_DIR}/hz6_r1_contract_smoke"

"${OUT_DIR}/hz6_r1_contract_smoke"
