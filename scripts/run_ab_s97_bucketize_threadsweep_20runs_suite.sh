#!/usr/bin/env bash
set -euo pipefail

# A/B suite for S97 bucketize (flush-side merge) with a thread-count sweep.
#
# Goal:
# - Reduce time-drift bias via interleaved runs.
# - Make "thread-count sensitivity" visible (T=8/16/32 by default).
#
# Usage:
#   RUNS=20 ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
#
# Override examples:
#   THREADS_LIST="8 32" RUNS=30 REMOTE_PCTS="50 90" \
#     ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
#
# Notes:
# - This script intentionally uses compile-time flags (lane split). No env knobs in allocator.
# - To reduce variance further, optionally pin with PIN=1 (requires taskset).

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "${ROOT_DIR}"

RUNS="${RUNS:-20}"
WARMUP="${WARMUP:-0}"
PIN="${PIN:-0}"

THREADS_LIST="${THREADS_LIST:-8 16 32}"
REMOTE_PCTS="${REMOTE_PCTS:-50 90}"

WS="${WS:-400}"
MIN_SZ="${MIN_SZ:-16}"
MAX_SZ="${MAX_SZ:-2048}"
RING_SLOTS="${RING_SLOTS:-65536}"

BASE_SO="/tmp/hz3_s97_base.so"
TREAT_SO="/tmp/hz3_s97_treat.so"

# Keep stats ON for both sides so we can correlate shape under noise.
BASE_DEFS_EXTRA="${BASE_DEFS_EXTRA:--DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=0 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1}"
TREAT_DEFS_EXTRA="${TREAT_DEFS_EXTRA:--DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=2 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1}"

build_lib() {
  local out_so="$1"; shift
  local defs_extra="$1"; shift
  # NOTE: Do not run `make -j clean <build-target>` (clean may race with compile).
  make -C hakozuna/hz3 clean >/tmp/build_hz3_s97_ab.log 2>&1
  make -C hakozuna/hz3 all_ldpreload_scale -j HZ3_LDPRELOAD_DEFS_EXTRA="${defs_extra}" >>/tmp/build_hz3_s97_ab.log 2>&1
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

extract_s97_saved_pct() {
  local log_path="$1"; shift
  python3 - "$log_path" <<'PY'
import re, sys
s = open(sys.argv[1], "r", errors="ignore").read()
m = re.search(r"\[HZ3_S97_REMOTE\].*?entries=(\d+).*?saved_calls=(\d+)", s)
if not m:
    print("nan")
    raise SystemExit(0)
entries = int(m.group(1))
saved = int(m.group(2))
if entries <= 0:
    print("nan")
else:
    print(saved * 100.0 / entries)
PY
}

median() {
  python3 - "$@" <<'PY'
import statistics, sys
vals = [float(x) for x in sys.argv[1:] if x != "nan"]
if not vals:
    print("nan")
else:
    print(statistics.median(vals))
PY
}

median_pct_delta_paired() {
  python3 - "$@" <<'PY'
import statistics, sys
vals = [float(x) for x in sys.argv[1:]]
print(statistics.median(vals))
PY
}

run_one() {
  local so_path="$1"; shift
  local threads="$1"; shift
  local cmd=( "$@" )

  if [[ "${PIN}" != "0" ]] && command -v taskset >/dev/null 2>&1; then
    local max_cpu
    max_cpu="$((threads - 1))"
    env -u LD_PRELOAD LD_PRELOAD="${so_path}" taskset -c "0-${max_cpu}" "${cmd[@]}"
  else
    env -u LD_PRELOAD LD_PRELOAD="${so_path}" "${cmd[@]}"
  fi
}

run_case_ab_interleaved() {
  local case_name="$1"; shift
  local threads="$1"; shift
  local base_so_path="$1"; shift
  local treat_so_path="$1"; shift
  local cmd=( "$@" )
  local rates_base=()
  local rates_treat=()
  local deltas=()
  local treat_saved_pct=()

  if [[ "${WARMUP}" != "0" ]]; then
    ( run_one "${base_so_path}" "${threads}" "${cmd[@]}" ) >/tmp/ab_"${case_name}"_warmup_base.log 2>&1 || true
    ( run_one "${treat_so_path}" "${threads}" "${cmd[@]}" ) >/tmp/ab_"${case_name}"_warmup_treat.log 2>&1 || true
  fi

  for i in $(seq 1 "${RUNS}"); do
    local log_b="/tmp/ab_${case_name}_base_${i}.log"
    local log_t="/tmp/ab_${case_name}_treat_${i}.log"

    # Interleave base/treat and alternate order.
    if (( (i % 2) == 1 )); then
      ( run_one "${base_so_path}" "${threads}" "${cmd[@]}" ) >"${log_b}" 2>&1 || true
      ( run_one "${treat_so_path}" "${threads}" "${cmd[@]}" ) >"${log_t}" 2>&1 || true
    else
      ( run_one "${treat_so_path}" "${threads}" "${cmd[@]}" ) >"${log_t}" 2>&1 || true
      ( run_one "${base_so_path}" "${threads}" "${cmd[@]}" ) >"${log_b}" 2>&1 || true
    fi

    local rb rt
    rb="$(extract_rate "${log_b}")"
    rt="$(extract_rate "${log_t}")"
    rates_base+=( "${rb}" )
    rates_treat+=( "${rt}" )
    deltas+=( "$(python3 -c "b=float('${rb}'); t=float('${rt}'); print((t/b-1.0)*100.0)")" )

    treat_saved_pct+=( "$(extract_s97_saved_pct "${log_t}")" )
  done

  local base_med treat_med delta_med saved_med
  base_med="$(median "${rates_base[@]}")"
  treat_med="$(median "${rates_treat[@]}")"
  delta_med="$(median_pct_delta_paired "${deltas[@]}")"
  saved_med="$(median "${treat_saved_pct[@]}")"
  echo "${base_med} ${treat_med} ${delta_med} ${saved_med}"
}

echo "[S97_AB] RUNS=${RUNS} THREADS_LIST='${THREADS_LIST}' REMOTE_PCTS='${REMOTE_PCTS}' WS=${WS} MIN=${MIN_SZ} MAX=${MAX_SZ} PIN=${PIN}"
echo "[S97_AB] BASE_DEFS_EXTRA='${BASE_DEFS_EXTRA}'"
echo "[S97_AB] TREAT_DEFS_EXTRA='${TREAT_DEFS_EXTRA}'"

build_lib "${BASE_SO}" "${BASE_DEFS_EXTRA}"
build_lib "${TREAT_SO}" "${TREAT_DEFS_EXTRA}"

printf "\n%-18s %8s %12s %12s %12s %12s\n" "case" "threads" "base(M/s)" "treat(M/s)" "delta_med" "saved_med"
for threads in ${THREADS_LIST}; do
  for r in ${REMOTE_PCTS}; do
    case_name="mt_remote_r${r}_t${threads}"
    cmd=( ./hakozuna/out/bench_random_mixed_mt_remote_malloc "${threads}" 2000000 "${WS}" "${MIN_SZ}" "${MAX_SZ}" "${r}" "${RING_SLOTS}" )
    read -r base_med treat_med delta_med saved_med < <(run_case_ab_interleaved "${case_name}" "${threads}" "${BASE_SO}" "${TREAT_SO}" "${cmd[@]}")

    base_m="$(python3 -c "print(float('${base_med}')/1e6)")"
    treat_m="$(python3 -c "print(float('${treat_med}')/1e6)")"
    if [[ "${saved_med}" == "nan" ]]; then
      printf "%-18s %8d %12.2f %12.2f %+11.2f%% %12s\n" "r${r}" "${threads}" "${base_m}" "${treat_m}" "${delta_med}" "n/a"
    else
      printf "%-18s %8d %12.2f %12.2f %+11.2f%% %11.2f%%\n" "r${r}" "${threads}" "${base_m}" "${treat_m}" "${delta_med}" "${saved_med}"
    fi
  done
done
