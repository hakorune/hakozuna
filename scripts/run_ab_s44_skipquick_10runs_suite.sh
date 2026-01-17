#!/usr/bin/env bash
set -euo pipefail

# A/B suite for S44-4: compare current scale defaults ("safe") vs
# SKIP_QUICK + UNCOND candidate on multiple workloads with N runs.
#
# Output logs go to /tmp and a summary line is printed per case.
#
# Usage:
#   RUNS=10 ./hakozuna/hz3/scripts/run_ab_s44_skipquick_10runs_suite.sh
#   RUNS=10 THREADS=32 WS=400 ./hakozuna/hz3/scripts/run_ab_s44_skipquick_10runs_suite.sh

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}"

RUNS="${RUNS:-10}"
THREADS="${THREADS:-32}"
WS="${WS:-400}"
RING_SLOTS="${RING_SLOTS:-65536}"

SAFE_SO="/tmp/hz3_s44_safe.so"
SKIP_SO="/tmp/hz3_s44_skipquick.so"

build_lib() {
  local out_so="$1"; shift
  local defs_extra="$1"; shift
  make -C hakozuna/hz3 clean all_ldpreload_scale -j HZ3_LDPRELOAD_DEFS_EXTRA="${defs_extra}" >/tmp/build_hz3_ab.log 2>&1
  cp ./libhakozuna_hz3_scale.so "${out_so}"
}

extract_rate() {
  # Supports both:
  #   ops/s=123.45
  #   ops_s=123
  local log_path="$1"; shift
  python3 - "$log_path" <<'PY'
import re, sys
s = open(sys.argv[1], "r", errors="ignore").read()
m = re.search(r"ops/s=([0-9.]+)", s)
if m:
    print(m.group(1))
    raise SystemExit(0)
m = re.search(r"ops_s=([0-9.]+)", s)
if m:
    print(m.group(1))
    raise SystemExit(0)
raise SystemExit(2)
PY
}

median() {
  python3 - "$@" <<'PY'
import statistics, sys
vals = [float(x) for x in sys.argv[1:]]
print(statistics.median(vals))
PY
}

run_case() {
  local case_name="$1"; shift
  local so_path="$1"; shift
  local cmd=( "$@" )
  local rates=()

  for i in $(seq 1 "${RUNS}"); do
    local log="/tmp/ab_${case_name}_$(basename "${so_path}" .so)_${i}.log"
    ( env -u LD_PRELOAD LD_PRELOAD="${so_path}" "${cmd[@]}" ) >"${log}" 2>&1 || true
    rates+=( "$(extract_rate "${log}")" )
  done

  median "${rates[@]}"
}

echo "[AB] RUNS=${RUNS} THREADS=${THREADS} WS=${WS} RING_SLOTS=${RING_SLOTS}"

# Baseline "safe" = current scale defaults: EPF_HINT=1 + UNCOND=1, SKIP_QUICK=0
build_lib "${SAFE_SO}" "-DHZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=0 -DHZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=1 -DHZ3_S44_4_WALK_PREFETCH_UNCOND=1"

# Candidate = SKIP_QUICK + UNCOND (force EPF_HINT=0 for clarity)
build_lib "${SKIP_SO}" "-DHZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=1 -DHZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=0 -DHZ3_S44_4_WALK_PREFETCH_UNCOND=1"

declare -a CASES=(
  # Remote-heavy (small range): should exercise owner stash hard.
  "mt_remote_r50_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 3000000 ${WS} 16 2048 50 ${RING_SLOTS}"
  "mt_remote_r90_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 2000000 ${WS} 16 2048 90 ${RING_SLOTS}"
  "mt_remote_r0_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 3000000 ${WS} 16 2048 0 ${RING_SLOTS}"

  # Dist (ST): sanity for non-remote general workload.
  "dist_app:./hakozuna/out/bench_random_mixed_malloc_dist 20000000 ${WS} 16 32768 0x12345678 app"
  "dist_uniform:./hakozuna/out/bench_random_mixed_malloc_dist 20000000 ${WS} 16 32768 0x12345678 uniform"

  # Flood (MT): throughput sanity (may not hit remote path).
  "malloc_flood_32t:./hakozuna/out/bench_malloc_flood_mt ${THREADS} 2 256 1 1"
)

printf "\n%-22s %12s %12s %9s\n" "case" "safe(M/s)" "skip(M/s)" "delta"
for spec in "${CASES[@]}"; do
  case_name="${spec%%:*}"
  rest="${spec#*:}"
  # shellcheck disable=SC2206
  cmd=( ${rest} )

  safe_med="$(run_case "${case_name}" "${SAFE_SO}" "${cmd[@]}")"
  skip_med="$(run_case "${case_name}" "${SKIP_SO}" "${cmd[@]}")"

  safe_m="$(python3 -c "print(float('${safe_med}')/1e6)")"
  skip_m="$(python3 -c "print(float('${skip_med}')/1e6)")"
  delta="$(python3 -c "s=float('${safe_med}'); t=float('${skip_med}'); print((t/s-1.0)*100.0)")"

  printf "%-22s %12.2f %12.2f %+8.2f%%\n" "${case_name}" "${safe_m}" "${skip_m}" "${delta}"
done

