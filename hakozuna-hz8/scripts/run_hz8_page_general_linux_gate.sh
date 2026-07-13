#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_page_general_linux_gate_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-5}"
WORK_SCALE="${WORK_SCALE:-1}"
BASELINE="${ROOT}/h8_bench_release"
GENERAL="${ROOT}/h8_bench_release_page_general"
CANDIDATE="${ROOT}/h8_bench_release_page_general_entry_boundary"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-release bench-release-page-general \
  bench-release-page-general-entry-boundary \
  smoke-page-general-entry-boundary-api safety-stress-page-general-entry-boundary >/dev/null

"${ROOT}/h8_page_general_entry_boundary_api_smoke" >"${OUTDIR}/api_smoke.log"
"${ROOT}/h8_safety_stress_page_general_entry_boundary" >"${OUTDIR}/safety_stress.log"

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
work_scale=${WORK_SCALE}
baseline_flags=HZ8_DEFAULT_CFLAGS
general_flags=HZ8_DEFAULT_CFLAGS,H8_MEDIUM_PAGE8K_REMOTE_L1,H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1,H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1,H8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1
candidate_flags=HZ8_DEFAULT_CFLAGS,H8_MEDIUM_PAGE8K_REMOTE_L1,H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1,H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1,H8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1,H8_MEDIUM_PAGE_ENTRY_BOUNDARY_L1
diagnostic_counters=disabled
environment=$(uname -a)
note=WSL results are directional controls; native Ubuntu is required for cross-platform promotion.
EOF

csv="${OUTDIR}/samples.csv"
printf 'row,run,order,allocator,throughput,post_rss,peak_rss,log\n' >"${csv}"

row_args() {
  case "$1" in
    fixed8k) printf '%s\n' '--threads 4 --iters 200000 --min-size 8192 --max-size 8192 --remote-pct 0 --interleaved 1' ;;
    fixed16k) printf '%s\n' '--threads 4 --iters 120000 --min-size 16384 --max-size 16384 --remote-pct 0 --interleaved 1' ;;
    fixed32k) printf '%s\n' '--threads 4 --iters 80000 --min-size 32768 --max-size 32768 --remote-pct 0 --interleaved 1' ;;
    balanced) printf '%s\n' '--threads 8 --iters 25000 --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 1' ;;
    wide_ws) printf '%s\n' '--threads 8 --iters 20000 --min-size 16 --max-size 1024 --remote-pct 0 --interleaved 1 --live-window 16384' ;;
    larger_sizes) printf '%s\n' '--threads 4 --iters 15000 --min-size 256 --max-size 8192 --remote-pct 0 --interleaved 1 --live-window 4096' ;;
    *) echo "unknown row: $1" >&2; return 2 ;;
  esac
}

run_one() {
  local row="$1" run="$2" order="$3" name="$4" bin="$5"
  local args log throughput post peak
  args="$(row_args "${row}")"
  args="${args/--iters 200000/--iters $((200000 * WORK_SCALE))}"
  args="${args/--iters 120000/--iters $((120000 * WORK_SCALE))}"
  args="${args/--iters 80000/--iters $((80000 * WORK_SCALE))}"
  args="${args/--iters 25000/--iters $((25000 * WORK_SCALE))}"
  args="${args/--iters 20000/--iters $((20000 * WORK_SCALE))}"
  args="${args/--iters 15000/--iters $((15000 * WORK_SCALE))}"
  log="${OUTDIR}/${row}_r${run}_${name}.log"
  # shellcheck disable=SC2086
  "${bin}" --runs 1 ${args} >"${log}" 2>&1
  throughput="$(awk '/^throughput / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  post="$(awk '/^post_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  peak="$(awk '/^peak_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  [[ -n "${throughput}" && -n "${post}" && -n "${peak}" ]]
  printf '%s,%s,%s,%s,%s,%s,%s,%s\n' "${row}" "${run}" "${order}" "${name}" "${throughput}" "${post}" "${peak}" "${log}" >>"${csv}"
}

rows=(fixed8k fixed16k fixed32k balanced wide_ws larger_sizes)
for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    case $(((run - 1) % 3)) in
      0)
        run_one "${row}" "${run}" ABC baseline "${BASELINE}"
        run_one "${row}" "${run}" ABC general "${GENERAL}"
        run_one "${row}" "${run}" ABC entry_boundary "${CANDIDATE}"
        ;;
      1)
        run_one "${row}" "${run}" BCA general "${GENERAL}"
        run_one "${row}" "${run}" BCA entry_boundary "${CANDIDATE}"
        run_one "${row}" "${run}" BCA baseline "${BASELINE}"
        ;;
      2)
        run_one "${row}" "${run}" CAB entry_boundary "${CANDIDATE}"
        run_one "${row}" "${run}" CAB baseline "${BASELINE}"
        run_one "${row}" "${run}" CAB general "${GENERAL}"
        ;;
    esac
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1], sys.argv[2]
groups = defaultdict(lambda: defaultdict(list))
for item in csv.DictReader(open(src, newline="")):
    groups[item["row"]][item["allocator"]].append(item)

with open(dst, "w", newline="") as out:
    out.write("# HZ8 General Medium Page Linux Gate\n\n")
    out.write("Fresh-process three-way rotation; speed binaries only; WSL is directional evidence.\n\n")
    out.write("| row | baseline | general | general delta | entry-boundary | entry delta | entry vs general | baseline peak RSS | entry peak RSS |\n")
    out.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in ("fixed8k", "fixed16k", "fixed32k", "balanced", "wide_ws", "larger_sizes"):
        base = groups[row]["baseline"]
        general = groups[row]["general"]
        cand = groups[row]["entry_boundary"]
        bops = statistics.median(float(x["throughput"]) for x in base)
        gops = statistics.median(float(x["throughput"]) for x in general)
        cops = statistics.median(float(x["throughput"]) for x in cand)
        bpeak = statistics.median(int(x["peak_rss"]) for x in base)
        cpeak = statistics.median(int(x["peak_rss"]) for x in cand)
        out.write(f"| {row} | {bops/1e6:.3f}M | {gops/1e6:.3f}M | {(gops/bops-1)*100:+.2f}% | {cops/1e6:.3f}M | {(cops/bops-1)*100:+.2f}% | {(cops/gops-1)*100:+.2f}% | {bpeak/1048576:.2f} MiB | {cpeak/1048576:.2f} MiB |\n")
PY

cat "${OUTDIR}/summary.md"
echo "results=${OUTDIR}"
