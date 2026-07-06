#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz10_larson_thread_churn_attribution_l0}"
RUNS="${RUNS:-3}"
THREADS_CSV="${THREADS_CSV:-1,2,4}"
CHUNKS_CSV="${CHUNKS_CSV:-32,64,128}"
LARSON_SECONDS="${LARSON_SECONDS:-1}"
LARSON_MIN="${LARSON_MIN:-8}"
LARSON_MAX="${LARSON_MAX:-128}"
LARSON_ROUNDS="${LARSON_ROUNDS:-1}"
LARSON_SEED="${LARSON_SEED:-12345}"
ALLOCATORS_CSV="${ALLOCATORS_CSV:-glibc,hz10-thread-stats,hz10-coarse,hz10-base,hz10+orphan}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" preload preload-front preload-thread-stats preload-coarse preload-base preload-fine preload-orphan-adoption preload-orphan-partial >/dev/null

log="${OUTDIR}/combined.log"
summary="${OUTDIR}/summary.tsv"
thread_stats="${OUTDIR}/thread_stats.tsv"
thread_totals="${OUTDIR}/thread_totals.tsv"
attribution_summary="${OUTDIR}/attribution_summary.tsv"
: >"${log}"
printf 'allocator\tthreads\tchunks\trun\twall_sec\tcurrent_rss_kb\tthroughput\textra\n' >"${summary}"
printf 'allocator\tthreads\tchunks\trun\tclass_id\tslot_size\tslot_count\tactive_pages\tretired_pages\tpage_bytes\tevictions\tretired\treclaimed_ready\treclaimed_sweep\treclaimed_local_free\n' >"${thread_stats}"
printf 'allocator\tthreads\tchunks\trun\tdump_index\tactive_pages\tretired_pages\tpage_bytes\tevictions\tretired\treclaimed_ready\treclaimed_sweep\treclaimed_local_free\n' >"${thread_totals}"
printf 'allocator\tthreads\tchunks\trun\tcurrent_rss_kb\tthread_page_bytes\tthread_page_explain_ratio\tactive_pages\tretired_pages\tdumps\tthroughput\n' >"${attribution_summary}"

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

larson_bin="$(find_larson_bin || true)"
if [[ -z "${larson_bin}" ]]; then
  echo "larson executable not found" >&2
  exit 2
fi

lib_for_allocator() {
  case "$1" in
    glibc) printf '%s\n' "" ;;
    hz10) printf '%s\n' "${ROOT}/libhz10.so" ;;
    hz10-thread-stats) printf '%s\n' "${ROOT}/libhz10_thread_stats.so" ;;
    hz10-front) printf '%s\n' "${ROOT}/libhz10_front.so" ;;
    hz10-coarse) printf '%s\n' "${ROOT}/libhz10_coarse.so" ;;
    hz10-base) printf '%s\n' "${ROOT}/libhz10_base.so" ;;
    hz10+fine) printf '%s\n' "${ROOT}/libhz10_fine.so" ;;
    hz10+orphan) printf '%s\n' "${ROOT}/libhz10_orphan.so" ;;
    hz10+orphan-partial) printf '%s\n' "${ROOT}/libhz10_orphan_partial.so" ;;
    *) return 1 ;;
  esac
}

run_sampled() {
  local allocator="$1"
  local threads="$2"
  local chunks="$3"
  local run="$4"
  local lib="$5"
  local out_file="${OUTDIR}/larson_${allocator}_${threads}t_${chunks}c_${run}.out"
  local err_file="${OUTDIR}/larson_${allocator}_${threads}t_${chunks}c_${run}.err"
  local start_ns end_ns wall rss rc
  local -a env_cmd=(env HZ10_SHIM_TOLERATE_FOREIGN=1)
  if [[ "${allocator}" == hz10* ]]; then
    env_cmd+=(HZ10_SHIM_EXIT_STATS=1 HZ10_SHIM_THREAD_EXIT_STATS=1)
    env_cmd+=(HZ10_SHIM_EXIT_STATS_CLASSES="${HZ10_SHIM_EXIT_STATS_CLASSES:-0}")
  fi
  if [[ -n "${lib}" ]]; then
    env_cmd+=(LD_PRELOAD="${lib}")
  fi

  echo "## allocator=${allocator} threads=${threads} chunks=${chunks} run=${run}" >>"${log}"
  start_ns="$(date +%s%N)"
  "${env_cmd[@]}" "${larson_bin}" "${LARSON_SECONDS}" "${LARSON_MIN}" \
    "${LARSON_MAX}" "${chunks}" "${LARSON_ROUNDS}" "${LARSON_SEED}" \
    "${threads}" >"${out_file}" 2>"${err_file}" &
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
  cat "${out_file}" >>"${log}" || true
  echo "stderr_file=${err_file} stderr_lines=$(wc -l <"${err_file}")" >>"${log}"
  if [[ "${rc}" -ne 0 ]]; then
    echo "larson failed rc=${rc}" >>"${log}"
    return "${rc}"
  fi
  local throughput
  throughput="$(awk '/Throughput/ {for (i=1;i<=NF;i++) if ($i == "=" && i<NF) print $(i+1)}' "${out_file}" | tail -n1)"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "${allocator}" "${threads}" \
    "${chunks}" "${run}" "${wall}" "${rss}" "${throughput:-NA}" \
    "seconds=${LARSON_SECONDS},min=${LARSON_MIN},max=${LARSON_MAX}" >>"${summary}"
}

IFS=',' read -r -a allocators <<<"${ALLOCATORS_CSV}"
IFS=',' read -r -a thread_values <<<"${THREADS_CSV}"
IFS=',' read -r -a chunk_values <<<"${CHUNKS_CSV}"

for allocator in "${allocators[@]}"; do
  lib="$(lib_for_allocator "${allocator}")"
  for threads in "${thread_values[@]}"; do
    for chunks in "${chunk_values[@]}"; do
      for run in $(seq 1 "${RUNS}"); do
        run_sampled "${allocator}" "${threads}" "${chunks}" "${run}" "${lib}"
      done
    done
  done
done

python3 - <<'PY' "${OUTDIR}" "${thread_stats}" "${thread_totals}"
import pathlib
import re
import sys

outdir = pathlib.Path(sys.argv[1])
thread_stats = pathlib.Path(sys.argv[2])
thread_totals = pathlib.Path(sys.argv[3])
name_re = re.compile(r"larson_(?P<allocator>.+)_(?P<threads>\d+)t_(?P<chunks>\d+)c_(?P<run>\d+)\.err$")
class_re = re.compile(
    r"hz10_shim_exit_stats class=(?P<class_id>\d+) slot_size=(?P<slot_size>\d+) "
    r"slot_count=(?P<slot_count>\d+) active_pages=(?P<active>\d+) "
    r"retired_pages=(?P<retired_pages>\d+) max_retired=(?P<max_retired>\d+) "
    r"evictions=(?P<evictions>\d+) retired=(?P<retired>\d+) "
    r"reclaimed_ready=(?P<ready>\d+) reclaimed_sweep=(?P<sweep>\d+) "
    r"reclaimed_local_free=(?P<local>\d+)"
)
totals_re = re.compile(
    r"hz10_shim_exit_stats class_totals active_pages=(?P<active>\d+) "
    r"retired_pages=(?P<retired_pages>\d+) max_retired_sum=(?P<max_retired>\d+) "
    r"page_bytes=(?P<page_bytes>\d+) evictions=(?P<evictions>\d+) "
    r"retired=(?P<retired>\d+) reclaimed_ready=(?P<ready>\d+) "
    r"reclaimed_sweep=(?P<sweep>\d+) reclaimed_local_free=(?P<local>\d+)"
)

with thread_stats.open("a") as class_out, thread_totals.open("a") as total_out:
    for path in sorted(outdir.glob("larson_*.err")):
        m = name_re.match(path.name)
        if not m:
            continue
        ident = m.groupdict()
        dump_index = 0
        for line in path.read_text(errors="ignore").splitlines():
            tm = totals_re.search(line)
            if tm:
                dump_index += 1
                d = tm.groupdict()
                total_out.write(
                    "{allocator}\t{threads}\t{chunks}\t{run}\t{dump_index}\t"
                    "{active}\t{retired_pages}\t{page_bytes}\t{evictions}\t"
                    "{retired}\t{ready}\t{sweep}\t{local}\n".format(
                        dump_index=dump_index, **ident, **d
                    )
                )
                continue
            cm = class_re.search(line)
            if not cm:
                continue
            d = cm.groupdict()
            page_bytes = (int(d["active"]) + int(d["retired_pages"])) * 65536
            class_out.write(
                "{allocator}\t{threads}\t{chunks}\t{run}\t"
                "{class_id}\t{slot_size}\t{slot_count}\t{active}\t"
                "{retired_pages}\t{page_bytes}\t{evictions}\t{retired}\t"
                "{ready}\t{sweep}\t{local}\n".format(
                    page_bytes=page_bytes, **ident, **d
                )
            )
PY

python3 - <<'PY' "${summary}" "${thread_totals}" "${attribution_summary}"
import collections
import csv
import sys

summary_path, totals_path, out_path = sys.argv[1:4]
rss = {}
throughput = {}
with open(summary_path, newline="") as f:
    for row in csv.DictReader(f, delimiter="\t"):
        key = (row["allocator"], row["threads"], row["chunks"], row["run"])
        rss[key] = int(row["current_rss_kb"])
        throughput[key] = row["throughput"]

agg = collections.defaultdict(lambda: {
    "page_bytes": 0,
    "active_pages": 0,
    "retired_pages": 0,
    "dumps": 0,
})
with open(totals_path, newline="") as f:
    for row in csv.DictReader(f, delimiter="\t"):
        key = (row["allocator"], row["threads"], row["chunks"], row["run"])
        agg[key]["page_bytes"] += int(row["page_bytes"])
        agg[key]["active_pages"] += int(row["active_pages"])
        agg[key]["retired_pages"] += int(row["retired_pages"])
        agg[key]["dumps"] += 1

with open(out_path, "w", newline="") as f:
    w = csv.writer(f, delimiter="\t", lineterminator="\n")
    w.writerow([
        "allocator", "threads", "chunks", "run", "current_rss_kb",
        "thread_page_bytes", "thread_page_explain_ratio", "active_pages",
        "retired_pages", "dumps", "throughput",
    ])
    for key in sorted(rss, key=lambda x: (x[0], int(x[1]), int(x[2]), int(x[3]))):
        vals = agg.get(key, {
            "page_bytes": 0,
            "active_pages": 0,
            "retired_pages": 0,
            "dumps": 0,
        })
        current_bytes = rss[key] * 1024
        ratio = 0.0 if current_bytes == 0 else vals["page_bytes"] / current_bytes
        w.writerow([
            *key,
            rss[key],
            vals["page_bytes"],
            f"{ratio:.3f}",
            vals["active_pages"],
            vals["retired_pages"],
            vals["dumps"],
            throughput.get(key, "NA"),
        ])
PY

{
  echo "# HZ10 larson thread churn attribution"
  echo "# generated=${STAMP}"
  echo "# larson=${larson_bin}"
  echo "# RUNS=${RUNS} THREADS_CSV=${THREADS_CSV} CHUNKS_CSV=${CHUNKS_CSV}"
  echo "# LARSON_SECONDS=${LARSON_SECONDS} LARSON_MIN=${LARSON_MIN} LARSON_MAX=${LARSON_MAX} LARSON_ROUNDS=${LARSON_ROUNDS}"
  echo
  echo "## summary"
  cat "${summary}"
  echo
  echo "## thread_stats"
  echo "path=${thread_stats}"
  echo "rows=$(($(wc -l <"${thread_stats}") - 1))"
  echo
  echo "## thread_totals"
  echo "path=${thread_totals}"
  echo "rows=$(($(wc -l <"${thread_totals}") - 1))"
  echo
  echo "## attribution_summary"
  cat "${attribution_summary}"
} >>"${log}"

cat "${log}"
echo "[hz10-larson-thread-churn] wrote ${OUTDIR}" >&2
