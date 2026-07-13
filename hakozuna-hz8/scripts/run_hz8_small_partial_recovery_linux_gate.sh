#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_small_partial_recovery_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-10}"
WORK_SCALE="${WORK_SCALE:-10}"
REMOTE_WORK_SCALE="${REMOTE_WORK_SCALE:-1}"
TRACE_PARITY="${TRACE_PARITY:-0}"
RECOVERY_TARGET="${RECOVERY_TARGET:-bench-release-small-partial-transition-only}"
RECOVERY_BIN="${RECOVERY_BIN:-${ROOT}/h8_bench_release_small_partial_transition_only}"
RECOVERY_LABEL="${RECOVERY_LABEL:-transition_only}"
BASELINE="${ROOT}/h8_bench_release"
ORIGINAL="${ROOT}/h8_bench_release_small_partial_depot"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-release bench-release-small-partial-depot "${RECOVERY_TARGET}" >/dev/null
[[ -x "${RECOVERY_BIN}" ]]

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
work_scale=${WORK_SCALE}
remote_work_scale=${REMOTE_WORK_SCALE}
trace_parity=${TRACE_PARITY}
recovery_target=${RECOVERY_TARGET}
recovery_bin=${RECOVERY_BIN}
recovery_label=${RECOVERY_LABEL}
environment=$(uname -a)
EOF

csv="${OUTDIR}/samples.csv"
printf 'row,run,order,allocator,throughput,post_rss,peak_rss,remote_enqueue,local_free,trace_hash,log\n' >"${csv}"

row_args() {
  case "$1" in
    fixed8k) printf '%s\n' "--threads 4 --iters $((200000 * WORK_SCALE)) --min-size 8192 --max-size 8192 --remote-pct 0 --interleaved 0 --working-set-ring 1 --working-set-lcg ${TRACE_PARITY} --live-window 256" ;;
    fixed16k) printf '%s\n' "--threads 4 --iters $((120000 * WORK_SCALE)) --min-size 16384 --max-size 16384 --remote-pct 0 --interleaved 0 --working-set-ring 1 --working-set-lcg ${TRACE_PARITY} --live-window 256" ;;
    fixed32k) printf '%s\n' "--threads 4 --iters $((80000 * WORK_SCALE)) --min-size 32768 --max-size 32768 --remote-pct 0 --interleaved 0 --working-set-ring 1 --working-set-lcg ${TRACE_PARITY} --live-window 256" ;;
    balanced) printf '%s\n' "--threads 8 --iters $((25000 * WORK_SCALE)) --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0 --working-set-ring 1 --working-set-lcg ${TRACE_PARITY} --live-window 4096" ;;
    wide_ws) printf '%s\n' "--threads 8 --iters $((20000 * WORK_SCALE)) --min-size 16 --max-size 1024 --remote-pct 0 --interleaved 0 --working-set-ring 1 --working-set-lcg ${TRACE_PARITY} --live-window 16384" ;;
    larger_sizes) printf '%s\n' "--threads 4 --iters $((15000 * WORK_SCALE)) --min-size 256 --max-size 8192 --remote-pct 0 --interleaved 0 --working-set-ring 1 --working-set-lcg ${TRACE_PARITY} --live-window 4096" ;;
    remote_small) printf '%s\n' "--threads 16 --iters $((100000 * REMOTE_WORK_SCALE)) --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1 --working-set-ring 0 --working-set-lcg 0 --live-window 0" ;;
    *) echo "unknown row: $1" >&2; return 2 ;;
  esac
}

run_one() {
  local row="$1" run="$2" order="$3" name="$4" bin="$5"
  local args log throughput post peak remote_enqueue local_free trace_hash
  args="$(row_args "${row}")"
  log="${OUTDIR}/${row}_r${run}_${name}.log"
  # shellcheck disable=SC2086
  "${bin}" --runs 1 ${args} >"${log}" 2>&1
  throughput="$(awk '/^throughput / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  post="$(awk '/^post_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  peak="$(awk '/^peak_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  remote_enqueue="$(awk '/^interleaved_work / { for (i=1;i<=NF;i++) if ($i ~ /^remote_enqueue=/) {split($i,a,"="); print a[2]} }' "${log}")"
  local_free="$(awk '/^interleaved_work / { for (i=1;i<=NF;i++) if ($i ~ /^local_free=/) {split($i,a,"="); print a[2]} }' "${log}")"
  trace_hash="$(awk '/^working_set_trace / { for (i=1;i<=NF;i++) if ($i ~ /^hash=/) {split($i,a,"="); print a[2]} }' "${log}")"
  [[ -n "${throughput}" && -n "${post}" && -n "${peak}" ]]
  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "${row}" "${run}" "${order}" "${name}" "${throughput}" "${post}" "${peak}" \
    "${remote_enqueue}" "${local_free}" "${trace_hash}" "${log}" >>"${csv}"
}

rows=(fixed8k fixed16k fixed32k balanced wide_ws larger_sizes remote_small)
for run in $(seq 1 "${RUNS}"); do
  for row in "${rows[@]}"; do
    case $(((run - 1) % 3)) in
      0)
        run_one "${row}" "${run}" ABC default "${BASELINE}"
        run_one "${row}" "${run}" ABC original "${ORIGINAL}"
        run_one "${row}" "${run}" ABC "${RECOVERY_LABEL}" "${RECOVERY_BIN}"
        ;;
      1)
        run_one "${row}" "${run}" BCA original "${ORIGINAL}"
        run_one "${row}" "${run}" BCA "${RECOVERY_LABEL}" "${RECOVERY_BIN}"
        run_one "${row}" "${run}" BCA default "${BASELINE}"
        ;;
      2)
        run_one "${row}" "${run}" CAB "${RECOVERY_LABEL}" "${RECOVERY_BIN}"
        run_one "${row}" "${run}" CAB default "${BASELINE}"
        run_one "${row}" "${run}" CAB original "${ORIGINAL}"
        ;;
    esac
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" "${RECOVERY_LABEL}" "${TRACE_PARITY}" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst, recovery_label, trace_parity = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
groups = defaultdict(lambda: defaultdict(list))
for item in csv.DictReader(open(src, newline="")):
    groups[item["row"]][item["allocator"]].append(item)

def median(items, field, cast=float):
    return statistics.median(cast(x[field]) for x in items)

with open(dst, "w", newline="") as out:
    out.write("# HZ8 Small Partial Recovery Linux Gate\n\n")
    out.write(f"Recovery: `{recovery_label}`; trace parity: `{trace_parity}`.\n\n")
    out.write("| row | default | original | original/default | recovery | recovery/default | recovery/original | default peak | original peak | recovery peak |\n")
    out.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in ("fixed8k", "fixed16k", "fixed32k", "balanced", "wide_ws", "larger_sizes", "remote_small"):
        base = groups[row]["default"]
        original = groups[row]["original"]
        recovery = groups[row][recovery_label]
        bops = median(base, "throughput")
        oops = median(original, "throughput")
        rops = median(recovery, "throughput")
        bpeak = median(base, "peak_rss", int)
        opeak = median(original, "peak_rss", int)
        rpeak = median(recovery, "peak_rss", int)
        out.write(f"| {row} | {bops/1e6:.3f}M | {oops/1e6:.3f}M | {(oops/bops-1)*100:+.2f}% | {rops/1e6:.3f}M | {(rops/bops-1)*100:+.2f}% | {(rops/oops-1)*100:+.2f}% | {bpeak/1048576:.2f} MiB | {opeak/1048576:.2f} MiB | {rpeak/1048576:.2f} MiB |\n")

    remote = groups["remote_small"]
    work = {
        name: {(int(x["remote_enqueue"]), int(x["local_free"])) for x in samples}
        for name, samples in remote.items()
    }
    if len(set(map(frozenset, work.values()))) != 1:
        raise SystemExit(f"remote work mismatch: {work}")
    out.write("\n- remote work matched: `yes`\n")

    if trace_parity:
        for row in ("fixed8k", "fixed16k", "fixed32k", "balanced", "wide_ws", "larger_sizes"):
            hashes = {x["trace_hash"] for samples in groups[row].values() for x in samples}
            if len(hashes) != 1 or "" in hashes:
                raise SystemExit(f"trace mismatch row={row}: {sorted(hashes)}")
        out.write("- three-way trace hashes matched: `yes`\n")

print(open(dst).read(), end="")
PY

echo "results=${OUTDIR}"
