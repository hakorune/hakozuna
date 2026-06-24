#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

ALLOCATORS="${ALLOCATORS:-hz8,hz3,hz4,mimalloc,tcmalloc,system}"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
PHASE_THREADS="${PHASE_THREADS:-2}"
PHASE_ITERS="${PHASE_ITERS:-4000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/bench_results/hz8_same_run_matrix_$(date -u +%Y%m%dT%H%M%SZ)}"
BUILD="${BUILD:-1}"
BENCH_BIN="${BENCH_BIN:-${ROOT_DIR}/bench/out/bench_matrix_malloc}"

usage() {
  cat <<'EOF'
Usage:
  bench/run_hz8_same_run_matrix.sh [options]

Options:
  --allocators LIST   comma-separated allocators
  --runs N            fresh process samples per row/allocator
  --threads N         worker threads
  --iters N           iterations per thread
  --phase-threads N   medium phase stress threads
  --phase-iters N     medium phase stress iterations per thread
  --outdir DIR        output directory
  --skip-build        do not build HZ8 preload or matrix harness
  --help              show this message

Rows:
  guard_local0                 16..2048, remote=0
  small_interleaved_remote90   16..4096, remote=90, interleaved=1
  small_phase_remote90         16..4096, remote=90, interleaved=0
  main_local0                  16..32768, remote=0, interleaved=1
  main_interleaved_remote50    16..32768, remote=50, interleaved=1
  main_interleaved_remote90    16..32768, remote=90, interleaved=1
  medium_local0                4097..65536, remote=0, interleaved=1
  medium_interleaved_remote50  4097..65536, remote=50, interleaved=1
  medium_phase_remote90        4097..65536, remote=90, interleaved=0
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
    --phase-threads)
      PHASE_THREADS="$2"; shift 2 ;;
    --phase-iters)
      PHASE_ITERS="$2"; shift 2 ;;
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

if [[ ! -x "${BENCH_BIN}" ]]; then
  echo "[ERR] missing matrix harness: ${BENCH_BIN}" >&2
  exit 2
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
[MATRIX] ts=$(date -u +%Y%m%dT%H%M%SZ)
[MATRIX] root=${ROOT_DIR}
[MATRIX] bench_bin=${BENCH_BIN}
[MATRIX] allocators=${ALLOCATORS}
[MATRIX] runs=${RUNS}
[MATRIX] threads=${THREADS}
[MATRIX] iters=${ITERS}
[MATRIX] phase_threads=${PHASE_THREADS}
[MATRIX] phase_iters=${PHASE_ITERS}
[MATRIX] allocator_behavior_sha=f916c803
[MATRIX] freeze_record_sha=f916c803
EOF

for alloc in "${allocator_list[@]}"; do
  if [[ "${alloc}" == "system" ]]; then
    echo "[MATRIX] ${alloc}=system" | tee -a "${OUTDIR}/README.log"
  else
    echo "[MATRIX] ${alloc}=${allocator_libs[$alloc]}" | tee -a "${OUTDIR}/README.log"
  fi
done

csv="${OUTDIR}/samples.csv"
printf 'row,run,allocator,lib,throughput,steady,post_rss,peak_rss,minor_faults,work_ms,tail_ms,remote_live,rounded_live,log\n' > "${csv}"

run_case() {
  local row="$1"
  local run="$2"
  local alloc="$3"
  local args="$4"
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
    /^throughput / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) {
        split($i, a, "="); throughput = a[2]
      }
    }
    /^steady_work / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^throughput_median=/) {
        split($i, a, "="); steady = a[2]
      }
    }
    /^post_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) {
        split($i, a, "="); post = a[2]
      }
    }
    /^peak_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) {
        split($i, a, "="); peak = a[2]
      }
    }
    /^page_faults / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^minor_median=/) {
        split($i, a, "="); faults = a[2]
      }
    }
    /^(phase_ms|interleaved_phase_ms) / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^(work_median|alloc_median)=/) {
          split($i, a, "="); work = a[2]
        } else if ($i ~ /^(tail_median|remote_median)=/) {
          split($i, a, "="); tail = a[2]
        }
      }
    }
    /^phase_live / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^remote_live_median=/) {
        split($i, a, "="); live = a[2]
      }
    }
    /^run_phase=/ {
      for (i = 1; i <= NF; ++i) if ($i ~ /^rounded_live_bytes=/) {
        split($i, a, "="); rounded = a[2]
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, run, alloc, lib, throughput, steady, post, peak, faults,
        work, tail, live, rounded, logfile
    }
  ' "${log}" >> "${csv}"
}

run_row() {
  local row="$1"
  local args="$2"
  local count="${#allocator_list[@]}"
  for run in $(seq 1 "${RUNS}"); do
    local offset=$(( (run - 1) % count ))
    for idx in $(seq 0 $((count - 1))); do
      local alloc="${allocator_list[$(((idx + offset) % count))]}"
      echo "[RUN] row=${row} run=${run} alloc=${alloc}"
      run_case "${row}" "${run}" "${alloc}" "${args}"
    done
  done
}

common="--runs 1 --threads ${THREADS} --iters ${ITERS}"
medium_phase_common="--runs 1 --threads ${PHASE_THREADS} --iters ${PHASE_ITERS}"
run_row "guard_local0" \
  "${common} --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0"
run_row "small_interleaved_remote90" \
  "${common} --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window ${LIVE_WINDOW}"
run_row "small_phase_remote90" \
  "${common} --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 0"
run_row "main_local0" \
  "${common} --min-size 16 --max-size 32768 --remote-pct 0 --interleaved 1 --live-window ${LIVE_WINDOW}"
run_row "main_interleaved_remote50" \
  "${common} --min-size 16 --max-size 32768 --remote-pct 50 --interleaved 1 --live-window ${LIVE_WINDOW}"
run_row "main_interleaved_remote90" \
  "${common} --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window ${LIVE_WINDOW}"
run_row "medium_local0" \
  "${common} --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 1 --live-window ${LIVE_WINDOW}"
run_row "medium_interleaved_remote50" \
  "${common} --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 --live-window ${LIVE_WINDOW}"
run_row "medium_phase_remote90" \
  "${medium_phase_common} --min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0"

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[(row["row"], row["allocator"])].append(row)

def med(items, key):
    vals = [float(x[key]) for x in items if x.get(key)]
    return statistics.median(vals) if vals else 0.0

def med_int(items, key):
    vals = [int(float(x[key])) for x in items if x.get(key)]
    return int(statistics.median(vals)) if vals else 0

with open(dst, "w", encoding="utf-8") as f:
    f.write("# Same Run Allocator Matrix\n\n")
    f.write("Behavior: HZ8 frozen for matrix; allocator code must not change.\n\n")
    f.write("| Row | Allocator | median ops/s | steady ops/s | post RSS | peak RSS | minor faults | work ms | tail ms |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|\n")
    for key in sorted(groups):
        items = groups[key]
        f.write(
            f"| {key[0]} | {key[1]} | {med(items,'throughput'):.3f} | "
            f"{med(items,'steady'):.3f} | {med_int(items,'post_rss')} | "
            f"{med_int(items,'peak_rss')} | {med_int(items,'minor_faults')} | "
            f"{med(items,'work_ms'):.3f} | {med(items,'tail_ms'):.3f} |\n"
        )
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "[DONE] matrix logs saved to ${OUTDIR}"
echo "[DONE] summary: ${OUTDIR}/summary.md"
