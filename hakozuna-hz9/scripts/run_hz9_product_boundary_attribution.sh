#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_product_boundary_attribution}"
RUNS="${RUNS:-3}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-50000}"
ROWS="${ROWS:-medium_local0,main_local0,medium_interleaved_r50,main_interleaved_r90}"
RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"

mkdir -p "${OUTDIR}"

row_args() {
  case "$1" in
    medium_local0) printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0" ;;
    main_local0) printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0 --live-window 0" ;;
    medium_interleaved_r50) printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 --live-window 0" ;;
    main_interleaved_r90) printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 0" ;;
    *) echo "[ERR] unknown row $1" >&2; exit 3 ;;
  esac
}

make -C "${ROOT}" bench-hz9baseline-matrix-api bench-hz9product-matrix-api >/dev/null

PRELOAD_OUTDIR="${OUTDIR}/preload"
RUNS="${RUNS}" THREADS="${THREADS}" ITERS="${ITERS}" ROWS="${ROWS}" \
  ALLOCATORS=hz8_ref,hz9_product OUTDIR="${PRELOAD_OUTDIR}" \
  "${ROOT}/scripts/run_hz9_product_entry_public_matrix.sh" "${STAMP}_preload" >/dev/null

csv="${OUTDIR}/api_samples.csv"
printf 'row,allocator,run,throughput,post_rss,peak_rss,minor_faults,work_ms,tail_ms,log\n' > "${csv}"
IFS=',' read -r -a row_list <<< "${ROWS}"

run_api_one() {
  local row="$1" alloc="$2" run="$3" bin="$4"
  local log="${OUTDIR}/${row}_run${run}_${alloc}.log"
  local args
  args="$(row_args "${row}")"
  # shellcheck disable=SC2086
  timeout "${RUN_TIMEOUT}" "${bin}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} > "${log}" 2>&1
  awk -v row="${row}" -v alloc="${alloc}" -v run="${run}" -v logfile="${log}" '
    /^run=/ {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "ops/s") throughput = a[2]
        if (a[1] == "post_rss") post = a[2]
        if (a[1] == "peak_rss") peak = a[2]
        if (a[1] == "minor_faults") faults = a[2]
        if (a[1] == "work_ms") work = a[2]
        if (a[1] == "tail_ms") tail = a[2]
      }
    }
    END { printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", row, alloc, run, throughput, post, peak, faults, work, tail, logfile }
  ' "${log}" >> "${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  for row in "${row_list[@]}"; do
    run_api_one "${row}" hz8_api "${run}" "${ROOT}/h8_bench_matrix_api_baseline"
    run_api_one "${row}" hz9_api "${run}" "${ROOT}/h8_bench_matrix_api_hz9product"
  done
done

python3 - "${csv}" "${PRELOAD_OUTDIR}/samples.csv" "${OUTDIR}/summary.md" <<'PY'
import csv, statistics, sys

api_csv, preload_csv, out = sys.argv[1:]

def load(path):
    rows = {}
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.setdefault((r["row"], r["allocator"]), []).append(float(r["throughput"]))
    return {k: statistics.median(v) for k, v in rows.items()}

api = load(api_csv)
pre = load(preload_csv)
rows = sorted({k[0] for k in api} | {k[0] for k in pre})
with open(out, "w") as f:
    f.write("# HZ9 Product Boundary Attribution\n\n")
    f.write("| row | hz8_api | hz9_api | api ratio | hz8_preload | hz9_preload | preload ratio |\n")
    f.write("|---|---:|---:|---:|---:|---:|---:|\n")
    for row in rows:
        h8a = api.get((row, "hz8_api"), 0.0)
        h9a = api.get((row, "hz9_api"), 0.0)
        h8p = pre.get((row, "hz8_ref"), 0.0)
        h9p = pre.get((row, "hz9_product"), 0.0)
        ar = h9a / h8a if h8a else 0.0
        pr = h9p / h8p if h8p else 0.0
        f.write(f"| {row} | {h8a:.3f} | {h9a:.3f} | {ar:.3f} | {h8p:.3f} | {h9p:.3f} | {pr:.3f} |\n")
PY

echo "${OUTDIR}"
