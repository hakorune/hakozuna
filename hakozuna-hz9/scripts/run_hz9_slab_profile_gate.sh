#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_slab_profile_gate}"
RUNS="${RUNS:-10}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-80000}"
ROWS="${ROWS:-fixed64_local0,medium_r50,main_r90}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-release bench-release-hz9mediumslabpage >/dev/null

row_args() {
  case "$1" in
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
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
    slab4) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage" ;;
    *)
      echo "unknown variant: $1" >&2
      return 1
      ;;
  esac
}

IFS=',' read -r -a row_list <<< "${ROWS}"
csv="${OUTDIR}/samples.csv"
printf 'row,variant,run,throughput,post_rss,peak_rss,minor_faults,route_valid,free_valid,remote_claim,registered_pages,registered_bytes,log\n' >"${csv}"

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
        if (a[1] == "registered_bytes") bytes = a[2]
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, variant, run, ops, post, peak, faults, route + 0, free + 0,
        remote + 0, pages + 0, bytes + 0, logfile
    }
  ' "${log}" >>"${csv}"
}

for run in $(seq 1 "${RUNS}"); do
  for row in "${row_list[@]}"; do
    run_one "${row}" baseline "${run}"
    run_one "${row}" slab4 "${run}"
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
data = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in data:
    groups[(row["row"], row["variant"])].append(row)

def vals(rows, key):
    return [float(r[key]) for r in rows if r[key] != ""]

def med(rows, key):
    xs = vals(rows, key)
    return statistics.median(xs) if xs else 0.0

def q(rows, key, idx):
    xs = sorted(vals(rows, key))
    if not xs:
        return 0.0
    if len(xs) == 1:
        return xs[0]
    return statistics.quantiles(xs, n=4, method="inclusive")[idx]

row_order = []
for row in data:
    if row["row"] not in row_order:
        row_order.append(row["row"])

lines = [
    "# HZ9 Slab Profile Gate",
    "",
    "Release builds; SlabPage is profile/evidence only unless all no-regression gates pass.",
    "",
    "| row | variant | ops median | ratio | p25 | p75 | post RSS | peak RSS |",
    "|---|---|---:|---:|---:|---:|---:|---:|",
]
for row in row_order:
    base = groups.get((row, "baseline"), [])
    base_ops = med(base, "throughput")
    for variant in ("baseline", "slab4"):
        rows = groups.get((row, variant), [])
        if not rows:
            continue
        ops = med(rows, "throughput")
        ratio = ops / base_ops if base_ops else 0.0
        lines.append(
            f"| {row} | {variant} | {ops:.3f} | {ratio:.3f} | "
            f"{q(rows, 'throughput', 0):.3f} | {q(rows, 'throughput', 2):.3f} | "
            f"{med(rows, 'post_rss'):.0f} | {med(rows, 'peak_rss'):.0f} |"
        )

open(dst, "w").write("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-slab-profile] logs=${OUTDIR}"
