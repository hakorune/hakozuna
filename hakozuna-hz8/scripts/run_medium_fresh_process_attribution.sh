#!/usr/bin/env bash
set -euo pipefail

HZ8_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "${HZ8_ROOT}/.." && pwd)"
OUTDIR="${OUTDIR:-${HZ8_ROOT}/bench_results/medium_fresh_process_attr_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
BENCH_BIN="${HZ8_ROOT}/h8_bench_release"
MATRIX_BIN="${ROOT_DIR}/bench/out/bench_matrix_malloc"
PRELOAD_SO="${HZ8_ROOT}/libhakozuna_hz8_preload.so"

mkdir -p "${OUTDIR}" "$(dirname "${MATRIX_BIN}")"

make -C "${HZ8_ROOT}" bench-release preload >/dev/null
"${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -pthread -o "${MATRIX_BIN}" "${ROOT_DIR}/bench/bench_matrix_malloc.c"

csv="${OUTDIR}/samples.csv"
printf 'mode,row,run,throughput,steady,post_rss,peak_rss,minor_faults,work_ms,tail_ms,wall_ms,log\n' > "${csv}"

run_one() {
  local mode="$1"
  local row="$2"
  local run="$3"
  shift 3
  local log="${OUTDIR}/${mode}_${row}_run${run}.log"
  local start_ns end_ns wall_ms
  start_ns="$(date +%s%N)"
  if [[ "${mode}" == "direct" ]]; then
    "${BENCH_BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" "$@" > "${log}" 2>&1
  else
    LD_PRELOAD="${PRELOAD_SO}" \
      "${MATRIX_BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" "$@" > "${log}" 2>&1
  fi
  end_ns="$(date +%s%N)"
  wall_ms="$(awk -v s="${start_ns}" -v e="${end_ns}" 'BEGIN { printf "%.3f", (e - s) / 1000000.0 }')"
  awk -v mode="${mode}" -v row="${row}" -v run="${run}" \
      -v wall="${wall_ms}" -v logfile="${log}" '
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
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        mode, row, run, throughput, steady, post, peak, faults, work, tail,
        wall, logfile
    }
  ' "${log}" >> "${csv}"
}

run_row() {
  local row="$1"
  shift
  for run in $(seq 1 "${RUNS}"); do
    run_one direct "${row}" "${run}" "$@"
    run_one preload "${row}" "${run}" "$@"
  done
}

run_row medium_interleaved_remote50 \
  --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 \
  --live-window "${LIVE_WINDOW}"

run_row medium_local0 \
  --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 1 \
  --live-window "${LIVE_WINDOW}"

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[(row["row"], row["mode"])].append(row)

def med(items, key):
    vals = [float(x[key]) for x in items if x.get(key)]
    return statistics.median(vals) if vals else 0.0

def p25(items, key):
    vals = sorted(float(x[key]) for x in items if x.get(key))
    return vals[int((len(vals) - 1) * 0.25 + 0.5)] if vals else 0.0

def mn(items, key):
    vals = [float(x[key]) for x in items if x.get(key)]
    return min(vals) if vals else 0.0

with open(dst, "w", encoding="utf-8") as f:
    f.write("# Medium Fresh Process Attribution\n\n")
    f.write("| Row | Mode | median ops/s | p25 ops/s | min ops/s | steady ops/s | peak RSS | minor faults | wall ms |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|---:|---:|\n")
    for key in sorted(groups):
        items = groups[key]
        f.write(
            f"| {key[0]} | {key[1]} | {med(items, 'throughput'):.3f} | "
            f"{p25(items, 'throughput'):.3f} | {mn(items, 'throughput'):.3f} | "
            f"{med(items, 'steady'):.3f} | {med(items, 'peak_rss'):.0f} | "
            f"{med(items, 'minor_faults'):.0f} | {med(items, 'wall_ms'):.3f} |\n"
        )
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "${OUTDIR}"
