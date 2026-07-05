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
BASE_PORT="${BASE_PORT:-6399}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" preload >/dev/null

log="${OUTDIR}/combined.log"
summary="${OUTDIR}/summary.tsv"
median_summary="${OUTDIR}/median_summary.tsv"
: >"${log}"
printf 'workload\tallocator\trun\twall_sec\tmax_rss_kb\textra\n' >"${summary}"

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

declare -a alloc_names=("glibc" "hz10")
declare -a alloc_libs=("" "${ROOT}/libhz10.so")

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

{
  echo "# HZ10 macro preload matrix"
  echo "# generated=${STAMP}"
  echo "# RUNS=${RUNS} REDIS_OPS=${REDIS_OPS} REDIS_CLIENTS=${REDIS_CLIENTS} PYTHON_LOOPS=${PYTHON_LOOPS}"
  echo "# hz10=${ROOT}/libhz10.so"
  echo "# tcmalloc=${tcmalloc_lib:-SKIP}"
  echo "# mimalloc=${mimalloc_lib:-SKIP}"
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
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "${workload}" "${allocator}" "${run}" \
    "${wall}" "${rss}" "-" >>"${summary}"
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
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "redis_server_rss_kb" "${allocator}" \
    "${run}" "-" "${server_rss:-NA}" "server" >>"${summary}"
  redis-cli -p "${port}" shutdown nosave >/dev/null 2>&1 || true
  trap - RETURN
}

for i in "${!alloc_names[@]}"; do
  name="${alloc_names[$i]}"
  lib="${alloc_libs[$i]}"
  for run in $(seq 1 "${RUNS}"); do
    run_python_alloc "${name}" "${lib}" "${run}"
    run_redis_setget "${name}" "${lib}" "${run}"
  done
done

python3 - <<'PY' "${summary}" "${median_summary}"
import csv
import statistics
import sys

summary_path, out_path = sys.argv[1:3]
rows = []
rss_only = []
with open(summary_path, newline="") as f:
    for row in csv.DictReader(f, delimiter="\t"):
        if row["wall_sec"] == "-":
            rss_only.append(row)
            continue
        rows.append(row)

by_key = {}
for row in rows:
    key = (row["workload"], row["allocator"])
    by_key.setdefault(key, {"wall": [], "rss": []})
    by_key[key]["wall"].append(float(row["wall_sec"]))
    by_key[key]["rss"].append(float(row["max_rss_kb"]))

glibc_wall = {}
for (workload, allocator), vals in by_key.items():
    if allocator == "glibc":
        glibc_wall[workload] = statistics.median(vals["wall"])

rss_by_key = {}
for row in rss_only:
    key = (row["workload"], row["allocator"])
    rss_by_key.setdefault(key, []).append(float(row["max_rss_kb"]))

with open(out_path, "w", newline="") as f:
    w = csv.writer(f, delimiter="\t", lineterminator="\n")
    w.writerow(["workload", "allocator", "median_wall_sec",
                "median_max_rss_kb", "wall_vs_glibc"])
    for workload, allocator in sorted(by_key):
        vals = by_key[(workload, allocator)]
        wall = statistics.median(vals["wall"])
        rss = statistics.median(vals["rss"])
        base = glibc_wall.get(workload)
        ratio = "-" if not base else f"{wall / base:.3f}"
        w.writerow([workload, allocator, f"{wall:.3f}", f"{rss:.0f}", ratio])
    for workload, allocator in sorted(rss_by_key):
        rss = statistics.median(rss_by_key[(workload, allocator)])
        w.writerow([workload, allocator, "-", f"{rss:.0f}", "-"])
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
