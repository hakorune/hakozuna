#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${1:-$(date -u +%Y%m%dT%H%M%SZ)}_hz9_product_entry_matrix_api}"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-50000}"
LIVE_WINDOW="${LIVE_WINDOW:-4096}"
RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
BASE_BIN="${ROOT}/h8_bench_matrix_api_baseline"
CAND_BIN="${ROOT}/h8_bench_matrix_api_hz9product"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9baseline-matrix-api bench-hz9product-matrix-api >/dev/null

cat >&2 <<EOF
[hz9-product-entry-matrix-api-probe]
  OUTDIR=${OUTDIR}
  RUNS=${RUNS}
  THREADS=${THREADS}
  ITERS=${ITERS}
  ROWS=${ROWS:-default}
  note=Direct h8_malloc/h8_free matrix payload path; no LD_PRELOAD.
EOF

row_args() {
  case "$1" in
    guard_local0)
      printf '%s\n' "--min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    medium_interleaved_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 --live-window 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    main_interleaved_r50)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 50 --interleaved 1 --live-window 0"
      ;;
    main_interleaved_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    medium_phase_r90)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 90 --interleaved 0 --live-window ${LIVE_WINDOW}"
      ;;
    *)
      echo "[ERR] unknown row $1" >&2
      exit 3
      ;;
  esac
}

if [[ -n "${ROWS:-}" ]]; then
  IFS=',' read -r -a rows <<< "${ROWS}"
else
  rows=(
    guard_local0
    small_interleaved_remote90
    fixed64_local0
    medium_local0
    medium_interleaved_r50
    main_local0
    main_interleaved_r50
    main_interleaved_r90
  )
fi

csv="${OUTDIR}/samples.csv"
printf 'row,allocator,run,throughput,post_rss,peak_rss,minor_faults,work_ms,tail_ms,log\n' > "${csv}"

run_one() {
  local row="$1"
  local alloc="$2"
  local run="$3"
  local bin="$4"
  local log="${OUTDIR}/${row}_run${run}_${alloc}.log"
  local args
  args="$(row_args "${row}")"
  # shellcheck disable=SC2086
  timeout "${RUN_TIMEOUT}" "${bin}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} > "${log}" 2>&1
  awk -v row="${row}" -v alloc="${alloc}" -v run="${run}" -v logfile="${log}" '
    /^throughput / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); throughput = a[2] }
    }
    /^post_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); post = a[2] }
    }
    /^peak_rss / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); peak = a[2] }
    }
    /^page_faults / {
      for (i = 1; i <= NF; ++i) if ($i ~ /^minor_median=/) { split($i, a, "="); faults = a[2] }
    }
    /^phase_ms / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^work_median=/) { split($i, a, "="); work = a[2] }
        if ($i ~ /^tail_median=/) { split($i, a, "="); tail = a[2] }
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, alloc, run, throughput, post, peak, faults, work, tail, logfile
    }
  ' "${log}" >> "${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    if (( run % 2 == 1 )); then
      run_one "${row}" hz8_api "${run}" "${BASE_BIN}"
      run_one "${row}" hz9_product_api "${run}" "${CAND_BIN}"
    else
      run_one "${row}" hz9_product_api "${run}" "${CAND_BIN}"
      run_one "${row}" hz8_api "${run}" "${BASE_BIN}"
    fi
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[(row["row"], row["allocator"])].append(row)

def nums(items, key):
    return sorted(float(x[key]) for x in items if x.get(key))

def pick(items, key, pct):
    values = nums(items, key)
    if not values:
        return 0.0
    return values[int((len(values) - 1) * pct + 0.5)]

row_names = []
for row in rows:
    if row["row"] not in row_names:
        row_names.append(row["row"])

with open(dst, "w", encoding="utf-8") as f:
    f.write("# HZ9 ProductEntry Matrix API Probe\n\n")
    f.write("Direct `h8_malloc/h8_free` payload matrix path. No LD_PRELOAD.\n\n")
    f.write("| Row | hz8_api ops/s | hz9_product_api ops/s | ratio | hz8 RSS | hz9 RSS |\n")
    f.write("|---|---:|---:|---:|---:|---:|\n")
    for name in row_names:
        base = groups.get((name, "hz8_api"), [])
        cand = groups.get((name, "hz9_product_api"), [])
        bops = pick(base, "throughput", 0.5)
        cops = pick(cand, "throughput", 0.5)
        ratio = cops / bops if bops else 0.0
        f.write(
            f"| {name} | {bops:.3f} | {cops:.3f} | {ratio:.3f} | "
            f"{pick(base, 'post_rss', 0.5):.0f} | {pick(cand, 'post_rss', 0.5):.0f} |\n"
        )
PY

echo "${OUTDIR}"
