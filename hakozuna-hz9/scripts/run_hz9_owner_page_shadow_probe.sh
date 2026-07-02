#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_owner_page_shadow_probe}"
RUNS="${RUNS:-3}"
THREADS="${THREADS:-8}"
ITERS="${ITERS:-30000}"
RUN_TIMEOUT="${RUN_TIMEOUT:-120s}"
ROWS="${ROWS:-medium_local0,main_local0,medium_r50,main_r90,fixed64_local0}"
BIN="${ROOT}/h8_bench_hz9ownerpagepool_shadow"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-hz9ownerpagepool-shadow >/dev/null

row_args() {
  case "$1" in
    fixed24_local0)
      printf '%s\n' "--min-size 24576 --max-size 24576 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    fixed48_local0)
      printf '%s\n' "--min-size 49152 --max-size 49152 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1 --live-window 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    main_r90)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    small_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    *)
      echo "[ERR] unknown row: $1" >&2
      return 1
      ;;
  esac
}

csv="${OUTDIR}/samples.csv"
printf 'row,run,throughput,post_rss,peak_rss,minor_faults,alloc,active_owner,active_nonfull,local_free,remote_free,pure_local,local_ratio,remote_ratio,alloc_class,local_class,remote_class,log\n' >"${csv}"

run_one() {
  local row="$1"
  local run="$2"
  local args log
  args="$(row_args "${row}")"
  log="${OUTDIR}/${row}_run${run}.log"
  # shellcheck disable=SC2086
  timeout "${RUN_TIMEOUT}" "${BIN}" --runs 1 --threads "${THREADS}" \
    --iters "${ITERS}" ${args} >"${log}" 2>&1
  awk -v row="${row}" -v run="${run}" -v logfile="${log}" '
    /^throughput / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); throughput = a[2] }
      }
    }
    /^post_rss / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); post = a[2] }
      }
    }
    /^peak_rss / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^median=/) { split($i, a, "="); peak = a[2] }
      }
    }
    /^page_faults / {
      for (i = 1; i <= NF; ++i) {
        if ($i ~ /^minor_median=/) { split($i, a, "="); faults = a[2] }
      }
    }
    /^h9_owner_page_shadow / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "alloc") alloc = a[2]
        if (a[1] == "active_owner") active_owner = a[2]
        if (a[1] == "active_nonfull") active_nonfull = a[2]
        if (a[1] == "local_free") local_free = a[2]
        if (a[1] == "remote_free") remote_free = a[2]
        if (a[1] == "pure_local") pure_local = a[2]
        if (a[1] == "local_ratio") local_ratio = a[2]
        if (a[1] == "remote_ratio") remote_ratio = a[2]
      }
    }
    /^h9_owner_page_shadow_class / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "alloc") alloc_class = a[2]
        if (a[1] == "local_free") local_class = a[2]
        if (a[1] == "remote_free") remote_class = a[2]
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,\"%s\",\"%s\",\"%s\",%s\n",
        row, run, throughput, post, peak, faults, alloc, active_owner,
        active_nonfull, local_free, remote_free, pure_local, local_ratio,
        remote_ratio, alloc_class, local_class, remote_class, logfile
    }
  ' "${log}" >>"${csv}"
}

IFS=',' read -r -a row_list <<<"${ROWS}"
for row in "${row_list[@]}"; do
  for run in $(seq 1 "${RUNS}"); do
    echo "[hz9-owner-page-shadow] row=${row} run=${run}/${RUNS}"
    run_one "${row}" "${run}"
  done
done

python3 - "${csv}" "${OUTDIR}/README.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, out = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in rows:
    groups[row["row"]].append(row)

def med(items, key):
    vals = [float(x[key]) for x in items if x.get(key)]
    return statistics.median(vals) if vals else 0.0

lines = [
    "# HZ9 Owner Page Shadow Probe",
    "",
    "No-behavior debug counter run for `HZ9OwnerLocalPagePoolShadow-L0`.",
    "",
    "| row | ops median | alloc | local free | remote free | pure local | active owner | active nonfull | local ratio | remote ratio | post RSS |",
    "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
]
for name in groups:
    items = groups[name]
    lines.append(
        f"| `{name}` | {med(items, 'throughput'):.3f} | "
        f"{med(items, 'alloc'):.0f} | {med(items, 'local_free'):.0f} | "
        f"{med(items, 'remote_free'):.0f} | {med(items, 'pure_local'):.0f} | "
        f"{med(items, 'active_owner'):.0f} | "
        f"{med(items, 'active_nonfull'):.0f} | "
        f"{med(items, 'local_ratio'):.3f} | "
        f"{med(items, 'remote_ratio'):.3f} | "
        f"{med(items, 'post_rss'):.0f} |"
    )

lines += [
    "",
    "## Class Distribution",
    "",
    "| row | alloc class | local class | remote class |",
    "|---|---|---|---|",
]
for name in groups:
    item = groups[name][len(groups[name]) // 2]
    lines.append(
        f"| `{name}` | `{item['alloc_class']}` | "
        f"`{item['local_class']}` | `{item['remote_class']}` |"
    )

open(out, "w").write("\n".join(lines) + "\n")
PY

cat "${OUTDIR}/README.md"
echo "[hz9-owner-page-shadow] out=${OUTDIR}"
