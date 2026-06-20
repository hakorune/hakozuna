#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_benchmark}"
CC_BIN="${CC:-cc}"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-}"
HZ6_BENCHMARK_USE_SELECTED_FLAGS="${HZ6_BENCHMARK_USE_SELECTED_FLAGS:-0}"
HZ6_BENCHMARK_ENABLE_MIDPAGE_BOUNDARY="${HZ6_BENCHMARK_ENABLE_MIDPAGE_BOUNDARY:-1}"

mkdir -p "$OUT_DIR"

source "${HZ6_DIR}/linux/hz6_sources.sh"
source "${HZ6_DIR}/linux/hz6_preload_flags.sh"
HZ6_INCLUDES=("${HZ6_COMMON_INCLUDES[@]}" "${HZ6_DIR}/bench")

BENCH_SOURCE="${HZ6_DIR}/bench/hz6_allocator_bench.c"
BENCH_BIN="${OUT_DIR}/hz6_allocator_bench"

HZ6_INCLUDE_FLAGS=()
for include_dir in "${HZ6_INCLUDES[@]}"; do
  HZ6_INCLUDE_FLAGS+=("-I${include_dir}")
done

HZ6_EXTRA_CFLAGS_ARRAY=()
if [[ "$HZ6_BENCHMARK_USE_SELECTED_FLAGS" -ne 0 ]]; then
  hz6_preload_effective_selected_cflags HZ6_EXTRA_CFLAGS_ARRAY \
    "$HZ6_BENCHMARK_ENABLE_MIDPAGE_BOUNDARY"
fi
if [[ -n "${HZ6_EXTRA_CFLAGS}" ]]; then
  # shellcheck disable=SC2206
  HZ6_EXTRA_CFLAGS_ARRAY+=(${HZ6_EXTRA_CFLAGS})
fi

command -v "$CC_BIN" >/dev/null 2>&1 || {
  echo "compiler not found in PATH: $CC_BIN" >&2
  exit 1
}

[[ -f "$BENCH_SOURCE" ]] || {
  echo "benchmark source not found: $BENCH_SOURCE" >&2
  exit 1
}

echo "[linux][hz6] building benchmark: ${BENCH_BIN}"
"$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 -D_POSIX_C_SOURCE=200809L \
  "${HZ6_EXTRA_CFLAGS_ARRAY[@]}" \
  "${HZ6_INCLUDE_FLAGS[@]}" \
  "${HZ6_LIB_SOURCES[@]}" \
  "$BENCH_SOURCE" \
  -o "$BENCH_BIN"
echo "[linux][hz6] bench output: ${BENCH_BIN}"
