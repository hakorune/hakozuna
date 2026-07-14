#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_small_partial_depot_linux_gate_$(date -u +%Y%m%dT%H%M%SZ)}"
RUNS="${RUNS:-10}"
WORK_SCALE="${WORK_SCALE:-10}"
REMOTE_WORK_SCALE="${REMOTE_WORK_SCALE:-1}"
TRACE_PARITY="${TRACE_PARITY:-0}"
BASELINE="${ROOT}/h8_bench_release"
CANDIDATE_TARGET="${CANDIDATE_TARGET:-bench-release-small-partial-depot}"
CANDIDATE="${CANDIDATE:-${ROOT}/h8_bench_release_small_partial_depot}"
CANDIDATE_LABEL="${CANDIDATE_LABEL:-small_partial_depot}"
CANDIDATE_FLAGS="${CANDIDATE_FLAGS:-HZ8_DEFAULT_CFLAGS,H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1}"
CANDIDATE_SMOKE_TARGET="${CANDIDATE_SMOKE_TARGET:-smoke-small-partial-depot}"
CANDIDATE_SMOKE="${CANDIDATE_SMOKE:-${ROOT}/h8_smoke_small_partial_depot}"
CANDIDATE_SAFETY_TARGET="${CANDIDATE_SAFETY_TARGET:-safety-stress-small-partial-depot}"
CANDIDATE_SAFETY="${CANDIDATE_SAFETY:-${ROOT}/h8_safety_stress_small_partial_depot}"
SUMMARY_TITLE="${SUMMARY_TITLE:-HZ8 Small Partial Transition Depot Linux Gate}"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-release "${CANDIDATE_TARGET}" \
  "${CANDIDATE_SMOKE_TARGET}" "${CANDIDATE_SAFETY_TARGET}" >/dev/null

"${CANDIDATE_SMOKE}" >"${OUTDIR}/smoke.log"
"${CANDIDATE_SAFETY}" >"${OUTDIR}/safety_stress.log"

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
runs=${RUNS}
work_scale=${WORK_SCALE}
remote_work_scale=${REMOTE_WORK_SCALE}
trace_parity=${TRACE_PARITY}
baseline_flags=HZ8_DEFAULT_CFLAGS
candidate_label=${CANDIDATE_LABEL}
candidate_target=${CANDIDATE_TARGET}
candidate_flags=${CANDIDATE_FLAGS}
diagnostic_counters=disabled
environment=$(uname -a)
local_worker=working_set_ring
remote_worker=interleaved_remote90
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
    if ((run % 2 == 1)); then
      run_one "${row}" "${run}" AB baseline "${BASELINE}"
      run_one "${row}" "${run}" AB candidate "${CANDIDATE}"
    else
      run_one "${row}" "${run}" BA candidate "${CANDIDATE}"
      run_one "${row}" "${run}" BA baseline "${BASELINE}"
    fi
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" "${TRACE_PARITY}" "${SUMMARY_TITLE}" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst, trace_parity, title = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
groups = defaultdict(lambda: defaultdict(list))
for item in csv.DictReader(open(src, newline="")):
    groups[item["row"]][item["allocator"]].append(item)

with open(dst, "w", newline="") as out:
    out.write(f"# {title}\n\n")
    out.write("Fresh-process AB/BA rotation; speed binaries only.\n\n")
    out.write("| row | default | candidate | delta | default post RSS | candidate post RSS | default peak RSS | candidate peak RSS |\n")
    out.write("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    for row in ("fixed8k", "fixed16k", "fixed32k", "balanced", "wide_ws", "larger_sizes", "remote_small"):
        base = groups[row]["baseline"]
        cand = groups[row]["candidate"]
        bops = statistics.median(float(x["throughput"]) for x in base)
        cops = statistics.median(float(x["throughput"]) for x in cand)
        bpost = statistics.median(int(x["post_rss"]) for x in base)
        cpost = statistics.median(int(x["post_rss"]) for x in cand)
        bpeak = statistics.median(int(x["peak_rss"]) for x in base)
        cpeak = statistics.median(int(x["peak_rss"]) for x in cand)
        out.write(f"| {row} | {bops/1e6:.3f}M | {cops/1e6:.3f}M | {(cops/bops-1)*100:+.2f}% | {bpost/1048576:.2f} MiB | {cpost/1048576:.2f} MiB | {bpeak/1048576:.2f} MiB | {cpeak/1048576:.2f} MiB |\n")

    remote = groups["remote_small"]
    counts = {
        name: {(int(x["remote_enqueue"]), int(x["local_free"])) for x in samples}
        for name, samples in remote.items()
    }
    out.write("\n## Remote Work\n\n")
    out.write(f"- default remote_enqueue/local_free: `{sorted(counts['baseline'])}`\n")
    out.write(f"- candidate remote_enqueue/local_free: `{sorted(counts['candidate'])}`\n")
    out.write(f"- matched: `{'yes' if counts['baseline'] == counts['candidate'] else 'no'}`\n")

    trace_rows = {
        "fixed8k": (4, 256, 8192, 8192),
        "fixed16k": (4, 256, 16384, 16384),
        "fixed32k": (4, 256, 32768, 32768),
        "balanced": (8, 4096, 16, 2048),
        "wide_ws": (8, 16384, 16, 1024),
        "larger_sizes": (4, 4096, 256, 8192),
    }
    def mix_u32(h, value):
        for shift in range(0, 32, 8):
            h ^= (value >> shift) & 0xff
            h = (h * 1099511628211) & 0xffffffffffffffff
        return h
    def oracle(threads, window, lo, hi):
        h = 14695981039346656037
        for thread in range(threads):
            occupied = bytearray(window)
            state = 1234 + thread
            for _ in range(65536):
                state = (state * 1664525 + 1013904223) & 0xffffffff
                index = state % window
                is_free = int(bool(occupied[index]))
                size = 0 if is_free else lo + state % (hi - lo + 1)
                if is_free:
                    occupied[index] = 0
                else:
                    occupied[index] = 1
                if size == 0 or size > 4096:
                    class_id = 9
                else:
                    class_id = next(i for i, bound in enumerate((16, 32, 64, 128, 256, 512, 1024, 2048, 4096)) if size <= bound)
                for value in (thread, state, index, is_free, size, class_id):
                    h = mix_u32(h, value)
        return f"{h:016x}"
    observed = {}
    if trace_parity:
        for row, args in trace_rows.items():
            hashes = {x["trace_hash"] for name in ("baseline", "candidate") for x in groups[row][name]}
            expected = oracle(*args)
            if hashes != {expected}:
                raise SystemExit(f"trace mismatch row={row} observed={sorted(hashes)} expected={expected}")
            observed[row] = expected
        out.write("\n## Trace Parity\n\n")
        for row, value in observed.items():
            out.write(f"- {row}: `{value}`\n")
        out.write("- standalone oracle match: `yes`\n")
PY

cat "${OUTDIR}/summary.md"
echo "results=${OUTDIR}"
