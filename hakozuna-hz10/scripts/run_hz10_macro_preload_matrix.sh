#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/bench/lib/hz10_bench_common.sh"

STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz10_macro_preload_matrix}"
RUNS="${RUNS:-3}"
REDIS_OPS="${REDIS_OPS:-20000}"
REDIS_CLIENTS="${REDIS_CLIENTS:-32}"
PYTHON_LOOPS="${PYTHON_LOOPS:-80}"
RUN_LARSON="${RUN_LARSON:-1}"
RUN_XMALLOC="${RUN_XMALLOC:-1}"
RUN_CACHE_SCRATCH="${RUN_CACHE_SCRATCH:-1}"
RUN_MSTRESS="${RUN_MSTRESS:-1}"
RUN_SH6BENCH="${RUN_SH6BENCH:-1}"
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
CACHE_SCRATCH_THREADS="${CACHE_SCRATCH_THREADS:-4}"
CACHE_SCRATCH_ITERATIONS="${CACHE_SCRATCH_ITERATIONS:-5000}"
CACHE_SCRATCH_OBJECT_SIZE="${CACHE_SCRATCH_OBJECT_SIZE:-256}"
CACHE_SCRATCH_REPETITIONS="${CACHE_SCRATCH_REPETITIONS:-5000}"
CACHE_SCRATCH_CONCURRENCY="${CACHE_SCRATCH_CONCURRENCY:-4}"
MSTRESS_THREADS="${MSTRESS_THREADS:-8}"
MSTRESS_SCALE="${MSTRESS_SCALE:-80}"
MSTRESS_ITER="${MSTRESS_ITER:-3}"
SH6_CALL_COUNT="${SH6_CALL_COUNT:-100000}"
SH6_MIN_BLOCK="${SH6_MIN_BLOCK:-1}"
SH6_MAX_BLOCK="${SH6_MAX_BLOCK:-1000}"
SH6_THREADS="${SH6_THREADS:-4}"
BASE_PORT="${BASE_PORT:-6399}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" preload preload-coarse preload-base preload-fine preload-orphan-adoption preload-orphan-partial >/dev/null

log="${OUTDIR}/combined.log"
summary="${OUTDIR}/summary.tsv"
median_summary="${OUTDIR}/median_summary.tsv"
: >"${log}"
printf 'workload\tallocator\trun\twall_sec\tmax_rss_kb\tcurrent_rss_kb\textra\n' >"${summary}"

find_mimalloc_lib() {
  if [[ -n "${MIMALLOC_LIB:-}" && -f "${MIMALLOC_LIB}" ]]; then
    printf '%s\n' "${MIMALLOC_LIB}"
    return 0
  fi
  local path
  for path in \
    /mnt/workdisk/public_share/hakmem/mimalloc-install/lib/libmimalloc.so.2.2 \
    /mnt/workdisk/public_share/hakmem/mimalloc_src/out/release/libmimalloc.so.2.2 \
    /mnt/workdisk/public_share/hakmem/mimalloc/build/libmimalloc.so.2.2 \
    /mnt/workdisk/public_share/hakmem/allocators/mimalloc/libmimalloc.so; do
    if [[ -f "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

find_larson_bin() {
  if [[ -n "${LARSON_BIN:-}" && -x "${LARSON_BIN}" ]]; then
    printf '%s\n' "${LARSON_BIN}"
    return 0
  fi
  local path
  for path in \
    /mnt/workdisk/public_share/hakmem/larson_system \
    /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/larson; do
    if [[ -x "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

find_mimalloc_bench_bin() {
  local name="$1"
  if [[ -n "${MIMALLOC_BENCH_DIR:-}" && -x "${MIMALLOC_BENCH_DIR}/${name}" ]]; then
    printf '%s\n' "${MIMALLOC_BENCH_DIR}/${name}"
    return 0
  fi
  local path
  for path in \
    "/mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/${name}" \
    "/mnt/workdisk/public_share/hakmem/mimalloc-bench/build/bench/${name}"; do
    if [[ -x "${path}" ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

declare -a alloc_names=("glibc" "hz10" "hz10-coarse" "hz10-base" "hz10+orphan")
declare -a alloc_libs=("" "${ROOT}/libhz10.so" "${ROOT}/libhz10_coarse.so" "${ROOT}/libhz10_base.so" "${ROOT}/libhz10_orphan.so")
declare -a compat_alloc_names=("hz10+fine" "hz10+orphan-partial")
declare -a compat_alloc_libs=("${ROOT}/libhz10_fine.so" "${ROOT}/libhz10_orphan_partial.so")

tcmalloc_lib="$(hz10_bench_find_tcmalloc_lib || true)"
if [[ -n "${tcmalloc_lib}" ]]; then
  alloc_names+=("tcmalloc")
  alloc_libs+=("${tcmalloc_lib}")
fi

mimalloc_lib="$(find_mimalloc_lib || true)"
if [[ -n "${mimalloc_lib}" ]]; then
  alloc_names+=("mimalloc")
  alloc_libs+=("${mimalloc_lib}")
fi

if [[ -n "${ALLOCATORS_CSV:-}" ]]; then
  declare -a all_alloc_names=("${alloc_names[@]}" "${compat_alloc_names[@]}")
  declare -a all_alloc_libs=("${alloc_libs[@]}" "${compat_alloc_libs[@]}")
  alloc_names=()
  alloc_libs=()
  IFS=',' read -r -a requested_allocators <<<"${ALLOCATORS_CSV}"
  for requested in "${requested_allocators[@]}"; do
    found=0
    for i in "${!all_alloc_names[@]}"; do
      if [[ "${all_alloc_names[$i]}" == "${requested}" ]]; then
        alloc_names+=("${all_alloc_names[$i]}")
        alloc_libs+=("${all_alloc_libs[$i]}")
        found=1
        break
      fi
    done
    if [[ "${found}" -ne 1 ]]; then
      echo "unknown or unavailable allocator in ALLOCATORS_CSV: ${requested}" >&2
      exit 2
    fi
  done
fi

larson_bin="$(find_larson_bin || true)"
xmalloc_bin="$(find_mimalloc_bench_bin xmalloc-test || true)"
cache_scratch_bin="$(find_mimalloc_bench_bin cache-scratch || true)"
mstress_bin="$(find_mimalloc_bench_bin mstress || true)"
sh6bench_bin="$(find_mimalloc_bench_bin sh6bench || true)"

{
  echo "# HZ10 macro preload matrix"
  echo "# generated=${STAMP}"
  echo "# RUNS=${RUNS} REDIS_OPS=${REDIS_OPS} REDIS_CLIENTS=${REDIS_CLIENTS} PYTHON_LOOPS=${PYTHON_LOOPS}"
  echo "# RUN_LARSON=${RUN_LARSON} LARSON_SECONDS=${LARSON_SECONDS} LARSON_MIN=${LARSON_MIN} LARSON_MAX=${LARSON_MAX} LARSON_CHUNKS=${LARSON_CHUNKS} LARSON_ROUNDS=${LARSON_ROUNDS} LARSON_SEED=${LARSON_SEED} LARSON_THREADS=${LARSON_THREADS}"
  echo "# RUN_XMALLOC=${RUN_XMALLOC} XMALLOC_WORKERS=${XMALLOC_WORKERS} XMALLOC_SECONDS=${XMALLOC_SECONDS} XMALLOC_SIZE=${XMALLOC_SIZE}"
  echo "# RUN_CACHE_SCRATCH=${RUN_CACHE_SCRATCH} CACHE_SCRATCH_THREADS=${CACHE_SCRATCH_THREADS} CACHE_SCRATCH_ITERATIONS=${CACHE_SCRATCH_ITERATIONS} CACHE_SCRATCH_OBJECT_SIZE=${CACHE_SCRATCH_OBJECT_SIZE} CACHE_SCRATCH_REPETITIONS=${CACHE_SCRATCH_REPETITIONS} CACHE_SCRATCH_CONCURRENCY=${CACHE_SCRATCH_CONCURRENCY}"
  echo "# RUN_MSTRESS=${RUN_MSTRESS} MSTRESS_THREADS=${MSTRESS_THREADS} MSTRESS_SCALE=${MSTRESS_SCALE} MSTRESS_ITER=${MSTRESS_ITER}"
  echo "# RUN_SH6BENCH=${RUN_SH6BENCH} SH6_CALL_COUNT=${SH6_CALL_COUNT} SH6_MIN_BLOCK=${SH6_MIN_BLOCK} SH6_MAX_BLOCK=${SH6_MAX_BLOCK} SH6_THREADS=${SH6_THREADS}"
  echo "# hz10=${ROOT}/libhz10.so"
  echo "# hz10_coarse=${ROOT}/libhz10_coarse.so"
  echo "# hz10_base=${ROOT}/libhz10_base.so"
  echo "# hz10_fine_compat=${ROOT}/libhz10_fine.so"
  echo "# hz10_orphan=${ROOT}/libhz10_orphan.so"
  echo "# hz10_orphan_partial_compat=${ROOT}/libhz10_orphan_partial.so"
  echo "# tcmalloc=${tcmalloc_lib:-SKIP}"
  echo "# mimalloc=${mimalloc_lib:-SKIP}"
  echo "# larson=${larson_bin:-SKIP}"
  echo "# xmalloc_test=${xmalloc_bin:-SKIP}"
  echo "# cache_scratch=${cache_scratch_bin:-SKIP}"
  echo "# mstress=${mstress_bin:-SKIP}"
  echo "# sh6bench=${sh6bench_bin:-SKIP}"
  echo "# allocators=${alloc_names[*]}"
} >>"${log}"

run_timed() {
  local workload="$1"
  local allocator="$2"
  local run="$3"
  shift 3
  local time_file="${OUTDIR}/${workload}_${allocator}_${run}.time"
  local out_file="${OUTDIR}/${workload}_${allocator}_${run}.out"
  local err_file="${OUTDIR}/${workload}_${allocator}_${run}.err"
  echo "## workload=${workload} allocator=${allocator} run=${run}" >>"${log}"
  /usr/bin/time -f 'wall_sec=%e max_rss_kb=%M' -o "${time_file}" "$@" \
    >"${out_file}" 2>"${err_file}"
  cat "${time_file}" >>"${log}"
  local wall rss
  wall="$(awk '{for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="wall_sec") print a[2]}}' "${time_file}")"
  rss="$(awk '{for (i=1;i<=NF;i++) {split($i,a,"="); if (a[1]=="max_rss_kb") print a[2]}}' "${time_file}")"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${workload}" "${allocator}" "${run}" \
    "${wall}" "${rss}" "-" "-" >>"${summary}"
}

run_timed_sampled() {
  local workload="$1"
  local allocator="$2"
  local run="$3"
  local extra="$4"
  shift 4
  local out_file="${OUTDIR}/${workload}_${allocator}_${run}.out"
  local err_file="${OUTDIR}/${workload}_${allocator}_${run}.err"
  echo "## workload=${workload} allocator=${allocator} run=${run}" >>"${log}"
  local start_ns end_ns wall rss rc
  start_ns="$(date +%s%N)"
  "$@" >"${out_file}" 2>"${err_file}" &
  local pid=$!
  rss=0
  while kill -0 "${pid}" 2>/dev/null; do
    local sample
    sample="$(ps -o rss= -p "${pid}" 2>/dev/null | awk '{print $1}' || true)"
    if [[ -n "${sample}" && "${sample}" != "0" && "${sample}" -gt "${rss}" ]]; then
      rss="${sample}"
    fi
    sleep 0.05
  done
  set +e
  wait "${pid}"
  rc=$?
  set -e
  end_ns="$(date +%s%N)"
  wall="$(awk -v s="${start_ns}" -v e="${end_ns}" 'BEGIN { printf "%.3f", (e - s) / 1000000000 }')"
  echo "wall_sec=${wall} sampled_current_rss_kb=${rss} rc=${rc}" >>"${log}"
  if [[ "${rc}" -ne 0 ]]; then
    cat "${err_file}" >>"${log}" || true
    return "${rc}"
  fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${workload}" "${allocator}" "${run}" \
    "${wall}" "${rss}" "${rss}" "${extra}" >>"${summary}"
}

run_python_alloc() {
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local -a env_cmd=(env PYTHONMALLOC=malloc HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi
  run_timed python_alloc "${allocator}" "${run}" "${env_cmd[@]}" \
    /usr/bin/python3 - "${PYTHON_LOOPS}" <<'PY'
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

run_redis_setget() {
  command -v redis-server >/dev/null || return 0
  command -v redis-benchmark >/dev/null || return 0
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local port=$((BASE_PORT + run))
  local dir="${OUTDIR}/redis_${allocator}_${run}"
  mkdir -p "${dir}"
  local -a server_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    server_cmd+=(LD_PRELOAD="${lib}")
  fi
  "${server_cmd[@]}" redis-server --port "${port}" --bind 127.0.0.1 \
    --save "" --appendonly no --dir "${dir}" --daemonize yes \
    --pidfile "${dir}/redis.pid" >>"${log}" 2>&1
  trap 'if [[ -f "${dir}/redis.pid" ]]; then kill "$(cat "${dir}/redis.pid")" 2>/dev/null || true; fi' RETURN
  for _ in $(seq 1 50); do
    if redis-cli -p "${port}" ping >/dev/null 2>&1; then
      break
    fi
    sleep 0.1
  done
  local -a client_cmd=(redis-benchmark -p "${port}" -n "${REDIS_OPS}" -c "${REDIS_CLIENTS}" -t set,get --csv)
  run_timed redis_setget "${allocator}" "${run}" "${client_cmd[@]}"
  local server_rss
  server_rss="$(ps -o rss= -p "$(cat "${dir}/redis.pid")" 2>/dev/null | awk '{print $1}' || true)"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "redis_server_rss_kb" "${allocator}" \
    "${run}" "-" "${server_rss:-NA}" "${server_rss:-NA}" "server" >>"${summary}"
  redis-cli -p "${port}" shutdown nosave >/dev/null 2>&1 || true
  trap - RETURN
}

run_larson() {
  [[ "${RUN_LARSON}" == "1" ]] || return 0
  [[ -n "${larson_bin}" ]] || return 0
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local -a env_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi
  run_timed_sampled larson "${allocator}" "${run}" \
    "seconds=${LARSON_SECONDS},min=${LARSON_MIN},max=${LARSON_MAX},chunks=${LARSON_CHUNKS},rounds=${LARSON_ROUNDS},seed=${LARSON_SEED},threads=${LARSON_THREADS}" \
    "${env_cmd[@]}" "${larson_bin}" "${LARSON_SECONDS}" "${LARSON_MIN}" \
    "${LARSON_MAX}" "${LARSON_CHUNKS}" "${LARSON_ROUNDS}" "${LARSON_SEED}" \
    "${LARSON_THREADS}"
  local throughput
  throughput="$(awk '/Throughput/ {for (i=1;i<=NF;i++) if ($i == "=" && i<NF) print $(i+1)}' \
    "${OUTDIR}/larson_${allocator}_${run}.out" | tail -n1)"
  if [[ -n "${throughput}" ]]; then
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "larson_throughput" "${allocator}" \
      "${run}" "-" "-" "-" "${throughput}" >>"${summary}"
  fi
}

run_xmalloc_test() {
  [[ "${RUN_XMALLOC}" == "1" ]] || return 0
  [[ -n "${xmalloc_bin}" ]] || return 0
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local -a env_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi
  run_timed xmalloc_test "${allocator}" "${run}" "${env_cmd[@]}" \
    "${xmalloc_bin}" -w "${XMALLOC_WORKERS}" -t "${XMALLOC_SECONDS}" \
    -s "${XMALLOC_SIZE}"
}

run_cache_scratch() {
  [[ "${RUN_CACHE_SCRATCH}" == "1" ]] || return 0
  [[ -n "${cache_scratch_bin}" ]] || return 0
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local -a env_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi
  run_timed cache_scratch "${allocator}" "${run}" "${env_cmd[@]}" \
    "${cache_scratch_bin}" "${CACHE_SCRATCH_THREADS}" \
    "${CACHE_SCRATCH_ITERATIONS}" "${CACHE_SCRATCH_OBJECT_SIZE}" \
    "${CACHE_SCRATCH_REPETITIONS}" "${CACHE_SCRATCH_CONCURRENCY}"
}

run_mstress() {
  [[ "${RUN_MSTRESS}" == "1" ]] || return 0
  [[ -n "${mstress_bin}" ]] || return 0
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local -a env_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi
  run_timed mstress "${allocator}" "${run}" "${env_cmd[@]}" \
    "${mstress_bin}" "${MSTRESS_THREADS}" "${MSTRESS_SCALE}" \
    "${MSTRESS_ITER}"
}

run_sh6bench() {
  [[ "${RUN_SH6BENCH}" == "1" ]] || return 0
  [[ -n "${sh6bench_bin}" ]] || return 0
  local allocator="$1"
  local lib="$2"
  local run="$3"
  local input="${OUTDIR}/sh6bench_${allocator}_${run}.in"
  printf '%s\n%s\n%s\n%s\n' "${SH6_CALL_COUNT}" "${SH6_MIN_BLOCK}" \
    "${SH6_MAX_BLOCK}" "${SH6_THREADS}" >"${input}"
  local -a env_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi
  run_timed sh6bench "${allocator}" "${run}" "${env_cmd[@]}" \
    "${sh6bench_bin}" "${input}"
}

for i in "${!alloc_names[@]}"; do
  name="${alloc_names[$i]}"
  lib="${alloc_libs[$i]}"
  for run in $(seq 1 "${RUNS}"); do
    run_python_alloc "${name}" "${lib}" "${run}"
    run_redis_setget "${name}" "${lib}" "${run}"
    run_larson "${name}" "${lib}" "${run}"
    run_xmalloc_test "${name}" "${lib}" "${run}"
    run_cache_scratch "${name}" "${lib}" "${run}"
    run_mstress "${name}" "${lib}" "${run}"
    run_sh6bench "${name}" "${lib}" "${run}"
  done
done

python3 - <<'PY' "${summary}" "${median_summary}"
import csv
import statistics
import sys

summary_path, out_path = sys.argv[1:3]
rows = []
rss_only = []
extra_only = []
with open(summary_path, newline="") as f:
    for row in csv.DictReader(f, delimiter="\t"):
        if row["wall_sec"] == "-":
            if row["max_rss_kb"] not in ("-", "NA", ""):
                rss_only.append(row)
            else:
                extra_only.append(row)
            continue
        rows.append(row)

by_key = {}
for row in rows:
    key = (row["workload"], row["allocator"])
    by_key.setdefault(key, {"wall": [], "rss": [], "current": []})
    by_key[key]["wall"].append(float(row["wall_sec"]))
    by_key[key]["rss"].append(float(row["max_rss_kb"]))
    if row["current_rss_kb"] != "-":
        by_key[key]["current"].append(float(row["current_rss_kb"]))

glibc_wall = {}
for (workload, allocator), vals in by_key.items():
    if allocator == "glibc":
        glibc_wall[workload] = statistics.median(vals["wall"])

rss_by_key = {}
for row in rss_only:
    key = (row["workload"], row["allocator"])
    rss_by_key.setdefault(key, {"rss": [], "current": []})
    rss_by_key[key]["rss"].append(float(row["max_rss_kb"]))
    if row["current_rss_kb"] not in ("-", "NA", ""):
        rss_by_key[key]["current"].append(float(row["current_rss_kb"]))

with open(out_path, "w", newline="") as f:
    w = csv.writer(f, delimiter="\t", lineterminator="\n")
    w.writerow(["workload", "allocator", "median_wall_sec",
                "median_max_rss_kb", "median_current_rss_kb",
                "wall_vs_glibc"])
    for workload, allocator in sorted(by_key):
        vals = by_key[(workload, allocator)]
        wall = statistics.median(vals["wall"])
        rss = statistics.median(vals["rss"])
        current = "-"
        if vals["current"]:
            current = f"{statistics.median(vals['current']):.0f}"
        base = glibc_wall.get(workload)
        ratio = "-" if not base else f"{wall / base:.3f}"
        w.writerow([workload, allocator, f"{wall:.3f}", f"{rss:.0f}",
                    current, ratio])
    for workload, allocator in sorted(rss_by_key):
        vals = rss_by_key[(workload, allocator)]
        rss = statistics.median(vals["rss"])
        current = "-"
        if vals["current"]:
            current = f"{statistics.median(vals['current']):.0f}"
        w.writerow([workload, allocator, "-", f"{rss:.0f}", current, "-"])
PY

{
  echo
  echo "## summary"
  cat "${summary}"
  echo
  echo "## median summary"
  cat "${median_summary}"
} >>"${log}"

cat "${log}"
echo "[hz10-macro-preload] wrote ${OUTDIR}" >&2
