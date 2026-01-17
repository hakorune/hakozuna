#!/usr/bin/env bash
#
# S44 A/B Test (ABBA ordering) for hz3-scale.
#
# - Builds two variants once (S44=1 and S44=0) and copies the .so files into
#   a run directory under bench_results/, then runs ABBA without rebuilding.
# - Uses HZ3_LDPRELOAD_DEFS_EXTRA (never edits Makefile).
#
# Usage:
#   ./hakozuna/hz3/scripts/run_s44_abba_test.sh
#
# Optional env:
#   RUNS=30                        # total runs per setting (must be even)
#   BENCH_CONFIGS="...;..."        # semicolon-separated bench args
#   BENCH_NAMES="...;..."          # semicolon-separated names (same count)
#
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
HZ3_MAKE_DIR="$ROOT/hakozuna/hz3"
BENCH_BIN="$ROOT/hakozuna/out/bench_random_mixed_mt_remote_malloc"
RUNS="${RUNS:-30}"

if (( RUNS % 2 != 0 )); then
  echo "RUNS must be even (got: $RUNS)" >&2
  exit 2
fi

DEFAULT_CONFIGS="32 5000000 400 16 2048 50;32 5000000 400 16 32768 50"
DEFAULT_NAMES="T32_R50_16-2048;T32_R50_16-32768"
BENCH_CONFIGS="${BENCH_CONFIGS:-$DEFAULT_CONFIGS}"
BENCH_NAMES="${BENCH_NAMES:-$DEFAULT_NAMES}"

IFS=';' read -r -a CONFIG_ARR <<< "$BENCH_CONFIGS"
IFS=';' read -r -a NAME_ARR <<< "$BENCH_NAMES"
if (( ${#CONFIG_ARR[@]} != ${#NAME_ARR[@]} )); then
  echo "BENCH_CONFIGS and BENCH_NAMES count mismatch (${#CONFIG_ARR[@]} vs ${#NAME_ARR[@]})" >&2
  exit 2
fi

RUN_DIR="$ROOT/bench_results/hz3/s44_abba_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RUN_DIR"

RAW_LOG="$RUN_DIR/raw.log"
META_LOG="$RUN_DIR/meta.txt"

git -C "$ROOT" rev-parse HEAD >"$RUN_DIR/git.txt" 2>/dev/null || true

echo "RUN_DIR=$RUN_DIR" | tee -a "$META_LOG"
echo "RUNS=$RUNS" | tee -a "$META_LOG"
echo "BENCH_BIN=$BENCH_BIN" | tee -a "$META_LOG"
echo "BENCH_CONFIGS=$BENCH_CONFIGS" | tee -a "$META_LOG"
echo "BENCH_NAMES=$BENCH_NAMES" | tee -a "$META_LOG"

build_variant() {
  local s44="$1"
  local out_so="$2"

  echo "[BUILD] S44=$s44 -> $out_so"
  make -C "$HZ3_MAKE_DIR" clean all_ldpreload_scale \
    HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_S44_OWNER_STASH=$s44" >/dev/null
  cp -f "$ROOT/libhakozuna_hz3_scale.so" "$out_so"
}

extract_ops() {
  sed -n 's/.*ops\/s=\([0-9.][0-9.]*\).*/\1/p' | head -1
}

extract_failed() {
  # Prints 0 when not found.
  local v
  v="$(sed -n 's/.*alloc_failed[=:][[:space:]]*\([0-9][0-9]*\).*/\1/p' | head -1 || true)"
  if [[ -z "${v:-}" ]]; then
    echo "0"
  else
    echo "$v"
  fi
}

run_bench() {
  local so="$1"
  shift
  local output ops failed
  output="$(LD_PRELOAD="$so" "$BENCH_BIN" "$@" 2>&1 || true)"
  ops="$(printf "%s\n" "$output" | extract_ops)"
  failed="$(printf "%s\n" "$output" | extract_failed)"
  if [[ -z "${ops:-}" ]]; then
    echo "failed to parse ops/s from output:" >&2
    printf "%s\n" "$output" >&2
    exit 3
  fi
  printf "%s %s\n" "$ops" "$failed"
}

SO_S44_1="$RUN_DIR/libhakozuna_hz3_scale_s44_1.so"
SO_S44_0="$RUN_DIR/libhakozuna_hz3_scale_s44_0.so"

build_variant 1 "$SO_S44_1"
build_variant 0 "$SO_S44_0"

SETS=$((RUNS / 2))

echo "==========================================" | tee -a "$RAW_LOG"
echo "S44 A/B ABBA ordering" | tee -a "$RAW_LOG"
echo "RUNS=$RUNS per setting (SETS=$SETS)" | tee -a "$RAW_LOG"
echo "==========================================" | tee -a "$RAW_LOG"

for idx in "${!CONFIG_ARR[@]}"; do
  name="${NAME_ARR[$idx]}"
  config="${CONFIG_ARR[$idx]}"
  echo "" | tee -a "$RAW_LOG"
  echo "=== Benchmark: $name ===" | tee -a "$RAW_LOG"
  echo "Config: $config" | tee -a "$RAW_LOG"

  for set_i in $(seq 1 "$SETS"); do
    echo "--- Set $set_i/$SETS ---" | tee -a "$RAW_LOG"

    # A: S44=1
    read -r ops failed < <(run_bench "$SO_S44_1" $config)
    echo "S44=1 $ops $failed" | tee -a "$RAW_LOG"

    # B: S44=0
    read -r ops failed < <(run_bench "$SO_S44_0" $config)
    echo "S44=0 $ops $failed" | tee -a "$RAW_LOG"

    # B: S44=0
    read -r ops failed < <(run_bench "$SO_S44_0" $config)
    echo "S44=0 $ops $failed" | tee -a "$RAW_LOG"

    # A: S44=1
    read -r ops failed < <(run_bench "$SO_S44_1" $config)
    echo "S44=1 $ops $failed" | tee -a "$RAW_LOG"
  done
done

echo "" | tee -a "$RAW_LOG"
echo "Raw log: $RAW_LOG"
