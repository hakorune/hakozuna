#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux"
CC_BIN="${CC:-cc}"

mkdir -p "$OUT_DIR"

"$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 \
  -I"${ROOT_DIR}/hakozuna-hz6/include" \
  -I"${ROOT_DIR}/hakozuna-hz6/route" \
  -I"${ROOT_DIR}/hakozuna-hz6/transfer" \
  -I"${ROOT_DIR}/hakozuna-hz6/owner" \
  -I"${ROOT_DIR}/hakozuna-hz6/source" \
  -I"${ROOT_DIR}/hakozuna-hz6/frontcache" \
  -I"${ROOT_DIR}/hakozuna-hz6/policy" \
  -I"${ROOT_DIR}/hakozuna-hz6/api" \
  "${ROOT_DIR}/hakozuna-hz6/api/hz6_allocator.c" \
  "${ROOT_DIR}/hakozuna-hz6/frontcache/hz6_frontcache.c" \
  "${ROOT_DIR}/hakozuna-hz6/frontcache/hz6_size_class.c" \
  "${ROOT_DIR}/hakozuna-hz6/fronts/hz6_front.c" \
  "${ROOT_DIR}/hakozuna-hz6/fronts/hz6_front_util.c" \
  "${ROOT_DIR}/hakozuna-hz6/fronts/large/hz6_large128_front.c" \
  "${ROOT_DIR}/hakozuna-hz6/fronts/toy/hz6_toy_front.c" \
  "${ROOT_DIR}/hakozuna-hz6/policy/hz6_profiles.c" \
  "${ROOT_DIR}/hakozuna-hz6/route/hz6_route.c" \
  "${ROOT_DIR}/hakozuna-hz6/source/hz6_source.c" \
  "${ROOT_DIR}/hakozuna-hz6/transfer/hz6_transfer.c" \
  "${ROOT_DIR}/hakozuna-hz6/tests/hz6_r1_contract_smoke.c" \
  -o "${OUT_DIR}/hz6_r1_contract_smoke"

"${OUT_DIR}/hz6_r1_contract_smoke"
