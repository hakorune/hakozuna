#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="${1:-$(date -u +%Y%m%dT%H%M%SZ)}"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/${STAMP}_hz9_candidate_gate}"
RUNS="${RUNS:-5}"
THREADS="${THREADS:-16}"
ITERS="${ITERS:-60000}"
ROWS="${ROWS:-fixed64_local0,medium_local0,main_local0,medium_r50,main_r90}"
# Evidence-only variants such as tlscache_remoteclass stay opt-in:
#   VARIANTS=baseline,tlscache_remoteclass scripts/run_hz9_candidate_gate.sh
VARIANTS="${VARIANTS:-baseline,tlscache,tlscache_gated,slab4}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" \
  bench-release \
  bench-release-hz9mediumtlscache \
  bench-release-hz9mediumtlscache-gated \
  bench-release-hz9mediumtlscache-remoteclass \
  bench-release-hz9localentry \
  bench-release-hz9localentry-tlscache \
  bench-release-hz9localarena \
  bench-release-hz9localarena-adaptive \
  bench-release-hz9localarena-adaptive-min4 \
  bench-release-hz9localarena-adaptive-min5 \
  bench-release-hz9localarena-adaptive-class4 \
  bench-release-hz9localarena-adaptive-max3 \
  bench-release-hz9localarena-adaptive-min2-max3 \
  bench-release-hz9localarena-dense-adaptive \
  bench-release-hz9localarena-dense-adaptive-min4 \
  bench-release-hz9localarena-dense-ownerfast \
  bench-release-hz9localarena-dense-ownerfast-retire \
  bench-release-hz9localarena-dense-ownerfast-retire-prob8 \
  bench-release-hz9localarena-dense-ownerfast-admit \
  bench-release-hz9localarena-dense-ownerfast-admit-min4 \
  bench-release-hz9localarena-dense-ownerfast-remotesafe \
  bench-release-hz9mediumslabpage \
  bench-release-hz9mediumslabpage-classes-min0 \
  bench-release-hz9mediumslabpage-classes-min0-midcap \
  bench-release-hz9mediumslabpage-classes-min0-layout32 \
  bench-release-hz9mediumslabpage-classes-min0-entry \
  bench-release-hz9mediumslabpage-classes-min0-sidecar \
  bench-release-hz9mediumslabpage-classes-min0-sidecar2 \
  bench-release-hz9mediumslabpage-classes-min0-entry-sidecar2 \
  bench-release-hz9mediumslabpage-classes-min0-localfast \
  bench-release-hz9mediumslabpage-direct-use-proof \
  bench-release-hz9mediumslabpage-classes-min0-localfast-adaptive-hot \
  bench-release-hz9mediumslabpage-classes-min0-localfast-adaptive-poll32 \
  bench-release-hz9mediumslabpage-classes-min2-localfast-adaptive-poll32 \
  bench-release-hz9mediumslabpage-classes-min4-localfast-adaptive-poll32 \
  bench-release-hz9mediumslabpage-classes-min4-localfast-adaptive-poll32-bypass \
  bench-release-hz9mediumslabpage-classes-min4-integrated-localfast-adaptive \
  bench-release-hz9mediumslabpage-classes-min4-integrated-no-free-route-proof \
  bench-release-hz9mediumslabpage-classes-min4-integrated-no-route-proof \
  bench-release-hz9mediumslabpage-classes-min4-integrated-layout-neutral-proof \
  bench-release-hz9mediumslabpage-classes \
  bench-release-hz9mediumslabpage-classes-min3 \
  bench-release-hz9mediumslabpage-classes-min4 \
  bench-release-hz9mediumslabpage-adaptive \
  bench-release-hz9mediumslabpage-adaptive-min5 >/dev/null
make -C "${ROOT}" \
  bench-release-hz9mediumslabpage-adaptive-layout32 \
  bench-release-hz9mediumslabpage-adaptive-entry \
  bench-release-hz9mediumslabpage-adaptive-entry-hotmask >/dev/null

row_args() {
  case "$1" in
    fixed64_local0)
      printf '%s\n' "--min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    fixed48_local0)
      printf '%s\n' "--min-size 49152 --max-size 49152 --remote-pct 0 --interleaved 0"
      ;;
    medium_local0)
      printf '%s\n' "--min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0"
      ;;
    main_local0)
      printf '%s\n' "--min-size 16 --max-size 32768 --remote-pct 0 --interleaved 0"
      ;;
    guard_local0)
      printf '%s\n' "--min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --live-window 0"
      ;;
    small_interleaved_remote90)
      printf '%s\n' "--min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --live-window 0"
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
    tlscache) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumtlscache" ;;
    tlscache_gated)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumtlscache_gated"
      ;;
    tlscache_remoteclass)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumtlscache_remoteclass"
      ;;
    localentry)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localentry"
      ;;
    localentry_tlscache)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localentry_tlscache"
      ;;
    localarena)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena"
      ;;
    localarena_adaptive)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_adaptive"
      ;;
    localarena_adaptive_min4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_adaptive_min4"
      ;;
    localarena_adaptive_min5)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_adaptive_min5"
      ;;
    localarena_adaptive_class4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_adaptive_class4"
      ;;
    localarena_adaptive_max3)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_adaptive_max3"
      ;;
    localarena_adaptive_min2_max3)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_adaptive_min2_max3"
      ;;
    localarena_dense_adaptive)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_adaptive"
      ;;
    localarena_dense_adaptive_min4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_adaptive_min4"
      ;;
    localarena_dense_ownerfast)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast"
      ;;
    localarena_dense_ownerfast_retire)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast_retire"
      ;;
    localarena_dense_ownerfast_retire_prob8)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast_retire_prob8"
      ;;
    localarena_dense_ownerfast_admit)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast_admit"
      ;;
    localarena_dense_ownerfast_admit_min4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast_admit_min4"
      ;;
    localarena_dense_ownerfast_remotesafe)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast_remotesafe"
      ;;
    localarena_dense_ownerfast_remoteactive)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9localarena_dense_ownerfast_remoteactive"
      ;;
    slab4) printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage" ;;
    slabclasses_min0)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0"
      ;;
    slabclasses_min0_midcap)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_midcap"
      ;;
    slabclasses_min0_layout32)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_layout32"
      ;;
    slabclasses_min0_entry)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_entry"
      ;;
    slabclasses_min0_sidecar)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_sidecar"
      ;;
    slabclasses_min0_sidecar2)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_sidecar2"
      ;;
    slabclasses_min0_entry_sidecar2)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_entry_sidecar2"
      ;;
    slablocalfast)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_localfast"
      ;;
    slabdirectuse)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_direct_use_proof"
      ;;
    slablocalfast_adaptive_hot)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_localfast_adaptive_hot"
      ;;
    slablocalfast_adaptive_poll32)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min0_localfast_adaptive_poll32"
      ;;
    slablocalfast_adaptive_poll32_min2)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min2_localfast_adaptive_poll32"
      ;;
    slablocalfast_adaptive_poll32_min4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_localfast_adaptive_poll32"
      ;;
    slablocalfast_adaptive_poll32_min4_bypass)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_localfast_adaptive_poll32_bypass"
      ;;
    slabintegrated_min4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_localfast_adaptive"
      ;;
    slabintegrated_min4_nofreeroute)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_no_free_route_proof"
      ;;
    slabintegrated_min4_noroute)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_no_route_proof"
      ;;
    slabintegrated_min4_layoutneutral)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4_integrated_layout_neutral_proof"
      ;;
    slabclasses)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes"
      ;;
    slabclasses_min3)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min3"
      ;;
    slabclasses_min4)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_classes_min4"
      ;;
    slabadaptive)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive"
      ;;
    slabadaptive_min5)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_min5"
      ;;
    slabadaptive_layout32)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_layout32"
      ;;
    slabadaptive_entry)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_entry"
      ;;
    slabadaptive_entry_hotmask)
      printf '%s\n' "${ROOT}/h8_bench_release_hz9mediumslabpage_adaptive_entry_hotmask"
      ;;
    *)
      echo "unknown variant: $1" >&2
      return 1
      ;;
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
row_order, variant_order = [], []
for item in data:
    groups[(item["row"], item["variant"])].append(item)
    if item["row"] not in row_order:
        row_order.append(item["row"])
    if item["variant"] not in variant_order:
        variant_order.append(item["variant"])

def values(rows, key):
    return [float(row[key]) for row in rows if row[key] != ""]

def median(rows, key):
    vals = values(rows, key)
    return statistics.median(vals) if vals else 0.0

def q(rows, key, idx):
    vals = sorted(values(rows, key))
    if not vals:
        return 0.0
    if len(vals) == 1:
        return vals[0]
    return statistics.quantiles(vals, n=4, method="inclusive")[idx]

lines = [
    "# HZ9 Candidate Gate",
    "",
    "Release builds. This is a direction-finding gate, not promotion evidence.",
    "",
    "| row | variant | median ops | ratio | p25 ratio | peak RSS |",
    "|---|---|---:|---:|---:|---:|",
]
for row in row_order:
    base = groups.get((row, "baseline"), [])
    base_med = median(base, "throughput")
    base_p25 = q(base, "throughput", 0)
    for variant in variant_order:
        rows = groups.get((row, variant), [])
        if not rows:
            continue
        med = median(rows, "throughput")
        p25 = q(rows, "throughput", 0)
        ratio = med / base_med if base_med else 0.0
        p25_ratio = p25 / base_p25 if base_p25 else 0.0
        lines.append(
            f"| {row} | {variant} | {med:.3f} | {ratio:.3f} | "
            f"{p25_ratio:.3f} | {median(rows, 'peak_rss'):.0f} |"
        )

open(dst, "w").write("\n".join(lines) + "\n")
print("\n".join(lines))
PY

echo "[hz9-candidate-gate] logs=${OUTDIR}"
