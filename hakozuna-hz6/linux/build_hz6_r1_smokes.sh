#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux"
CC_BIN="${CC:-cc}"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

mkdir -p "$OUT_DIR"

source "${HZ6_DIR}/linux/hz6_sources.sh"
HZ6_INCLUDES=("${HZ6_COMMON_INCLUDES[@]}")

HZ6_SMOKES=(
  "tests/hz6_r1_core_contract_smoke.c:hz6_r1_core_contract_smoke"
  "tests/hz6_r1_route_smoke.c:hz6_r1_route_smoke"
  "tests/hz6_r1_transfer_contract_smoke.c:hz6_r1_transfer_contract_smoke"
  "tests/hz6_r1_source_contract_smoke.c:hz6_r1_source_contract_smoke"
  "tests/hz6_r1_allocator_smoke.c:hz6_r1_allocator_smoke"
  "tests/hz6_r1_prefill_smoke.c:hz6_r1_prefill_smoke"
  "tests/hz6_r1_sourceblock_smoke.c:hz6_r1_sourceblock_smoke"
  "tests/hz6_r1_transfer_smoke.c:hz6_r1_transfer_smoke"
  "tests/hz6_r1_reclaim_smoke.c:hz6_r1_reclaim_smoke"
  "tests/hz6_r1_safety_smoke.c:hz6_r1_safety_smoke"
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

for smoke in "${HZ6_SMOKES[@]}"; do
  test_source="${smoke%%:*}"
  output_name="${smoke##*:}"
  build_smoke "${HZ6_DIR}/${test_source}" "$output_name"
done
