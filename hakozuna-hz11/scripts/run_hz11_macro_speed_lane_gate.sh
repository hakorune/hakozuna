#!/usr/bin/env bash
# HZ11MacroSpeedLaneGate-L1:
# Macro workload gate for libhz11_span_transfer.so. No allocator hot-path
# changes belong in this box.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${ROOT}/.." && pwd)"
ROOT_DIR="${REPO_ROOT}"
if [[ -f "${REPO_ROOT}/bench/lib/bench_common.sh" ]]; then
  source "${REPO_ROOT}/bench/lib/bench_common.sh"
fi

STAMP="${STAMP:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${REPO_ROOT}/bench_results/hz11_macro_speed_lane_${STAMP}}"
RUNS="${RUNS:-5}"
ALLOCATORS="${ALLOCATORS:-glibc,tcmalloc,mimalloc,hz11-span-soa,hz11-span-transfer}"
HZ11_MACRO_CANDIDATE="${HZ11_MACRO_CANDIDATE:-hz11-span-transfer}"
HZ11_REQUIRE_SPAN_SOA_CHECK="${HZ11_REQUIRE_SPAN_SOA_CHECK:-1}"
HZ11_COUNTER_EXPECT_INSERT_MIN="${HZ11_COUNTER_EXPECT_INSERT_MIN:-1024}"
BUILD="${BUILD:-1}"

PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
LARSON_SECONDS="${LARSON_SECONDS:-2}"
LARSON_MIN="${LARSON_MIN:-8}"
LARSON_MAX="${LARSON_MAX:-128}"
LARSON_CHUNKS="${LARSON_CHUNKS:-128}"
LARSON_ROUNDS="${LARSON_ROUNDS:-1}"
LARSON_SEED="${LARSON_SEED:-12345}"
LARSON_THREADS="${LARSON_THREADS:-4}"
XMALLOC_WORKERS="${XMALLOC_WORKERS:-4}"
XMALLOC_SECONDS="${XMALLOC_SECONDS:-2}"
XMALLOC_SIZE="${XMALLOC_SIZE:--1}"
SH6_CALL_COUNT="${SH6_CALL_COUNT:-100000}"
SH6_MIN_BLOCK="${SH6_MIN_BLOCK:-1}"
SH6_MAX_BLOCK="${SH6_MAX_BLOCK:-1000}"
SH6_THREADS="${SH6_THREADS:-4}"
CACHE_SCRATCH_THREADS="${CACHE_SCRATCH_THREADS:-4}"
CACHE_SCRATCH_ITERATIONS="${CACHE_SCRATCH_ITERATIONS:-5000}"
CACHE_SCRATCH_OBJECT_SIZE="${CACHE_SCRATCH_OBJECT_SIZE:-256}"
CACHE_SCRATCH_REPETITIONS="${CACHE_SCRATCH_REPETITIONS:-5000}"
CACHE_SCRATCH_CONCURRENCY="${CACHE_SCRATCH_CONCURRENCY:-4}"
MSTRESS_THREADS="${MSTRESS_THREADS:-8}"
MSTRESS_SCALE="${MSTRESS_SCALE:-80}"
MSTRESS_ITER="${MSTRESS_ITER:-3}"

RUN_PYTHON_ALLOC="${RUN_PYTHON_ALLOC:-1}"
RUN_LARSON="${RUN_LARSON:-1}"
RUN_XMALLOC="${RUN_XMALLOC:-1}"
RUN_SH6BENCH="${RUN_SH6BENCH:-1}"
RUN_CACHE_SCRATCH="${RUN_CACHE_SCRATCH:-1}"
RUN_MSTRESS="${RUN_MSTRESS:-1}"

usage() {
  cat <<'EOF'
Usage:
  hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh [options]

Options:
  --allocators LIST   comma-separated allocators
  --candidate NAME    HZ11 allocator to gate (default hz11-span-transfer)
  --skip-span-soa-check
                      do not require candidate/span-soa wall check
  --runs N            fresh process samples per workload/allocator (default 5)
  --outdir DIR        output directory
  --skip-build        do not build HZ11 lanes
  --help              show this message

Rows:
  python_alloc
  larson
  xmalloc_test
  sh6bench if available
  cache_scratch and mstress if available
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --allocators)
      ALLOCATORS="$2"; shift 2 ;;
    --candidate)
      HZ11_MACRO_CANDIDATE="$2"; shift 2 ;;
    --skip-span-soa-check)
      HZ11_REQUIRE_SPAN_SOA_CHECK=0; shift ;;
    --runs)
      RUNS="$2"; shift 2 ;;
    --outdir)
      OUTDIR="$2"; shift 2 ;;
    --skip-build)
      BUILD=0; shift ;;
    --help|-h)
      usage; exit 0 ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1 ;;
  esac
done

mkdir -p "${OUTDIR}"

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT}" preload-span-soa preload-span-transfer \
    preload-span-transfer-thread-exit preload-span-transfer-thread-exit-cap \
    preload-span-transfer-thread-exit-cap-batch32 \
    preload-span-transfer-thread-exit-cap-batch32-fineclass >/dev/null
fi

find_first_existing() {
  local path
  for path in "$@"; do
    if [[ -n "${path}" && -f "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

find_first_executable() {
  local path
  for path in "$@"; do
    if [[ -n "${path}" && -x "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

find_tcmalloc_lib() {
  if [[ -n "${TCMALLOC_SO:-}" && -f "${TCMALLOC_SO}" ]]; then
    printf '%s\n' "${TCMALLOC_SO}"
    return 0
  fi
  if [[ -n "${TCMALLOC_LIB:-}" && -f "${TCMALLOC_LIB}" ]]; then
    printf '%s\n' "${TCMALLOC_LIB}"
    return 0
  fi
  find_first_existing \
    /lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
    /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
    /usr/local/lib/libtcmalloc_minimal.so.4 \
    /usr/lib/libtcmalloc_minimal.so.4 && return 0
  if declare -F bench_find_tcmalloc_library >/dev/null; then
    bench_find_tcmalloc_library
    return $?
  fi
  return 1
}

find_mimalloc_lib() {
  if [[ -n "${MIMALLOC_SO:-}" && -f "${MIMALLOC_SO}" ]]; then
    printf '%s\n' "${MIMALLOC_SO}"
    return 0
  fi
  if [[ -n "${MIMALLOC_LIB:-}" && -f "${MIMALLOC_LIB}" ]]; then
    printf '%s\n' "${MIMALLOC_LIB}"
    return 0
  fi
  find_first_existing \
    /mnt/workdisk/public_share/hakmem/mimalloc-install/lib/libmimalloc.so.2.2 \
    /mnt/workdisk/public_share/hakmem/mimalloc_src/out/release/libmimalloc.so.2.2 \
    /mnt/workdisk/public_share/hakmem/mimalloc/build/libmimalloc.so.2.2 \
    /mnt/workdisk/public_share/hakmem/allocators/mimalloc/libmimalloc.so \
    "${REPO_ROOT}/hakmem/mimalloc-install/lib/libmimalloc.so.2.2"
}

find_larson_bin() {
  find_first_executable \
    "${LARSON_BIN:-}" \
    /mnt/workdisk/public_share/hakmem/larson_system \
    /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/larson
}

find_mimalloc_bench_bin() {
  local name="$1"
  find_first_executable \
    "${MIMALLOC_BENCH_DIR:-}/${name}" \
    "/mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/${name}" \
    "/mnt/workdisk/public_share/hakmem/mimalloc-bench/build/bench/${name}"
}

larson_bin="$(find_larson_bin || true)"
xmalloc_bin="$(find_mimalloc_bench_bin xmalloc-test || true)"
cache_scratch_bin="$(find_mimalloc_bench_bin cache-scratch || true)"
mstress_bin="$(find_mimalloc_bench_bin mstress || true)"
sh6bench_bin="$(find_mimalloc_bench_bin sh6bench || true)"
tcmalloc_lib="$(find_tcmalloc_lib || true)"
mimalloc_lib="$(find_mimalloc_lib || true)"

declare -a alloc_names=()
declare -a alloc_libs=()
declare -a alloc_skip_reasons=()

add_allocator() {
  alloc_names+=("$1")
  alloc_libs+=("$2")
  alloc_skip_reasons+=("$3")
}

IFS=',' read -r -a requested_allocators <<<"${ALLOCATORS}"
for alloc in "${requested_allocators[@]}"; do
  case "${alloc}" in
    glibc)
      add_allocator glibc "" "" ;;
    tcmalloc)
      if [[ -n "${tcmalloc_lib}" ]]; then
        add_allocator tcmalloc "${tcmalloc_lib}" ""
      else
        add_allocator tcmalloc "" "tcmalloc library not found"
      fi ;;
    mimalloc)
      if [[ -n "${mimalloc_lib}" ]]; then
        add_allocator mimalloc "${mimalloc_lib}" ""
      else
        add_allocator mimalloc "" "mimalloc library not found"
      fi ;;
    hz11-span-soa)
      add_allocator hz11-span-soa "${ROOT}/libhz11_span_soa.so" "" ;;
    hz11-span-transfer)
      add_allocator hz11-span-transfer "${ROOT}/libhz11_span_transfer.so" "" ;;
    hz11-thread-exit)
      add_allocator hz11-thread-exit "${ROOT}/libhz11_span_transfer_thread_exit.so" "" ;;
    hz11-thread-exit-cap)
      add_allocator hz11-thread-exit-cap "${ROOT}/libhz11_span_transfer_thread_exit_cap.so" "" ;;
    hz11-thread-exit-cap-batch32)
      add_allocator hz11-thread-exit-cap-batch32 "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32.so" "" ;;
    hz11-thread-exit-cap-batch32-fineclass)
      add_allocator hz11-thread-exit-cap-batch32-fineclass "${ROOT}/libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so" "" ;;
    /*)
      if [[ -f "${alloc}" ]]; then
        add_allocator "$(basename "${alloc}")" "${alloc}" ""
      else
        add_allocator "${alloc}" "" "explicit allocator library not found"
      fi ;;
    *)
      add_allocator "${alloc}" "" "unknown allocator" ;;
  esac
done

read_proc_status_kb() {
  local pid="$1" key="$2"
  awk -v key="${key}:" '$1 == key { print $2; exit }' "/proc/${pid}/status" 2>/dev/null || true
}

csv="${OUTDIR}/samples.csv"
summary_md="${OUTDIR}/summary.md"
readme_log="${OUTDIR}/README.log"
printf 'workload,allocator,run,status,exit_code,wall_sec,max_rss_kb,current_rss_kb,xfer_hit,xfer_insert,xfer_spill,central_hit,central_insert,timeout_sec,log,stderr_log,stdout_log,note\n' >"${csv}"

{
  echo "[HZ11_MACRO_SPEED_LANE] ts=${STAMP}"
  echo "[HZ11_MACRO_SPEED_LANE] root=${REPO_ROOT}"
  echo "[HZ11_MACRO_SPEED_LANE] hz11_root=${ROOT}"
  echo "[HZ11_MACRO_SPEED_LANE] runs=${RUNS}"
  echo "[HZ11_MACRO_SPEED_LANE] allocators=${ALLOCATORS}"
  echo "[HZ11_MACRO_SPEED_LANE] candidate=${HZ11_MACRO_CANDIDATE}"
  echo "[HZ11_MACRO_SPEED_LANE] require_span_soa_check=${HZ11_REQUIRE_SPAN_SOA_CHECK}"
  echo "[HZ11_MACRO_SPEED_LANE] counter_expect_insert_min=${HZ11_COUNTER_EXPECT_INSERT_MIN}"
  echo "[HZ11_MACRO_SPEED_LANE] tcmalloc=${tcmalloc_lib:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] mimalloc=${mimalloc_lib:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] larson=${larson_bin:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] xmalloc_test=${xmalloc_bin:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] sh6bench=${sh6bench_bin:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] cache_scratch=${cache_scratch_bin:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] mstress=${mstress_bin:-SKIP}"
  echo "[HZ11_MACRO_SPEED_LANE] git_sha=$(git -C "${REPO_ROOT}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
} >"${readme_log}"

csv_escape() {
  local s="${1//$'\n'/ }"
  s="${s//\"/\"\"}"
  printf '"%s"' "${s}"
}

append_sample() {
  local workload="$1" allocator="$2" run="$3" status="$4" rc="$5" wall="$6"
  local max_rss="$7" current_rss="$8" xfer_hit="$9" xfer_insert="${10}"
  local xfer_spill="${11}" central_hit="${12}" central_insert="${13}"
  local timeout_sec="${14}" log_file="${15}" err_file="${16}" out_file="${17}"
  local note="${18}"
  {
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,' \
      "${workload}" "${allocator}" "${run}" "${status}" "${rc}" "${wall}" \
      "${max_rss}" "${current_rss}" "${xfer_hit}" "${xfer_insert}" \
      "${xfer_spill}" "${central_hit}" "${central_insert}" "${timeout_sec}"
    csv_escape "${log_file}"; printf ','
    csv_escape "${err_file}"; printf ','
    csv_escape "${out_file}"; printf ','
    csv_escape "${note}"; printf '\n'
  } >>"${csv}"
}

run_sampled() {
  local workload="$1" allocator="$2" run="$3" timeout_sec="$4"
  shift 4
  local log_file="${OUTDIR}/${workload}_${allocator}_${run}.log"
  local out_file="${OUTDIR}/${workload}_${allocator}_${run}.out"
  local err_file="${OUTDIR}/${workload}_${allocator}_${run}.err"
  local start_ns now_ns wall rc status max_rss current_rss last_rss hwm sample
  echo "[RUN] workload=${workload} allocator=${allocator} run=${run}" | tee -a "${readme_log}"
  {
    echo "[CASE] workload=${workload}"
    echo "[CASE] allocator=${allocator}"
    echo "[CASE] run=${run}"
    echo "[CASE] timeout_sec=${timeout_sec}"
    echo "[CASE] command=$*"
    echo
  } >"${log_file}"
  start_ns="$(date +%s%N)"
  "$@" >"${out_file}" 2>"${err_file}" &
  local pid=$!
  max_rss="NA"
  current_rss="NA"
  last_rss=""
  status="OK"
  while kill -0 "${pid}" 2>/dev/null; do
    sample="$(read_proc_status_kb "${pid}" VmRSS)"
    hwm="$(read_proc_status_kb "${pid}" VmHWM)"
    if [[ -n "${sample}" ]]; then
      last_rss="${sample}"
      current_rss="${sample}"
    fi
    if [[ -n "${hwm}" && ( "${max_rss}" == "NA" || "${hwm}" -gt "${max_rss}" ) ]]; then
      max_rss="${hwm}"
    fi
    if [[ "${timeout_sec}" -gt 0 ]]; then
      now_ns="$(date +%s%N)"
      if awk -v s="${start_ns}" -v n="${now_ns}" -v t="${timeout_sec}" 'BEGIN { exit !(((n-s)/1000000000) > t) }'; then
        status="TIMEOUT"
        kill -KILL "${pid}" 2>/dev/null || true
        break
      fi
    fi
    sleep 0.02
  done
  set +e
  wait "${pid}" 2>/dev/null
  rc=$?
  set -e
  now_ns="$(date +%s%N)"
  wall="$(awk -v s="${start_ns}" -v e="${now_ns}" 'BEGIN { printf "%.3f", (e - s) / 1000000000 }')"
  if [[ "${status}" != "TIMEOUT" && "${rc}" -ne 0 ]]; then
    status="FAIL"
  fi
  if [[ -z "${last_rss}" ]]; then
    current_rss="NA"
  fi
  if [[ "${max_rss}" == "NA" && "${current_rss}" != "NA" ]]; then
    max_rss="${current_rss}"
  fi
  cat "${out_file}" >>"${log_file}" || true
  cat "${err_file}" >>"${log_file}" || true
  local xfer_hit xfer_insert xfer_spill central_hit central_insert
  xfer_hit="$(awk '/hz11_shim_exit_stats/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="xfer_hit") v=a[2]}} END{print v+0}' "${err_file}" "${out_file}" 2>/dev/null || echo 0)"
  xfer_insert="$(awk '/hz11_shim_exit_stats/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="xfer_insert") v=a[2]}} END{print v+0}' "${err_file}" "${out_file}" 2>/dev/null || echo 0)"
  xfer_spill="$(awk '/hz11_shim_exit_stats/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="xfer_spill") v=a[2]}} END{print v+0}' "${err_file}" "${out_file}" 2>/dev/null || echo 0)"
  central_hit="$(awk '/hz11_shim_exit_stats/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="central_hit") v=a[2]}} END{print v+0}' "${err_file}" "${out_file}" 2>/dev/null || echo 0)"
  central_insert="$(awk '/hz11_shim_exit_stats/ {for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="central_insert") v=a[2]}} END{print v+0}' "${err_file}" "${out_file}" 2>/dev/null || echo 0)"
  append_sample "${workload}" "${allocator}" "${run}" "${status}" "${rc}" "${wall}" \
    "${max_rss}" "${current_rss}" "${xfer_hit}" "${xfer_insert}" \
    "${xfer_spill}" "${central_hit}" "${central_insert}" "${timeout_sec}" \
    "${log_file}" "${err_file}" "${out_file}" ""
}

run_skip() {
  local workload="$1" allocator="$2" run="$3" reason="$4"
  append_sample "${workload}" "${allocator}" "${run}" "SKIP" "-" "NA" "NA" "NA" \
    0 0 0 0 0 0 "" "" "" "${reason}"
}

allocator_env_prefix() {
  local allocator="$1" lib="$2"
  if [[ "${allocator}" == "glibc" ]]; then
    printf 'env'
  elif [[ "${allocator}" == "hz11-span-transfer" ]]; then
    printf 'env LD_PRELOAD=%q HZ11_DUMP_STATS=1' "${lib}"
  else
    printf 'env LD_PRELOAD=%q' "${lib}"
  fi
}

add_hz11_diag_env() {
  local allocator="$1"
  local -n cmd_ref="$2"
  case "${allocator}" in
    hz11-span-transfer)
      cmd_ref+=(HZ11_DUMP_STATS=1) ;;
    hz11-thread-exit)
      cmd_ref+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CURRENT_SPAN_POOL=1) ;;
    hz11-thread-exit-cap)
      cmd_ref+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CURRENT_SPAN_POOL=1 \
        HZ11_DUMP_CENTRAL_CLASSES=1) ;;
    hz11-thread-exit-cap-batch32)
      cmd_ref+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CURRENT_SPAN_POOL=1 \
        HZ11_DUMP_CENTRAL_CLASSES=1) ;;
    hz11-thread-exit-cap-batch32-fineclass)
      cmd_ref+=(HZ11_DUMP_STATS=1 HZ11_DUMP_CURRENT_SPAN_POOL=1 \
        HZ11_DUMP_CENTRAL_CLASSES=1) ;;
  esac
}

run_python_alloc() {
  local allocator="$1" lib="$2" run="$3"
  local -a cmd=(env PYTHONMALLOC=malloc)
  if [[ -n "${lib}" ]]; then
    cmd+=(LD_PRELOAD="${lib}")
  fi
  add_hz11_diag_env "${allocator}" cmd
  cmd+=(/usr/bin/python3 - "${PYTHON_LOOPS}")
  run_sampled python_alloc "${allocator}" "${run}" 60 "${cmd[@]}" <<'PY'
import sys
loops = int(sys.argv[1])
bag = []
for r in range(loops):
    for i in range(20000):
        bag.append(bytearray((i % 4096) + 1))
    del bag[::3]
    bag = bag[-20000:]
print(len(bag))
PY
}

run_larson() {
  local allocator="$1" lib="$2" run="$3"
  if [[ -z "${larson_bin}" ]]; then run_skip larson "${allocator}" "${run}" "larson binary not found"; return 0; fi
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then cmd+=(LD_PRELOAD="${lib}"); fi
  add_hz11_diag_env "${allocator}" cmd
  cmd+=("${larson_bin}" "${LARSON_SECONDS}" "${LARSON_MIN}" "${LARSON_MAX}" \
    "${LARSON_CHUNKS}" "${LARSON_ROUNDS}" "${LARSON_SEED}" "${LARSON_THREADS}")
  run_sampled larson "${allocator}" "${run}" 30 "${cmd[@]}"
}

run_xmalloc_test() {
  local allocator="$1" lib="$2" run="$3"
  if [[ -z "${xmalloc_bin}" ]]; then run_skip xmalloc_test "${allocator}" "${run}" "xmalloc-test binary not found"; return 0; fi
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then cmd+=(LD_PRELOAD="${lib}"); fi
  add_hz11_diag_env "${allocator}" cmd
  cmd+=("${xmalloc_bin}" -w "${XMALLOC_WORKERS}" -t "${XMALLOC_SECONDS}" -s "${XMALLOC_SIZE}")
  run_sampled xmalloc_test "${allocator}" "${run}" 30 "${cmd[@]}"
}

run_sh6bench() {
  local allocator="$1" lib="$2" run="$3"
  if [[ -z "${sh6bench_bin}" ]]; then run_skip sh6bench "${allocator}" "${run}" "sh6bench binary not found"; return 0; fi
  local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
  printf '%s\n%s\n%s\n%s\n' "${SH6_CALL_COUNT}" "${SH6_MIN_BLOCK}" \
    "${SH6_MAX_BLOCK}" "${SH6_THREADS}" >"${input}"
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then cmd+=(LD_PRELOAD="${lib}"); fi
  add_hz11_diag_env "${allocator}" cmd
  cmd+=("${sh6bench_bin}" "${input}")
  run_sampled sh6bench "${allocator}" "${run}" 30 "${cmd[@]}"
}

run_cache_scratch() {
  local allocator="$1" lib="$2" run="$3"
  if [[ -z "${cache_scratch_bin}" ]]; then run_skip cache_scratch "${allocator}" "${run}" "cache-scratch binary not found"; return 0; fi
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then cmd+=(LD_PRELOAD="${lib}"); fi
  add_hz11_diag_env "${allocator}" cmd
  cmd+=("${cache_scratch_bin}" "${CACHE_SCRATCH_THREADS}" "${CACHE_SCRATCH_ITERATIONS}" \
    "${CACHE_SCRATCH_OBJECT_SIZE}" "${CACHE_SCRATCH_REPETITIONS}" "${CACHE_SCRATCH_CONCURRENCY}")
  run_sampled cache_scratch "${allocator}" "${run}" 30 "${cmd[@]}"
}

run_mstress() {
  local allocator="$1" lib="$2" run="$3"
  if [[ -z "${mstress_bin}" ]]; then run_skip mstress "${allocator}" "${run}" "mstress binary not found"; return 0; fi
  local -a cmd=(env)
  if [[ -n "${lib}" ]]; then cmd+=(LD_PRELOAD="${lib}"); fi
  add_hz11_diag_env "${allocator}" cmd
  cmd+=("${mstress_bin}" "${MSTRESS_THREADS}" "${MSTRESS_SCALE}" "${MSTRESS_ITER}")
  run_sampled mstress "${allocator}" "${run}" 30 "${cmd[@]}"
}

for i in "${!alloc_names[@]}"; do
  allocator="${alloc_names[$i]}"
  lib="${alloc_libs[$i]}"
  skip_reason="${alloc_skip_reasons[$i]}"
  for run in $(seq 1 "${RUNS}"); do
    if [[ -n "${skip_reason}" ]]; then
      for workload in python_alloc larson xmalloc_test sh6bench cache_scratch mstress; do
        run_skip "${workload}" "${allocator}" "${run}" "${skip_reason}"
      done
      continue
    fi
    [[ "${RUN_PYTHON_ALLOC}" == "1" ]] && run_python_alloc "${allocator}" "${lib}" "${run}"
    [[ "${RUN_LARSON}" == "1" ]] && run_larson "${allocator}" "${lib}" "${run}"
    [[ "${RUN_XMALLOC}" == "1" ]] && run_xmalloc_test "${allocator}" "${lib}" "${run}"
    [[ "${RUN_SH6BENCH}" == "1" ]] && run_sh6bench "${allocator}" "${lib}" "${run}"
    [[ "${RUN_CACHE_SCRATCH}" == "1" ]] && run_cache_scratch "${allocator}" "${lib}" "${run}"
    [[ "${RUN_MSTRESS}" == "1" ]] && run_mstress "${allocator}" "${lib}" "${run}"
  done
done

python3 - "${csv}" "${summary_md}" "${RUNS}" "${ALLOCATORS}" \
  "${HZ11_MACRO_CANDIDATE}" "${HZ11_REQUIRE_SPAN_SOA_CHECK}" \
  "${HZ11_COUNTER_EXPECT_INSERT_MIN}" <<'PY'
import csv
import math
import statistics
import sys
from collections import defaultdict

src, dst, runs, allocators_arg, candidate, require_span_soa_arg, counter_insert_min_arg = sys.argv[1:8]
require_span_soa = require_span_soa_arg == "1"
counter_insert_min = float(counter_insert_min_arg)
rows = list(csv.DictReader(open(src, newline="")))

def completed(row):
    return row["status"] == "OK" and row["wall_sec"] not in ("", "NA")

groups = defaultdict(list)
for row in rows:
    if completed(row):
        groups[(row["workload"], row["allocator"])].append(row)

workloads = sorted({row["workload"] for row in rows})
allocators = [item for item in allocators_arg.split(",") if item]

def nums(items, key):
    out = []
    for item in items:
        value = item.get(key, "")
        if value not in ("", "NA", "-"):
            out.append(float(value))
    return out

def med(items, key):
    values = nums(items, key)
    return statistics.median(values) if values else math.nan

def total(items, key):
    return sum(nums(items, key))

def have(workload, allocator):
    return bool(groups.get((workload, allocator)))

def ratio(workload, cand, base, key="wall_sec"):
    if not have(workload, cand) or not have(workload, base):
        return math.nan
    b = med(groups[(workload, base)], key)
    c = med(groups[(workload, cand)], key)
    return c / b if b and not math.isnan(b) and not math.isnan(c) else math.nan

checks = []
def check(name, ok, detail):
    checks.append((name, bool(ok), detail))

candidate_failures = [r for r in rows if r["allocator"] == candidate and r["status"] in ("FAIL", "TIMEOUT")]
check(f"{candidate} has no crashes/timeouts", not candidate_failures,
      f"failures={len(candidate_failures)}")

available_vs_soa = [w for w in workloads if have(w, candidate) and have(w, "hz11-span-soa")]
within_soa = [w for w in available_vs_soa if ratio(w, candidate, "hz11-span-soa") <= 1.05]
need = math.ceil(len(available_vs_soa) * 0.75) if available_vs_soa else 1
span_soa_ok = len(within_soa) >= need if available_vs_soa else not require_span_soa
check(f"{candidate} wall time is within 5% of span-soa on >=3/4 available rows",
      span_soa_ok,
      f"{len(within_soa)}/{len(available_vs_soa)} rows within 1.05x" +
      ("" if available_vs_soa or require_span_soa else " (not requested)"))

available_vs_tc = [w for w in workloads if have(w, candidate) and have(w, "tcmalloc")]
tc_wins = [w for w in available_vs_tc if ratio(w, candidate, "tcmalloc") < 1.0]
tc_within_low_rss = []
for w in available_vs_tc:
    wall_ratio = ratio(w, candidate, "tcmalloc")
    rss_ratio = ratio(w, candidate, "tcmalloc", key="max_rss_kb")
    if wall_ratio <= 1.25 and rss_ratio < 0.90:
        tc_within_low_rss.append(w)
check(f"{candidate} has a macro tcmalloc win or within 1.25x with lower RSS",
      bool(tc_wins or tc_within_low_rss),
      f"wins={tc_wins} within_1.25x_lower_rss={tc_within_low_rss}")

rss_bad = []
current_bad = []
for w in available_vs_tc:
    max_ratio = ratio(w, candidate, "tcmalloc", key="max_rss_kb")
    cur_ratio = ratio(w, candidate, "tcmalloc", key="current_rss_kb")
    if not math.isnan(max_ratio) and max_ratio > 1.25:
        rss_bad.append((w, max_ratio))
    if not math.isnan(cur_ratio) and cur_ratio > 1.25:
        current_bad.append((w, cur_ratio))
check(f"{candidate} max RSS <= tcmalloc*1.25 on completed rows", not rss_bad,
      ", ".join(f"{w}={r:.3f}" for w, r in rss_bad) or "ok")
check(f"{candidate} current RSS <= tcmalloc*1.25 where measurable", not current_bad,
      ", ".join(f"{w}={r:.3f}" for w, r in current_bad) or "ok")

churn_rows = [w for w in ("larson", "xmalloc_test", "sh6bench", "cache_scratch", "mstress") if have(w, candidate)]
counter_missing = []
counter_not_expected = []
counter_hit_rows = []
for w in churn_rows:
    items = groups[(w, candidate)]
    inserted = total(items, "xfer_insert")
    hits = total(items, "xfer_hit")
    if inserted >= counter_insert_min and hits > 0:
        counter_hit_rows.append(w)
    elif inserted >= counter_insert_min:
        counter_missing.append(w)
    else:
        counter_not_expected.append(w)
check(f"{candidate} counters fire on multi-thread/churn rows where expected",
      not counter_missing and bool(counter_hit_rows),
      (("hit_rows=" + ",".join(counter_hit_rows)) if counter_hit_rows else "hit_rows=none") +
      ((" missing=" + ",".join(counter_missing)) if counter_missing else "") +
      ((" no_insert=" + ",".join(counter_not_expected)) if counter_not_expected else ""))

if any(not ok for name, ok, detail in checks if "crashes" in name):
    verdict = "NO-GO"
elif all(ok for _, ok, _ in checks):
    verdict = "GO"
else:
    verdict = "MIXED"

def fmt(v):
    return "NA" if math.isnan(v) else f"{v:.3f}"

with open(dst, "w", encoding="utf-8") as f:
    f.write("# HZ11 Macro Speed Lane Gate L1\n\n")
    f.write(f"Conditions: RUNS={runs}. Candidate: `{candidate}`. Allocators: " + ", ".join(f"`{a}`" for a in allocators) + ".\n\n")
    f.write(f"Verdict: **{verdict}**\n\n")
    f.write("## Summary\n\n")
    f.write("| Workload | Allocator | status runs | median wall sec | median max RSS KiB | median current RSS KiB | xfer_hit | xfer_insert |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|\n")
    for w in workloads:
        for a in allocators:
            all_items = [r for r in rows if r["workload"] == w and r["allocator"] == a]
            ok_items = groups.get((w, a), [])
            if not all_items:
                continue
            statuses = defaultdict(int)
            for item in all_items:
                statuses[item["status"]] += 1
            status_text = " ".join(f"{k}:{statuses[k]}" for k in sorted(statuses))
            f.write(
                f"| {w} | {a} | {status_text} | {fmt(med(ok_items, 'wall_sec'))} | "
                f"{fmt(med(ok_items, 'max_rss_kb'))} | {fmt(med(ok_items, 'current_rss_kb'))} | "
                f"{int(total(ok_items, 'xfer_hit'))} | {int(total(ok_items, 'xfer_insert'))} |\n"
            )
    f.write("\n## Ratios\n\n")
    f.write(f"| Workload | {candidate}/span-soa wall | {candidate}/tcmalloc wall | {candidate}/tcmalloc max RSS | {candidate}/tcmalloc current RSS | {candidate}/hz11-span-transfer wall | {candidate}/hz11-span-transfer max RSS |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|\n")
    for w in workloads:
        f.write(
            f"| {w} | {fmt(ratio(w, candidate, 'hz11-span-soa'))} | "
            f"{fmt(ratio(w, candidate, 'tcmalloc'))} | "
            f"{fmt(ratio(w, candidate, 'tcmalloc', key='max_rss_kb'))} | "
            f"{fmt(ratio(w, candidate, 'tcmalloc', key='current_rss_kb'))} | "
            f"{fmt(ratio(w, candidate, 'hz11-span-transfer'))} | "
            f"{fmt(ratio(w, candidate, 'hz11-span-transfer', key='max_rss_kb'))} |\n"
        )
    f.write("\n## Gate\n\n")
    f.write("| Check | Result | Detail |\n")
    f.write("|---|---|---|\n")
    for name, ok, detail in checks:
        f.write(f"| {name} | {'PASS' if ok else 'FAIL'} | {detail} |\n")
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] HZ11 macro speed lane logs saved to ${OUTDIR}"
echo "[DONE] summary: ${summary_md}"
