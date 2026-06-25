#!/usr/bin/env bash
set -euo pipefail

HZ8_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "${HZ8_ROOT}/.." && pwd)"
OUTDIR="${OUTDIR:-${HZ8_ROOT}/bench_results/medium_retention_closeout_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-30}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-100000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
BENCH_BIN="${BENCH_BIN:-${HZ8_ROOT}/h8_bench_release}"
MATRIX_BIN="${ROOT_DIR}/bench/out/bench_matrix_malloc"
PRELOAD_SO="${PRELOAD_SO:-${HZ8_ROOT}/libhakozuna_hz8_preload.so}"

mkdir -p "${OUTDIR}" "$(dirname "${MATRIX_BIN}")"

if [[ "${H8_SKIP_BUILD:-0}" != "1" ]]; then
  make -C "${HZ8_ROOT}" bench-release preload >/dev/null
fi
"${CC:-gcc}" -O3 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -pthread -o "${MATRIX_BIN}" "${ROOT_DIR}/bench/bench_matrix_malloc.c"

csv="${OUTDIR}/samples.csv"
printf 'mode,run,throughput,steady,post_rss,peak_rss,minor_faults,work_ms,tail_ms,wall_ms,log\n' > "${csv}"

run_one() {
  local mode="$1"
  local run="$2"
  local log="${OUTDIR}/${mode}_medium_r50_run${run}.log"
  local start_ns end_ns wall_ms

  start_ns="$(date +%s%N)"
  if [[ "${mode}" == "direct" ]]; then
    "${BENCH_BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" \
      --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 \
      --live-window "${LIVE_WINDOW}" > "${log}" 2>&1
  else
    LD_PRELOAD="${PRELOAD_SO}" \
      "${MATRIX_BIN}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" \
      --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 \
      --live-window "${LIVE_WINDOW}" > "${log}" 2>&1
  fi
  end_ns="$(date +%s%N)"
  wall_ms="$(awk -v s="${start_ns}" -v e="${end_ns}" \
    'BEGIN { printf "%.3f", (e - s) / 1000000.0 }')"

  awk -v mode="${mode}" -v run="${run}" -v wall="${wall_ms}" \
      -v logfile="${log}" '
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
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        mode, run, throughput, steady, post, peak, faults, work, tail, wall,
        logfile
    }
  ' "${log}" >> "${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  run_one direct "${run}"
  run_one preload "${run}"
done

python3 - "${csv}" "${OUTDIR}/README.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[row["mode"]].append(row)

def vals(items, key):
    return [float(x[key]) for x in items if x.get(key)]

def med(items, key):
    v = vals(items, key)
    return statistics.median(v) if v else 0.0

def p25(items, key):
    v = sorted(vals(items, key))
    return v[int((len(v) - 1) * 0.25 + 0.5)] if v else 0.0

def p95(items, key):
    v = sorted(vals(items, key))
    return v[int((len(v) - 1) * 0.95 + 0.5)] if v else 0.0

def mn(items, key):
    v = vals(items, key)
    return min(v) if v else 0.0

def mx(items, key):
    v = vals(items, key)
    return max(v) if v else 0.0

with open(dst, "w", encoding="utf-8") as f:
    f.write("# Medium Retention Closeout\n\n")
    f.write("Row: `medium_interleaved_remote50`, 4097..65536, fresh process samples.\n\n")
    f.write("Outlier threshold: `minor_faults > max(median_faults * 8, 100000)`.\n\n")
    f.write("| Mode | runs | median ops/s | p25 ops/s | min ops/s | median faults | p95 faults | max faults | outliers | median peak RSS | median post RSS |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n")
    for mode in sorted(groups):
        items = groups[mode]
        fault_median = med(items, "minor_faults")
        threshold = max(fault_median * 8.0, 100000.0)
        outliers = sum(1 for x in items if float(x["minor_faults"]) > threshold)
        f.write(
            f"| {mode} | {len(items)} | {med(items, 'throughput'):.3f} | "
            f"{p25(items, 'throughput'):.3f} | {mn(items, 'throughput'):.3f} | "
            f"{fault_median:.0f} | {p95(items, 'minor_faults'):.0f} | "
            f"{mx(items, 'minor_faults'):.0f} | {outliers} | "
            f"{med(items, 'peak_rss'):.0f} | {med(items, 'post_rss'):.0f} |\n"
        )
    f.write("\nRaw samples: `samples.csv`\n")
PY

echo "${OUTDIR}"
