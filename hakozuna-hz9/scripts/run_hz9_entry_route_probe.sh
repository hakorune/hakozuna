#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_entry_route_probe}"
RUNS="${RUNS:-3}"
THREADS="${THREADS:-8}"
ITERS="${ITERS:-30000}"
ROWS="${ROWS:-guard_local0,small_interleaved_remote90,medium_local0,main_local0,medium_r50,main_r90}"
VARIANTS="${VARIANTS:-baseline,entry}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" \
  bench \
  bench-hz9mediumslabpage-adaptive-entry \
  bench-hz9mediumslabpage-adaptive-entry-hotmask >/dev/null

row_args() {
  case "$1" in
    guard_local0) printf '%s\n' "--min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --live-window 0" ;;
    small_interleaved_remote90) printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0" ;;
    medium_local0) printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0" ;;
    main_local0) printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0" ;;
    medium_r50) printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1" ;;
    main_r90) printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1" ;;
    *) echo "unknown row: $1" >&2; return 1 ;;
  esac
}

variant_bin() {
  case "$1" in
    baseline) printf '%s\n' "${ROOT}/h8_bench" ;;
    entry) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_adaptive_entry" ;;
    hotmask) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_adaptive_entry_hotmask" ;;
    *) echo "unknown variant: $1" >&2; return 1 ;;
  esac
}

csv="${OUTDIR}/samples.csv"
printf 'row,variant,run,throughput,blocked,enabled,route_valid,free_valid,remote_claim,registered_pages,log\n' >"${csv}"

run_one() {
  local row="$1" variant="$2" run="$3" bin args log
  bin="$(variant_bin "${variant}")"
  args="$(row_args "${row}")"
  log="${OUTDIR}/${row}_${variant}_run${run}.log"
  # shellcheck disable=SC2086
  "${bin}" --runs 1 --threads "${THREADS}" --iters "${ITERS}" ${args} >"${log}" 2>&1
  awk -v row="${row}" -v variant="${variant}" -v run="${run}" -v logfile="${log}" '
    /^run=/ { split($2, a, "="); ops = a[2] }
    /^h9_slab_route / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "route_valid") route = a[2]
        if (a[1] == "free_valid") free = a[2]
      }
    }
    /^h9_slab_pending / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "remote_claim") remote = a[2]
      }
    }
    /^h9_slab_rss / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "registered_pages") pages = a[2]
      }
    }
    /^h9_slab_adaptive / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "alloc_blocked") blocked = a[2]
        if (a[1] == "alloc_enabled") enabled = a[2]
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, variant, run, ops, blocked + 0, enabled + 0, route + 0,
        free + 0, remote + 0, pages + 0, logfile
    }
  ' "${log}" >>"${csv}"
}

IFS=',' read -r -a row_list <<<"${ROWS}"
IFS=',' read -r -a variant_list <<<"${VARIANTS}"
for run in $(seq 1 "${RUNS}"); do
  for row in "${row_list[@]}"; do
    for variant in "${variant_list[@]}"; do
      run_one "${row}" "${variant}" "${run}"
    done
  done
done

python3 - "${csv}" "${OUTDIR}/README.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
data = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
row_order = []
for item in data:
    groups[(item["row"], item["variant"])].append(item)
    if item["row"] not in row_order:
        row_order.append(item["row"])

def med(rows, key):
    vals = [float(r[key]) for r in rows if r[key] != ""]
    return statistics.median(vals) if vals else 0.0

lines = [
    "# HZ9 Entry Route Probe",
    "",
    "Debug/attribution builds. Use for mechanism, not promotion timing.",
    "",
    "| row | variant | median ops | ratio | blocked | enabled | route_valid | free_valid | remote_claim | pages |",
    "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|",
]
for row in row_order:
    base = groups[(row, "baseline")]
    base_ops = med(base, "throughput")
    variants = [v for r, v in groups if r == row]
    variants.sort(key=lambda v: (v != "baseline", v))
    for variant in variants:
        rows = groups[(row, variant)]
        ops = med(rows, "throughput")
        ratio = ops / base_ops if base_ops else 0.0
        lines.append(
            f"| {row} | {variant} | {ops:.3f} | {ratio:.3f} | "
            f"{med(rows, 'blocked'):.0f} | {med(rows, 'enabled'):.0f} | "
            f"{med(rows, 'route_valid'):.0f} | {med(rows, 'free_valid'):.0f} | "
            f"{med(rows, 'remote_claim'):.0f} | {med(rows, 'registered_pages'):.0f} |"
        )
lines += [
    "",
    "Read:",
    "",
    "```text",
    "alloc_blocked without pages means the entry path is paying adaptive checks but still falling back.",
    "route/free counters show actual slab-owned traffic.",
    "```",
]
open(dst, "w").write("\n".join(lines) + "\n")
print("\n".join(lines))
PY
