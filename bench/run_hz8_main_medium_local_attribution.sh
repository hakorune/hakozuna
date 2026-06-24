#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

ALLOCATORS="${ALLOCATORS:-hz8}"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/bench_results/hz8_main_medium_local_attr_$(date -u +%Y%m%dT%H%M%SZ)}"
BUILD="${BUILD:-1}"
BENCH_BIN="${BENCH_BIN:-${ROOT_DIR}/bench/out/bench_matrix_malloc}"

usage() {
  cat <<'EOF'
Usage:
  bench/run_hz8_main_medium_local_attribution.sh [options]

Options:
  --allocators LIST   comma-separated allocators, default hz8
  --runs N            fresh process samples per row/allocator
  --threads N         worker threads
  --iters N           iterations per thread
  --outdir DIR        output directory
  --skip-build        do not build HZ8 preload or matrix harness
  --help              show this message

Rows:
  small/main/medium local0 with interleaved=0 and interleaved=1
  fixed medium class rows for 8K, 16K, 32K, and 64K
  touch and no-touch variants for fixed class rows
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --allocators)
      ALLOCATORS="$2"; shift 2 ;;
    --runs)
      RUNS="$2"; shift 2 ;;
    --threads)
      THREADS="$2"; shift 2 ;;
    --iters)
      ITERS="$2"; shift 2 ;;
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

mkdir -p "${OUTDIR}" "$(dirname "${BENCH_BIN}")"

if [[ "${BUILD}" -ne 0 ]]; then
  make -C "${ROOT_DIR}/hakozuna-hz8" preload
  "${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
    -pthread -o "${BENCH_BIN}" "${ROOT_DIR}/bench/bench_matrix_malloc.c"
fi

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"
declare -A allocator_libs
for alloc in "${allocator_list[@]}"; do
  lib="$(bench_find_allocator_library "${alloc}" || true)"
  if [[ "${alloc}" != "system" && -z "${lib}" ]]; then
    echo "[ERR] missing allocator library for ${alloc}" >&2
    bench_print_allocator_hints "${alloc}"
    exit 2
  fi
  allocator_libs["${alloc}"]="${lib}"
done

cat > "${OUTDIR}/README.log" <<EOF
[ATTR] ts=$(date -u +%Y%m%dT%H%M%SZ)
[ATTR] root=${ROOT_DIR}
[ATTR] bench_bin=${BENCH_BIN}
[ATTR] allocators=${ALLOCATORS}
[ATTR] runs=${RUNS}
[ATTR] threads=${THREADS}
[ATTR] iters=${ITERS}
[ATTR] allocator_behavior_sha=f916c803
EOF

csv="${OUTDIR}/samples.csv"
printf 'row,run,allocator,lib,throughput,steady,post_rss,peak_rss,minor_faults,work_ms,tail_ms,log\n' > "${csv}"

run_case() {
  local row="$1" run="$2" alloc="$3" args="$4"
  local lib="${allocator_libs[$alloc]}"
  local log="${OUTDIR}/${row}_run${run}_${alloc}.log"
  {
    echo "[CASE] row=${row}"
    echo "[CASE] run=${run}"
    echo "[CASE] alloc=${alloc}"
    echo "[CASE] lib=${lib:-system}"
    echo "[CASE] args=${args}"
    echo
  } > "${log}"
  read -r -a argv <<< "${args}"
  if [[ "${alloc}" == "system" ]]; then
    "${BENCH_BIN}" "${argv[@]}" >> "${log}" 2>&1
  else
    bench_run_with_allocator "${alloc}" "${lib}" "${BENCH_BIN}" "${argv[@]}" \
      >> "${log}" 2>&1
  fi
  awk -v row="${row}" -v run="${run}" -v alloc="${alloc}" \
      -v lib="${lib:-system}" -v logfile="${log}" '
    /^throughput / { for (i=1;i<=NF;++i) if ($i~/^median=/) { split($i,a,"="); throughput=a[2] } }
    /^steady_work / { for (i=1;i<=NF;++i) if ($i~/^throughput_median=/) { split($i,a,"="); steady=a[2] } }
    /^post_rss / { for (i=1;i<=NF;++i) if ($i~/^median=/) { split($i,a,"="); post=a[2] } }
    /^peak_rss / { for (i=1;i<=NF;++i) if ($i~/^median=/) { split($i,a,"="); peak=a[2] } }
    /^page_faults / { for (i=1;i<=NF;++i) if ($i~/^minor_median=/) { split($i,a,"="); faults=a[2] } }
    /^(phase_ms|interleaved_phase_ms) / {
      for (i=1;i<=NF;++i) {
        if ($i~/^(work_median|alloc_median)=/) { split($i,a,"="); work=a[2] }
        else if ($i~/^(tail_median|remote_median)=/) { split($i,a,"="); tail=a[2] }
      }
    }
    END { printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", row, run, alloc, lib, throughput, steady, post, peak, faults, work, tail, logfile }
  ' "${log}" >> "${csv}"
}

run_row() {
  local row="$1" args="$2" count="${#allocator_list[@]}"
  for run in $(seq 1 "${RUNS}"); do
    local offset=$(( (run - 1) % count ))
    for idx in $(seq 0 $((count - 1))); do
      local alloc="${allocator_list[$(((idx + offset) % count))]}"
      echo "[RUN] row=${row} run=${run} alloc=${alloc}"
      run_case "${row}" "${run}" "${alloc}" "${args}"
    done
  done
}

common="--runs 1 --threads ${THREADS} --iters ${ITERS} --remote-pct 0"
run_row "small_local_i0_touch" "${common} --min-size 16 --max-size 2048 --interleaved 0 --touch 1"
run_row "small_local_i1_touch" "${common} --min-size 16 --max-size 2048 --interleaved 1 --touch 1 --live-window ${LIVE_WINDOW}"
run_row "main_local_i0_touch" "${common} --min-size 16 --max-size 32768 --interleaved 0 --touch 1"
run_row "main_local_i1_touch" "${common} --min-size 16 --max-size 32768 --interleaved 1 --touch 1 --live-window ${LIVE_WINDOW}"
run_row "medium_local_i0_touch" "${common} --min-size 4097 --max-size 65536 --interleaved 0 --touch 1"
run_row "medium_local_i1_touch" "${common} --min-size 4097 --max-size 65536 --interleaved 1 --touch 1 --live-window ${LIVE_WINDOW}"

for spec in "8k 8192" "16k 16384" "32k 32768" "64k 65536"; do
  set -- ${spec}
  name="$1"; size="$2"
  run_row "medium_${name}_local_touch" "${common} --min-size ${size} --max-size ${size} --interleaved 0 --touch 1"
  run_row "medium_${name}_local_notouch" "${common} --min-size ${size} --max-size ${size} --interleaved 0 --touch 0"
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv, statistics, sys
from collections import defaultdict
src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[(row["row"], row["allocator"])].append(row)
def med(items, key):
    vals = [float(x[key]) for x in items if x.get(key)]
    return statistics.median(vals) if vals else 0.0
def medi(items, key):
    vals = [int(float(x[key])) for x in items if x.get(key)]
    return int(statistics.median(vals)) if vals else 0
with open(dst, "w", encoding="utf-8") as f:
    f.write("# HZ8 Main/Medium Local Attribution\n\n")
    f.write("Behavior: attribution only; allocator behavior unchanged.\n\n")
    f.write("| Row | Allocator | median ops/s | steady ops/s | post RSS | peak RSS | minor faults | work ms | tail ms |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|\n")
    for key in sorted(groups):
        items = groups[key]
        f.write(f"| {key[0]} | {key[1]} | {med(items,'throughput'):.3f} | {med(items,'steady'):.3f} | {medi(items,'post_rss')} | {medi(items,'peak_rss')} | {medi(items,'minor_faults')} | {med(items,'work_ms'):.3f} | {med(items,'tail_ms'):.3f} |\n")
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] attribution logs saved to ${OUTDIR}"
echo "[DONE] summary: ${OUTDIR}/summary.md"
