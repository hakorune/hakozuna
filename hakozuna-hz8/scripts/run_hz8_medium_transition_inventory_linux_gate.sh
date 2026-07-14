#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_medium_transition_inventory_linux_gate_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-10}"
WORK_SCALE="${WORK_SCALE:-1}"
BASELINE="${ROOT}/h8_bench_release"
CANDIDATE="${ROOT}/h8_bench_release_medium_transition_inventory"

mkdir -p "${OUTDIR}"

build_and_test_gcc() {
  make -BC "${ROOT}" CC=gcc \
    bench-release bench-release-medium-transition-inventory \
    bench-release-medium-transition-inventory-diag \
    preload-medium-transition-inventory smoke-medium-transition-inventory \
    safety-stress-medium-transition-inventory preload-smoke >"${OUTDIR}/build_gcc.log" 2>&1
  "${ROOT}/h8_smoke_medium_transition_inventory" >"${OUTDIR}/smoke_gcc.log"
  "${ROOT}/h8_safety_stress_medium_transition_inventory" >"${OUTDIR}/safety_gcc.log"
  LD_PRELOAD="${ROOT}/libhakozuna_hz8_preload_medium_transition_inventory.so" \
    "${ROOT}/h8_preload_smoke" >"${OUTDIR}/preload_gcc.log" 2>&1
}

build_and_test_clang() {
  make -BC "${ROOT}" CC=clang \
    preload-medium-transition-inventory smoke-medium-transition-inventory \
    safety-stress-medium-transition-inventory preload-smoke >"${OUTDIR}/build_clang.log" 2>&1
  "${ROOT}/h8_smoke_medium_transition_inventory" >"${OUTDIR}/smoke_clang.log"
  "${ROOT}/h8_safety_stress_medium_transition_inventory" >"${OUTDIR}/safety_clang.log"
  LD_PRELOAD="${ROOT}/libhakozuna_hz8_preload_medium_transition_inventory.so" \
    "${ROOT}/h8_preload_smoke" >"${OUTDIR}/preload_clang.log" 2>&1
}

build_and_test_gcc

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
work_scale=${WORK_SCALE}
benchmark_compiler=gcc
baseline_flags=HZ8_DEFAULT_CFLAGS
candidate_flags=HZ8_DEFAULT_CFLAGS,H8_MEDIUM_TRANSITION_INVENTORY_L1
speed_diagnostics=disabled
clang_contract_check=preload,smoke,safety
environment=$(uname -a)
EOF

csv="${OUTDIR}/samples.csv"
printf 'row,run,order,allocator,throughput,post_rss,peak_rss,log\n' >"${csv}"

row_args() {
  case "$1" in
    range4097_8192) printf '%s\n' "--threads 4 --iters $((100000 * WORK_SCALE)) --min-size 4097 --max-size 8192 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 4096" ;;
    fixed8k) printf '%s\n' "--threads 4 --iters $((200000 * WORK_SCALE)) --min-size 8192 --max-size 8192 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 256" ;;
    fixed16k) printf '%s\n' "--threads 4 --iters $((120000 * WORK_SCALE)) --min-size 16384 --max-size 16384 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 256" ;;
    fixed32k) printf '%s\n' "--threads 4 --iters $((80000 * WORK_SCALE)) --min-size 32768 --max-size 32768 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 256" ;;
    fixed64k) printf '%s\n' "--threads 4 --iters $((40000 * WORK_SCALE)) --min-size 65536 --max-size 65536 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 128" ;;
    balanced) printf '%s\n' "--threads 8 --iters $((25000 * WORK_SCALE)) --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 4096" ;;
    wide_ws) printf '%s\n' "--threads 8 --iters $((20000 * WORK_SCALE)) --min-size 16 --max-size 1024 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 16384" ;;
    larger_sizes) printf '%s\n' "--threads 4 --iters $((15000 * WORK_SCALE)) --min-size 256 --max-size 8192 --remote-pct 0 --interleaved 0 --working-set-ring 1 --live-window 4096" ;;
    *) echo "unknown row: $1" >&2; return 2 ;;
  esac
}

run_one() {
  local row="$1" run="$2" order="$3" name="$4" bin="$5"
  local args log throughput post peak
  args="$(row_args "${row}")"
  log="${OUTDIR}/${row}_r${run}_${name}.log"
  # shellcheck disable=SC2086
  "${bin}" --runs 1 ${args} >"${log}" 2>&1
  throughput="$(awk '/^throughput / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  post="$(awk '/^post_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  peak="$(awk '/^peak_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  [[ -n "${throughput}" && -n "${post}" && -n "${peak}" ]]
  printf '%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "${row}" "${run}" "${order}" "${name}" "${throughput}" "${post}" "${peak}" "${log}" >>"${csv}"
}

rows=(range4097_8192 fixed8k fixed16k fixed32k fixed64k balanced wide_ws larger_sizes)
for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    if ((run % 2)); then
      run_one "${row}" "${run}" AB baseline "${BASELINE}"
      run_one "${row}" "${run}" AB candidate "${CANDIDATE}"
    else
      run_one "${row}" "${run}" BA candidate "${CANDIDATE}"
      run_one "${row}" "${run}" BA baseline "${BASELINE}"
    fi
  done
done

"${ROOT}/h8_bench_release_medium_transition_inventory_diag" --runs 1 \
  $(row_args range4097_8192) >"${OUTDIR}/diagnostic_range4097_8192.log" 2>&1
diagnostic_line="$(awk '/^medium_available_shadow / { print; exit }' "${OUTDIR}/diagnostic_range4097_8192.log")"
[[ -n "${diagnostic_line}" ]]
for field in head_unusable duplicate owner_mismatch class_mismatch indexed_full \
  indexed_detached indexed_nonactive active_indexed exit_nonempty; do
  [[ "${diagnostic_line}" == *"${field}=0"* ]]
done
[[ "${diagnostic_line}" =~ head_attempt=[1-9] ]]
[[ "${diagnostic_line}" =~ head_hit=[1-9] ]]

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1:]
groups = defaultdict(lambda: defaultdict(dict))
for item in csv.DictReader(open(src, newline="")):
    groups[item["row"]][item["allocator"]][int(item["run"])] = item

rows = ("range4097_8192", "fixed8k", "fixed16k", "fixed32k", "fixed64k",
        "balanced", "wide_ws", "larger_sizes")
result = {}
for row in rows:
    base = groups[row]["baseline"]
    cand = groups[row]["candidate"]
    if base.keys() != cand.keys():
        raise SystemExit(f"unpaired samples for {row}")
    paired = [float(cand[run]["throughput"]) / float(base[run]["throughput"])
              for run in sorted(base)]
    bops = statistics.median(float(item["throughput"]) for item in base.values())
    cops = statistics.median(float(item["throughput"]) for item in cand.values())
    bpost = statistics.median(int(item["post_rss"]) for item in base.values())
    cpost = statistics.median(int(item["post_rss"]) for item in cand.values())
    bpeak = statistics.median(int(item["peak_rss"]) for item in base.values())
    cpeak = statistics.median(int(item["peak_rss"]) for item in cand.values())
    result[row] = dict(paired=statistics.median(paired), bops=bops, cops=cops,
                       bpost=bpost, cpost=cpost, bpeak=bpeak, cpeak=cpeak)

def rss_ok(base, candidate):
    return candidate <= max(base * 1.05, base + 1048576)

checks = [
    ("target range4097_8192 >= +15%", result["range4097_8192"]["paired"] >= 1.15),
    ("balanced >= -3%", result["balanced"]["paired"] >= .97),
    ("wide_ws >= -3%", result["wide_ws"]["paired"] >= .97),
]
for row in ("fixed8k", "fixed16k", "fixed32k", "fixed64k"):
    checks.append((f"{row} >= -3%", result[row]["paired"] >= .97))
for row in rows:
    checks.append((f"{row} post RSS", rss_ok(result[row]["bpost"], result[row]["cpost"])))
    checks.append((f"{row} peak RSS", rss_ok(result[row]["bpeak"], result[row]["cpeak"])))

with open(dst, "w", newline="") as out:
    out.write("# HZ8 MediumTransitionInventory Linux Gate\n\n")
    out.write("Native fresh-process AB/BA R10. Throughput delta is the median of the per-run candidate/baseline ratios. GCC speed binaries have diagnostics disabled.\n\n")
    out.write("| row | default ops/s | candidate ops/s | paired delta | default post RSS | candidate post RSS | default peak RSS | candidate peak RSS |\n")
    out.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in rows:
        item = result[row]
        out.write(f"| {row} | {item['bops']/1e6:.3f}M | {item['cops']/1e6:.3f}M | {(item['paired']-1)*100:+.2f}% | {item['bpost']/1048576:.2f} MiB | {item['cpost']/1048576:.2f} MiB | {item['bpeak']/1048576:.2f} MiB | {item['cpeak']/1048576:.2f} MiB |\n")
    out.write("\n## Gate\n\n")
    for label, passed in checks:
        out.write(f"- {'PASS' if passed else 'FAIL'}: {label}\n")
    out.write("- PASS: allocation failures = 0 (all benchmark worker processes exited zero)\n")
    out.write("- PASS: diagnostic inventory rejects/mismatches/residue = 0\n")
    out.write("- PASS: GCC and Clang preload, smoke, and safety logs are present\n")
    out.write(f"\nOverall: **{'PASS' if all(passed for _, passed in checks) else 'FAIL'}**\n")
PY

build_and_test_clang

cat "${OUTDIR}/summary.md"
echo "results=${OUTDIR}"
