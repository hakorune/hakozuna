#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_local_mag_class_gate}"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-50000}"
RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
ROWS="${ROWS:-fixed24_local0,fixed32_local0,fixed48_local0,fixed64_local0,main_local0,medium_local0,medium_r50,small_remote90}"
VARIANTS="${VARIANTS:-hz8,hz9_min2,hz9_min4,hz9_min5,hz9_disabled}"

mkdir -p "${OUTDIR}"

make -C "${ROOT}" \
  bench-release \
  bench-release-hz9mediumlocalmag \
  bench-release-hz9mediumlocalmag-min2 \
  bench-release-hz9mediumlocalmag-min5 \
  bench-release-hz9mediumlocalmag-disabled >/dev/null

variant_bin() {
  case "$1" in
    hz8) printf '%s\n' "${ROOT}/h8_bench_release" ;;
    hz9_min2) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumlocalmag_min2" ;;
    hz9_min4) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumlocalmag" ;;
    hz9_min5) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumlocalmag_min5" ;;
    hz9_disabled) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumlocalmag_disabled" ;;
    hz9_unsafe_tlsonly) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumlocalmag_unsafe_tlsonly" ;;
    *) echo "[ERR] unknown variant $1" >&2; exit 2 ;;
  esac
}

row_args() {
  case "$1" in
    fixed24_local0)
      printf '%s\n' "--min-size 24576 --max-size 24576 --remote-pct 0 --interleaved 0"
      ;;
    fixed32_local0)
      printf '%s\n' "--min-size 32768 --max-size 32768 --remote-pct 0 --interleaved 0"
      ;;
    fixed48_local0)
      printf '%s\n' "--min-size 49152 --max-size 49152 --remote-pct 0 --interleaved 0"
      ;;
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
      ;;
    small_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1"
      ;;
    *) echo "[ERR] unknown row $1" >&2; exit 3 ;;
  esac
}

IFS=',' read -r -a row_list <<< "${ROWS}"
IFS=',' read -r -a variant_list <<< "${VARIANTS}"
csv="${OUTDIR}/samples.csv"
printf 'row,variant,run,throughput,post_rss,peak_rss,minor_faults,log\n' >"${csv}"

run_one() {
  local row="$1" variant="$2" run="$3" bin args log
  bin="$(variant_bin "${variant}")"
  args="$(row_args "${row}")"
  log="${OUTDIR}/${row}_${variant}_run${run}.log"
  # shellcheck disable=SC2086
  timeout "${RUN_TIMEOUT}" "${bin}" --runs 1 --threads "${THREADS}" \
    --iters "${ITERS}" ${args} --live-window 0 >"${log}" 2>&1
  awk -v row="${row}" -v variant="${variant}" -v run="${run}" \
      -v logfile="${log}" '
    /^run=/ { split($2, a, "="); ops = a[2] }
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
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, variant, run, ops, post, peak, faults, logfile
    }
  ' "${log}" >>"${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  for row in "${row_list[@]}"; do
    for variant in "${variant_list[@]}"; do
      run_one "${row}" "${variant}" "${run}"
    done
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
row_order, variant_order = [], []
for r in rows:
    groups[(r["row"], r["variant"])].append(r)
    if r["row"] not in row_order:
        row_order.append(r["row"])
    if r["variant"] not in variant_order:
        variant_order.append(r["variant"])

def nums(items, key):
    return [float(x[key]) for x in items if x.get(key)]

def median(items, key):
    values = nums(items, key)
    return statistics.median(values) if values else 0.0

def mib(v):
    return v / (1024.0 * 1024.0)

with open(dst, "w", encoding="utf-8") as f:
    f.write("# HZ9 Local Magazine Class Gate\n\n")
    f.write("Raw samples: `samples.csv`.\n\n")
    f.write("| Row | Variant | Median ops/s | Ratio vs hz8 | Peak RSS | Post RSS | n |\n")
    f.write("|---|---|---:|---:|---:|---:|---:|\n")
    for row in row_order:
        base = median(groups[(row, "hz8")], "throughput")
        for variant in variant_order:
            items = groups[(row, variant)]
            med = median(items, "throughput")
            ratio = med / base if base else 0.0
            peak = median(items, "peak_rss")
            post = median(items, "post_rss")
            f.write(
                f"| `{row}` | `{variant}` | {med:.3f} | {ratio:.3f} | "
                f"{mib(peak):.2f} MiB | {mib(post):.2f} MiB | {len(items)} |\n"
            )
print(dst)
PY

cat "${OUTDIR}/summary.md"
