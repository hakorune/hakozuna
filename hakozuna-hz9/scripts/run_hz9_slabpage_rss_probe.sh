#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_slabpage_rss_probe}"
RUNS="${RUNS:-3}"
THREADS="${THREADS:-8}"
ITERS="${ITERS:-30000}"
ROWS="${ROWS:-fixed64_local0,medium_r50,main_r90}"
VARIANTS="${VARIANTS:-baseline,slab4,slab6,slab8}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench bench-hz9mediumslabpage \
  bench-hz9mediumslabpage-classes \
  bench-hz9mediumslabpage-classes-min0 \
  bench-hz9mediumslabpage-classes-min0-midcap \
  bench-hz9mediumslabpage-classes-min0-entry \
  bench-hz9mediumslabpage-classes-min0-sidecar \
  bench-hz9mediumslabpage-classes-min0-sidecar2 \
  bench-hz9mediumslabpage-cap6 bench-hz9mediumslabpage-cap8 >/dev/null

row_args() {
  case "$1" in
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    guard_local0)
      printf '%s\n' "--min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    medium_r50)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
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
    baseline) printf '%s\n' "${ROOT}/h8_bench" ;;
    slab|slab4) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage" ;;
    slabclasses) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_classes" ;;
    slabclasses_min0) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_classes_min0" ;;
    slabclasses_min0_midcap)
      printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_classes_min0_midcap"
      ;;
    slabclasses_min0_entry)
      printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_classes_min0_entry"
      ;;
    slabclasses_min0_sidecar)
      printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_classes_min0_sidecar"
      ;;
    slabclasses_min0_sidecar2)
      printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_classes_min0_sidecar2"
      ;;
    slab6) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_cap6" ;;
    slab8) printf '%s\n' "${ROOT}/h8_bench_hz9mediumslabpage_cap8" ;;
    *)
      echo "unknown variant: $1" >&2
      return 1
      ;;
  esac
}

IFS=',' read -r -a row_list <<< "${ROWS}"
IFS=',' read -r -a variant_list <<< "${VARIANTS}"
csv="${OUTDIR}/samples.csv"
printf 'row,variant,run,throughput,post_rss,peak_rss,minor_faults,remote_claim,direct_slot,alloc_hit,free_first_hit,pending_retry_hit,try_active,try_partial,try_after_owner,try_new_page,pending_active,pending_partial,pending_after_owner,pending_new_page,queue_slot,purge_slot,registered_pages,registered_bytes,raw_reserved,thread_cap_fallback,find_active,find_hash,find_miss,find_no_pages,free_local,scan_steps,scan_miss,install0,install1,install2,install3,install4,install5,cap0,cap1,cap2,cap3,cap4,cap5,entry_malloc_medium,entry_malloc_fallback,entry_free_ready,entry_free_arena,entry_free_maybe_false,entry_free_slab_outer,entry_realloc_check,entry_realloc_slab,log\n' >"${csv}"

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
        if (a[1] == "alloc_hit") alloc_hit = a[2]
        if (a[1] == "free_first_hit") free_first = a[2]
        if (a[1] == "pending_retry_hit") retry_hit = a[2]
      }
    }
    /^h9_slab_rss / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "registered_pages") pages = a[2]
        if (a[1] == "registered_bytes") bytes = a[2]
        if (a[1] == "raw_reserved") raw = a[2]
        if (a[1] == "thread_cap_fallback") cap = a[2]
      }
    }
    /^h9_slab_thread_class / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "install") {
          gsub(/[\[\]]/, "", a[2])
          split(a[2], install, ",")
        }
        if (a[1] == "cap_fallback") {
          gsub(/[\[\]]/, "", a[2])
          split(a[2], cap_class, ",")
        }
      }
    }
    /^h9_slab_find / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "active_hit") find_active = a[2]
        if (a[1] == "hash_hit") find_hash = a[2]
        if (a[1] == "miss") find_miss = a[2]
        if (a[1] == "no_pages") find_no_pages = a[2]
      }
    }
    /^h9_slab_local / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "free_local") free_local = a[2]
        if (a[1] == "alloc_scan_steps") scan_steps = a[2]
        if (a[1] == "alloc_scan_miss") scan_miss = a[2]
      }
    }
    /^h9_slab_entry / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "malloc_medium") entry_mm = a[2]
        if (a[1] == "malloc_fallback") entry_mf = a[2]
        if (a[1] == "free_ready") entry_fr = a[2]
        if (a[1] == "free_arena") entry_fa = a[2]
        if (a[1] == "free_maybe_false") entry_fmf = a[2]
        if (a[1] == "free_slab_outer") entry_fso = a[2]
        if (a[1] == "realloc_check") entry_rc = a[2]
        if (a[1] == "realloc_slab") entry_rs = a[2]
      }
    }
    /^h9_slab_pending / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "remote_claim") remote = a[2]
        if (a[1] == "direct_slot") direct = a[2]
        if (a[1] == "queue_slot") queue = a[2]
        if (a[1] == "purge_slot") purge = a[2]
      }
    }
    /^h9_slab_direct / {
      for (i = 1; i <= NF; ++i) {
        split($i, a, "=")
        if (a[1] == "try_active") try_active = a[2]
        if (a[1] == "try_partial") try_partial = a[2]
        if (a[1] == "try_after_owner") try_after = a[2]
        if (a[1] == "try_new_page") try_new = a[2]
        if (a[1] == "pending_active") pend_active = a[2]
        if (a[1] == "pending_partial") pend_partial = a[2]
        if (a[1] == "pending_after_owner") pend_after = a[2]
        if (a[1] == "pending_new_page") pend_new = a[2]
      }
    }
    END {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
        row, variant, run, ops, post, peak, faults,
        remote + 0, direct + 0, alloc_hit + 0, free_first + 0, retry_hit + 0,
        try_active + 0, try_partial + 0, try_after + 0, try_new + 0,
        pend_active + 0, pend_partial + 0, pend_after + 0, pend_new + 0,
        queue + 0, purge + 0,
        pages + 0, bytes + 0, raw + 0, cap + 0,
        find_active + 0, find_hash + 0, find_miss + 0, find_no_pages + 0,
        free_local + 0, scan_steps + 0, scan_miss + 0,
        install[1] + 0, install[2] + 0, install[3] + 0,
        install[4] + 0, install[5] + 0, install[6] + 0,
        cap_class[1] + 0, cap_class[2] + 0, cap_class[3] + 0,
        cap_class[4] + 0, cap_class[5] + 0, cap_class[6] + 0,
        entry_mm + 0, entry_mf + 0, entry_fr + 0, entry_fa + 0,
        entry_fmf + 0, entry_fso + 0, entry_rc + 0, entry_rs + 0,
        logfile
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
data = list(csv.DictReader(open(src, newline="")))
groups = defaultdict(list)
for row in data:
    groups[(row["row"], row["variant"])].append(row)

def med(rows, key):
    vals = [float(r[key]) for r in rows if r[key] != ""]
    return statistics.median(vals) if vals else 0.0

order = []
variant_order = []
for row in data:
    if row["row"] not in order:
        order.append(row["row"])
    if row["variant"] not in variant_order:
        variant_order.append(row["variant"])

lines = ["# HZ9 SlabPage RSS Probe", "", "Debug/counter builds; use release gates separately for promotion.", "", "| row | variant | ops | vs baseline | find active | find hash | find miss | free local | scan/hit | scan miss | remote | direct | free-first | retry-hit | entry malloc medium | entry malloc fallback | entry free arena | entry free slab | registered | raw | cap fallback | cap class | install class | post RSS |", "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|---:|"]
for row in order:
    b_ops = med(groups.get((row, "baseline"), []), "throughput")
    for variant in variant_order:
        rows = groups.get((row, variant), [])
        if not rows:
            continue
        ops = med(rows, "throughput")
        ratio = ops / b_ops if b_ops else 0.0
        hits = med(rows, "alloc_hit")
        scan_per_hit = med(rows, "scan_steps") / hits if hits else 0.0
        cap_class = ",".join(str(int(med(rows, f"cap{i}"))) for i in range(6))
        install_class = ",".join(str(int(med(rows, f"install{i}"))) for i in range(6))
        lines.append(
            f"| {row} | {variant} | {ops:.3f} | {ratio:.3f} | "
            f"{med(rows, 'find_active'):.0f} | "
            f"{med(rows, 'find_hash'):.0f} | "
            f"{med(rows, 'find_miss'):.0f} | "
            f"{med(rows, 'free_local'):.0f} | {scan_per_hit:.2f} | "
            f"{med(rows, 'scan_miss'):.0f} | "
            f"{med(rows, 'remote_claim'):.0f} | {med(rows, 'direct_slot'):.0f} | "
            f"{med(rows, 'free_first_hit'):.0f} | "
            f"{med(rows, 'pending_retry_hit'):.0f} | "
            f"{med(rows, 'entry_malloc_medium'):.0f} | "
            f"{med(rows, 'entry_malloc_fallback'):.0f} | "
            f"{med(rows, 'entry_free_arena'):.0f} | "
            f"{med(rows, 'entry_free_slab_outer'):.0f} | "
            f"{med(rows, 'registered_bytes'):.0f} | "
            f"{med(rows, 'raw_reserved'):.0f} | "
            f"{med(rows, 'thread_cap_fallback'):.0f} | {cap_class} | "
            f"{install_class} | {med(rows, 'post_rss'):.0f} |"
        )

open(dst, "w").write("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-slab-rss] logs=${OUTDIR}"
