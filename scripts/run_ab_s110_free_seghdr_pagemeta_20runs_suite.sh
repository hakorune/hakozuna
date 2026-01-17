#!/usr/bin/env bash
set -euo pipefail

# A/B suite for S110-1: SegHdr PerPage FreeMeta Fast Path
#
# Baseline:  current scale defaults (PTAG path only)
# Treatment: HZ3_S110_SEGHDR_PAGEMETA_ENABLE=1 (fast path + PTAG fallback)
#
# Usage:
#   RUNS=20 ./hakozuna/hz3/scripts/run_ab_s110_free_seghdr_pagemeta_20runs_suite.sh
#   RUNS=10 THREADS=16 ./hakozuna/hz3/scripts/run_ab_s110_free_seghdr_pagemeta_20runs_suite.sh

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}"

RUNS="${RUNS:-20}"
THREADS="${THREADS:-32}"
WS="${WS:-400}"
RING_SLOTS="${RING_SLOTS:-65536}"
WARMUP="${WARMUP:-1}"

BASE_SO="/tmp/hz3_s110_base.so"
TREAT_SO="/tmp/hz3_s110_treat.so"

# Baseline: current defaults (no S110)
BASE_DEFS_EXTRA="${BASE_DEFS_EXTRA:-}"

# Treatment: S110 enabled with stats
TREAT_DEFS_EXTRA="${TREAT_DEFS_EXTRA:--DHZ3_S110_SEGHDR_PAGEMETA_ENABLE=1 -DHZ3_S110_SEGHDR_PAGEMETA_STATS=1}"

build_lib() {
  local out_so="$1"; shift
  local defs_extra="$1"; shift
  make -C hakozuna/hz3 clean all_ldpreload_scale -j HZ3_LDPRELOAD_DEFS_EXTRA="${defs_extra}" >/tmp/build_hz3_s110_ab.log 2>&1
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

    # Interleave base/treat to reduce time-drift bias; alternate order per iteration.
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

echo "[S110_AB] RUNS=${RUNS} THREADS=${THREADS} WS=${WS} RING_SLOTS=${RING_SLOTS}"
echo "[S110_AB] BASE_DEFS_EXTRA='${BASE_DEFS_EXTRA}'"
echo "[S110_AB] TREAT_DEFS_EXTRA='${TREAT_DEFS_EXTRA}'"

# Baseline = current scale defaults (PTAG path only)
echo "[S110_AB] Building baseline..."
build_lib "${BASE_SO}" "${BASE_DEFS_EXTRA}"

# Treatment = S110 enabled
echo "[S110_AB] Building treatment..."
build_lib "${TREAT_SO}" "${TREAT_DEFS_EXTRA}"

declare -a CASES=(
  "mt_remote_r50_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 3000000 ${WS} 16 2048 50 ${RING_SLOTS}"
  "mt_remote_r90_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 2000000 ${WS} 16 2048 90 ${RING_SLOTS}"
  "mt_remote_r0_small:./hakozuna/out/bench_random_mixed_mt_remote_malloc ${THREADS} 3000000 ${WS} 16 2048 0 ${RING_SLOTS}"
  "dist_app:./hakozuna/out/bench_random_mixed_malloc_dist 20000000 ${WS} 16 32768 0x12345678 app"
  "dist_uniform:./hakozuna/out/bench_random_mixed_malloc_dist 20000000 ${WS} 16 32768 0x12345678 uniform"
  "mstress_32t:./hakozuna/out/bench_mstress ${THREADS} 200000 ${WS}"
)

printf "\n%-22s %12s %12s %9s\n" "case" "base(M/s)" "treat(M/s)" "delta"
printf "%-22s %12s %12s %9s\n" "----" "---------" "----------" "-----"

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

echo ""
echo "[S110_AB] Done. Check /tmp/ab_*_hz3_s110_*.log for individual run logs."
echo "[S110_AB] Treatment logs include [HZ3_S110_FREE] stats at the end."
