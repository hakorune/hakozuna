#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_general_default_linux_gate_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-10}"
WORK_SCALE="${WORK_SCALE:-10}"
ROLLBACK="${ROOT}/h8_bench_release_v2_rollback"
DEFAULT="${ROOT}/h8_bench_release"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-release-v2-rollback bench-release >/dev/null

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
work_scale=${WORK_SCALE}
rollback_flags=HZ8_V2_ROLLBACK_CFLAGS
default_flags=HZ8_DEFAULT_CFLAGS,GeneralMediumPage,EntryBoundary-L1A
diagnostic_counters=disabled
worker=working_set_ring
environment=$(uname -a)
EOF

csv="${OUTDIR}/samples.csv"
printf 'row,run,order,allocator,throughput,post_rss,peak_rss,log\n' >"${csv}"

row_args() {
  case "$1" in
    fixed8k) printf '%s\n' '--threads 4 --iters 200000 --min-size 8192 --max-size 8192 --live-window 256' ;;
    fixed16k) printf '%s\n' '--threads 4 --iters 120000 --min-size 16384 --max-size 16384 --live-window 256' ;;
    fixed32k) printf '%s\n' '--threads 4 --iters 80000 --min-size 32768 --max-size 32768 --live-window 256' ;;
    balanced) printf '%s\n' '--threads 8 --iters 25000 --min-size 16 --max-size 2048 --live-window 4096' ;;
    wide_ws) printf '%s\n' '--threads 8 --iters 20000 --min-size 16 --max-size 1024 --live-window 16384' ;;
    larger_sizes) printf '%s\n' '--threads 4 --iters 15000 --min-size 256 --max-size 8192 --live-window 4096' ;;
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
  "${bin}" --runs 1 --remote-pct 0 --interleaved 0 \
    --working-set-ring 1 ${args} >"${log}" 2>&1
  throughput="$(awk '/^throughput / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  post="$(awk '/^post_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  peak="$(awk '/^peak_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  [[ -n "${throughput}" && -n "${post}" && -n "${peak}" ]]
  printf '%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "${row}" "${run}" "${order}" "${name}" "${throughput}" \
    "${post}" "${peak}" "${log}" >>"${csv}"
}

rows=(fixed8k fixed16k fixed32k balanced wide_ws larger_sizes)
for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    if ((run % 2 == 1)); then
      run_one "${row}" "${run}" AB rollback "${ROLLBACK}"
      run_one "${row}" "${run}" AB default "${DEFAULT}"
    else
      run_one "${row}" "${run}" BA default "${DEFAULT}"
      run_one "${row}" "${run}" BA rollback "${ROLLBACK}"
    fi
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
    out.write("# HZ8 General Medium Default Linux Gate\n\n")
    out.write("Fresh-process rollback/default AB/BA working-set rotation.\n\n")
    out.write("| row | rollback | default | delta | rollback post | default post | rollback peak | default peak |\n")
    out.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in ("fixed8k", "fixed16k", "fixed32k", "balanced", "wide_ws", "larger_sizes"):
        rollback = groups[row]["rollback"]
        default = groups[row]["default"]
        rops = statistics.median(float(x["throughput"]) for x in rollback)
        dops = statistics.median(float(x["throughput"]) for x in default)
        rpost = statistics.median(int(x["post_rss"]) for x in rollback)
        dpost = statistics.median(int(x["post_rss"]) for x in default)
        rpeak = statistics.median(int(x["peak_rss"]) for x in rollback)
        dpeak = statistics.median(int(x["peak_rss"]) for x in default)
        out.write(f"| {row} | {rops/1e6:.3f}M | {dops/1e6:.3f}M | {(dops/rops-1)*100:+.2f}% | {rpost/1048576:.2f} MiB | {dpost/1048576:.2f} MiB | {rpeak/1048576:.2f} MiB | {dpeak/1048576:.2f} MiB |\n")
PY

cat "${OUTDIR}/summary.md"
echo "results=${OUTDIR}"
