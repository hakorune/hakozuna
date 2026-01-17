#!/usr/bin/env bash
set -euo pipefail

# A/B suite for S99: compare baseline scale defaults vs PREPEND_LIST micro-opt.
#
# Usage:
#   RUNS=10 ./hakozuna/hz3/scripts/run_ab_s99_alloc_slow_prepend_list_10runs_suite.sh
#   RUNS=20 WARMUP=1 ./hakozuna/hz3/scripts/run_ab_s99_alloc_slow_prepend_list_10runs_suite.sh

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}"

RUNS="${RUNS:-10}"
THREADS="${THREADS:-32}"
WS="${WS:-400}"
RING_SLOTS="${RING_SLOTS:-65536}"
WARMUP="${WARMUP:-0}"

BASE_SO="/tmp/hz3_s99_base.so"
TREAT_SO="/tmp/hz3_s99_treat.so"

build_lib() {
  local out_so="$1"; shift
  local defs_extra="$1"; shift
  make -C hakozuna/hz3 clean all_ldpreload_scale -j HZ3_LDPRELOAD_DEFS_EXTRA="${defs_extra}" >/tmp/build_hz3_s99_ab.log 2>&1
  cp ./libhakozuna_hz3_scale.so "${out_so}"
}

extract_rate() {
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

run_case_ab_interleaved() {
  local case_name="$1"; shift
  local base_so_path="$1"; shift
  local treat_so_path="$1"; shift
  local cmd=( "$@" )
  local rates_base=()
  local rates_treat=()

  if [[ "${WARMUP}" != "0" ]]; then
    local log_wb="/tmp/ab_${case_name}_warmup_base.log"
    local log_wt="/tmp/ab_${case_name}_warmup_treat.log"
    ( env -u LD_PRELOAD LD_PRELOAD="${base_so_path}" "${cmd[@]}" ) >"${log_wb}" 2>&1 || true
    ( env -u LD_PRELOAD LD_PRELOAD="${treat_so_path}" "${cmd[@]}" ) >"${log_wt}" 2>&1 || true
  fi

  for i in $(seq 1 "${RUNS}"); do
    local log_b="/tmp/ab_${case_name}_$(basename "${base_so_path}" .so)_${i}.log"
    local log_t="/tmp/ab_${case_name}_$(basename "${treat_so_path}" .so)_${i}.log"

    if (( (i % 2) == 1 )); then
      ( env -u LD_PRELOAD LD_PRELOAD="${base_so_path}" "${cmd[@]}" ) >"${log_b}" 2>&1 || true
      ( env -u LD_PRELOAD LD_PRELOAD="${treat_so_path}" "${cmd[@]}" ) >"${log_t}" 2>&1 || true
    else
      ( env -u LD_PRELOAD LD_PRELOAD="${treat_so_path}" "${cmd[@]}" ) >"${log_t}" 2>&1 || true
      ( env -u LD_PRELOAD LD_PRELOAD="${base_so_path}" "${cmd[@]}" ) >"${log_b}" 2>&1 || true
    fi

    rates_base+=( "$(extract_rate "${log_b}")" )
    rates_treat+=( "$(extract_rate "${log_t}")" )
  done

  local base_med
  local treat_med
  base_med="$(median "${rates_base[@]}")"
  treat_med="$(median "${rates_treat[@]}")"
  echo "${base_med} ${treat_med}"
}

echo "[S99_AB] RUNS=${RUNS} THREADS=${THREADS} WS=${WS} RING_SLOTS=${RING_SLOTS}"

build_lib "${BASE_SO}" ""
build_lib "${TREAT_SO}" "-DHZ3_S99_ALLOC_SLOW_PREPEND_LIST=1"

declare -a CASES=(
  "mt_remote_r50_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 3000000 ${WS} 16 2048 50 ${RING_SLOTS}"
  "mt_remote_r90_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 2000000 ${WS} 16 2048 90 ${RING_SLOTS}"
  "mt_remote_r0_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 3000000 ${WS} 16 2048 0 ${RING_SLOTS}"
  "dist_app:./hakozuna/out/bench_random_mixed_malloc_dist 20000000 ${WS} 16 32768 0x12345678 app"
  "dist_uniform:./hakozuna/out/bench_random_mixed_malloc_dist 20000000 ${WS} 16 32768 0x12345678 uniform"
  "malloc_flood_32t:./hakozuna/out/bench_malloc_flood_mt ${THREADS} 2 256 1 1"
)

printf "\n%-22s %12s %12s %9s\n" "case" "base(M/s)" "treat(M/s)" "delta"
for spec in "${CASES[@]}"; do
  case_name="${spec%%:*}"
  rest="${spec#*:}"
  # shellcheck disable=SC2206
  cmd=( ${rest} )

  read -r base_med treat_med < <(run_case_ab_interleaved "${case_name}" "${BASE_SO}" "${TREAT_SO}" "${cmd[@]}")

  base_m="$(python3 -c "print(float('${base_med}')/1e6)")"
  treat_m="$(python3 -c "print(float('${treat_med}')/1e6)")"
  delta="$(python3 -c "s=float('${base_med}'); t=float('${treat_med}'); print((t/s-1.0)*100.0)")"

  printf "%-22s %12.2f %12.2f %+8.2f%%\n" "${case_name}" "${base_m}" "${treat_m}" "${delta}"
done

