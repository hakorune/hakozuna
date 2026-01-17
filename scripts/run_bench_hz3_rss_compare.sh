#!/usr/bin/env bash
set -euo pipefail

# S53-3: RSS comparison for baseline vs mem_mstress vs mem_large
# Usage: RUNS=10 ./run_bench_hz3_rss_compare.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

RUNS="${RUNS:-10}"
ITERS="${ITERS:-20000000}"
WS="${WS:-400}"

BENCH_BIN="${BENCH_BIN:-${ROOT}/hakozuna/out/bench_random_mixed_malloc_args}"
SO_BASELINE="${SO_BASELINE:-${ROOT}/libhakozuna_hz3_scale.so}"
SO_MEM_MSTRESS="${SO_MEM_MSTRESS:-${ROOT}/libhakozuna_hz3_scale_mem_mstress.so}"
SO_MEM_LARGE="${SO_MEM_LARGE:-${ROOT}/libhakozuna_hz3_scale_mem_large.so}"

# Verify files exist
if [ ! -x "$BENCH_BIN" ]; then
  echo "[ERROR] bench bin not found: $BENCH_BIN" >&2
  exit 1
fi
for so in "$SO_BASELINE" "$SO_MEM_MSTRESS" "$SO_MEM_LARGE"; do
  if [ ! -f "$so" ]; then
    echo "[ERROR] SO not found: $so" >&2
    exit 1
  fi
done

GIT_HASH="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo nogit)"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="/tmp/hz3_rss_compare_${GIT_HASH}_${STAMP}"
mkdir -p "$LOG_DIR"

sizes=(
  "small 16 2048"
  "medium 4096 32768"
  "mixed 16 32768"
)

lanes=(
  "baseline:$SO_BASELINE"
  "mem_mstress:$SO_MEM_MSTRESS"
  "mem_large:$SO_MEM_LARGE"
)

echo "[BENCH-HEADER] hz3 RSS comparison" | tee "$LOG_DIR/README.log"
echo "git=$GIT_HASH runs=$RUNS iters=$ITERS ws=$WS" | tee -a "$LOG_DIR/README.log"
echo "bench=$BENCH_BIN" | tee -a "$LOG_DIR/README.log"
echo "baseline=$SO_BASELINE" | tee -a "$LOG_DIR/README.log"
echo "mem_mstress=$SO_MEM_MSTRESS" | tee -a "$LOG_DIR/README.log"
echo "mem_large=$SO_MEM_LARGE" | tee -a "$LOG_DIR/README.log"
echo "" | tee -a "$LOG_DIR/README.log"

for entry in "${sizes[@]}"; do
  set -- $entry
  label="$1"
  min="$2"
  max="$3"

  echo "=== Testing $label (min=$min, max=$max) ===" | tee -a "$LOG_DIR/README.log"

  for lane_spec in "${lanes[@]}"; do
    IFS=':' read -r lane_name so_path <<< "$lane_spec"

    echo "  Lane: $lane_name" | tee -a "$LOG_DIR/README.log"
    log_perf="$LOG_DIR/${lane_name}_${label}_perf.log"
    log_rss="$LOG_DIR/${lane_name}_${label}_rss.log"

    # Performance measurement
    printf "[BENCH-HEADER] lane=%s label=%s min=%s max=%s runs=%s iters=%s ws=%s\n" \
      "$lane_name" "$label" "$min" "$max" "$RUNS" "$ITERS" "$WS" > "$log_perf"

    for _ in $(seq 1 "$RUNS"); do
      env -u LD_PRELOAD LD_PRELOAD="$so_path" \
        "$BENCH_BIN" "$ITERS" "$WS" "$min" "$max"
    done | tee -a "$log_perf"

    # RSS measurement (single run with /usr/bin/time -v)
    echo "    Measuring RSS..." | tee -a "$LOG_DIR/README.log"
    env -u LD_PRELOAD LD_PRELOAD="$so_path" \
      /usr/bin/time -v "$BENCH_BIN" "$ITERS" "$WS" "$min" "$max" 2>&1 >/dev/null | \
      grep -E 'Elapsed|Maximum resident|Minor|Major' > "$log_rss" || true
  done

  echo "" | tee -a "$LOG_DIR/README.log"
done

echo "logs: $LOG_DIR" | tee -a "$LOG_DIR/README.log"

# Generate summary
echo "" | tee -a "$LOG_DIR/README.log"
echo "=== Summary ===" | tee -a "$LOG_DIR/README.log"
for entry in "${sizes[@]}"; do
  set -- $entry
  label="$1"

  echo "" | tee -a "$LOG_DIR/README.log"
  echo "$label:" | tee -a "$LOG_DIR/README.log"
  for lane_spec in "${lanes[@]}"; do
    IFS=':' read -r lane_name so_path <<< "$lane_spec"
    log_perf="$LOG_DIR/${lane_name}_${label}_perf.log"
    log_rss="$LOG_DIR/${lane_name}_${label}_rss.log"

    if [ -f "$log_perf" ] && [ -f "$log_rss" ]; then
      median=$(grep -o '[0-9.]\+ M ops/sec' "$log_perf" | grep -o '[0-9.]\+' | sort -n | awk '{a[NR]=$1} END {print (NR%2==1)?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2}')
      rss=$(grep 'Maximum resident' "$log_rss" | grep -o '[0-9]\+' || echo "N/A")
      elapsed=$(grep 'Elapsed' "$log_rss" | grep -o '[0-9:\.]\+' || echo "N/A")

      printf "  %-15s: %8s M ops/sec, RSS=%s KB, Elapsed=%s\n" \
        "$lane_name" "$median" "$rss" "$elapsed" | tee -a "$LOG_DIR/README.log"
    fi
  done
done

echo "" | tee -a "$LOG_DIR/README.log"
echo "Full logs in: $LOG_DIR" | tee -a "$LOG_DIR/README.log"
