#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_owner_page_perf_gate}"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-60000}"
ROWS="${ROWS:-guard_local0,small_interleaved_remote90,medium_local0,main_local0,medium_r50,main_r90}"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" \
  bench-release \
  bench-release-hz9ownerpagepool-purelocal-api \
  bench-hz9ownerpagepool-purelocal-api >/dev/null

row_args() {
  case "$1" in
    guard_local0)
      printf '%s\n' "--min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    fixed48_local0)
      printf '%s\n' "--min-size 49152 --max-size 49152 --remote-pct 0 --interleaved 0"
      ;;
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
      ;;
    main_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1"
      ;;
    *)
      echo "unknown row: $1" >&2
      return 1
      ;;
  esac
}

variant_bin() {
  case "$1" in
    baseline) printf '%s\n' "${ROOT}/h8_bench_release" ;;
    ownerpage)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9ownerpagepool_purelocal_api"
      ;;
    ownerpage_debug)
      printf '%s\n' "${ROOT}/h8_bench_hz9ownerpagepool_purelocal_api"
      ;;
    *)
      echo "unknown variant: $1" >&2
      return 1
      ;;
  esac
}

run_one() {
  local row="$1" variant="$2" run="$3" bin args log
  bin="$(variant_bin "${variant}")"
  args="$(row_args "${row}")"
  log="${OUTDIR}/${row}_${variant}_run${run}.log"
  # shellcheck disable=SC2086
  "${bin}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} \
    >"${log}" 2>&1
  awk -v row="${row}" -v variant="${variant}" -v run="${run}" \
      -v logfile="${log}" '
    /^run=/ { split($2, a, "="); ops = a[2] }
    /^post_rss / { for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); post = a[2] } }
    /^peak_rss / { for (i = 1; i <= NF; ++i) if ($i ~ /^median=/) { split($i, a, "="); peak = a[2] } }
    /^page_faults / { for (i = 1; i <= NF; ++i) if ($i ~ /^minor_median=/) { split($i, a, "="); faults = a[2] } }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, variant, run, ops, post, peak, faults, logfile
    }
  ' "${log}" >>"${OUTDIR}/samples.csv"
}

printf 'row,variant,run,throughput,post_rss,peak_rss,minor_faults,log\n' \
  >"${OUTDIR}/samples.csv"
IFS=',' read -r -a row_list <<< "${ROWS}"
for run in $(seq 1 "${RUNS}"); do
  for row in "${row_list[@]}"; do
    run_one "${row}" baseline "${run}"
    run_one "${row}" ownerpage "${run}"
  done
done

for row in "${row_list[@]}"; do
  run_one "${row}" ownerpage_debug 1
done

python3 - "${OUTDIR}/samples.csv" "${OUTDIR}/README.md" \
  "${RUNS}" "${THREADS}" "${ITERS}" "${ROWS}" <<'PY'
import csv
import pathlib
import re
import statistics
import sys
from collections import defaultdict

csv_path, out_path, runs, threads, iters, rows_arg = sys.argv[1:7]
data = list(csv.DictReader(open(csv_path, newline="")))
groups = defaultdict(list)
row_order = []
for item in data:
    groups[(item["row"], item["variant"])].append(item)
    if item["row"] not in row_order:
        row_order.append(item["row"])

def vals(rows, key):
    return [float(r[key]) for r in rows if r[key] != ""]

def med(rows, key):
    v = vals(rows, key)
    return statistics.median(v) if v else 0.0

def p25(rows, key):
    v = sorted(vals(rows, key))
    if not v:
        return 0.0
    if len(v) == 1:
        return v[0]
    return statistics.quantiles(v, n=4, method="inclusive")[0]

def parse_owner_counters(log):
    text = pathlib.Path(log).read_text(errors="replace")
    line = next((ln for ln in text.splitlines()
                 if ln.startswith("h9_owner_page_api ")), "")
    local = next((ln for ln in text.splitlines()
                  if ln.startswith("h9_owner_page_api_local ")), "")
    return line, local

lines = [
    "# HZ9 Owner Page Perf Gate",
    "",
    "```text",
    f"runs: {runs}",
    f"threads: {threads}",
    f"iters: {iters}",
    f"rows: {rows_arg}",
    "variants: baseline, ownerpage",
    "```",
    "",
    "| row | baseline median | ownerpage median | ratio | p25 ratio | peak RSS ratio |",
    "|---|---:|---:|---:|---:|---:|",
]
for row in row_order:
    base = groups.get((row, "baseline"), [])
    cand = groups.get((row, "ownerpage"), [])
    if not base or not cand:
        continue
    base_med = med(base, "throughput")
    base_p25 = p25(base, "throughput")
    cand_med = med(cand, "throughput")
    cand_p25 = p25(cand, "throughput")
    peak_ratio = med(cand, "peak_rss") / med(base, "peak_rss") if med(base, "peak_rss") else 0.0
    lines.append(
        f"| {row} | {base_med:.3f} | {cand_med:.3f} | "
        f"{(cand_med / base_med if base_med else 0.0):.3f} | "
        f"{(cand_p25 / base_p25 if base_p25 else 0.0):.3f} | "
        f"{peak_ratio:.3f} |"
    )

lines += ["", "## Debug Counter Sample", ""]
for row in row_order:
    dbg = groups.get((row, "ownerpage_debug"), [])
    if not dbg:
        continue
    api, local = parse_owner_counters(dbg[0]["log"])
    lines += [f"### {row}", "", "```text", api, local, "```", ""]

lines += [
    "## Read",
    "",
    "```text",
    "Proceed only if medium/main local win is material and small/remote rows do",
    "not regress beyond the HZ9 L0 tolerance. If local rows are flat or worse,",
    "global route/lock plus mmap-backed owner pages are likely too expensive.",
    "```",
]
pathlib.Path(out_path).write_text("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-owner-page-perf-gate] logs=${OUTDIR}"
