#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTDIR="${OUTDIR:-${ROOT}/bench_results/hz8_small_transition_inventory_final_$(date -u +%Y%m%dT%H%M%SZ)}"
BLOCKS="${BLOCKS:-10}"
WORK_SCALE="${WORK_SCALE:-10}"
BASELINE="${ROOT}/h8_bench_release_pre_transition_post_rss_control"
CANDIDATE="${ROOT}/h8_bench_release_small_transition_inventory_post_rss_control"

mkdir -p "${OUTDIR}"
make -C "${ROOT}" bench-release-pre-transition-post-rss-control \
  bench-release-small-transition-inventory-post-rss-control \
  smoke-small-transition-inventory safety-stress-small-transition-inventory >/dev/null
"${ROOT}/h8_smoke_small_transition_inventory" >"${OUTDIR}/smoke.log"
"${ROOT}/h8_safety_stress_small_transition_inventory" >"${OUTDIR}/safety_stress.log"

cat >"${OUTDIR}/manifest.txt" <<EOF
commit=$(git -C "${ROOT}" rev-parse HEAD)
date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
blocks=${BLOCKS}
samples_per_allocator=$((BLOCKS * 2))
work_scale=${WORK_SCALE}
order=ABBA
rows=xorshift_wide,lcg_wide
settle_ms=100
trim_control=malloc_trim(0)
baseline_flags=HZ8_PRE_TRANSITION_DEFAULT_CFLAGS,H8_BENCH_POST_RSS_TRIM_CONTROL
candidate_flags=HZ8_DEFAULT_CFLAGS,H8_BENCH_POST_RSS_TRIM_CONTROL
allocator_behavior_change=none
environment=$(uname -a)
EOF

csv="${OUTDIR}/samples.csv"
printf 'row,block,position,allocator,throughput,post_rss,peak_rss,trace_hash,trim_result,raw_vm_rss,raw_rss_anon,raw_rss_file,raw_private_dirty,raw_anonymous,settled_vm_rss,settled_rss_anon,settled_private_dirty,settled_anonymous,trim_vm_rss,trim_rss_anon,trim_private_dirty,trim_anonymous,log\n' >"${csv}"

control_field() {
  local key="$1" log="$2"
  awk -v key="${key}" '/^post_rss_control / {
    for (i=1; i<=NF; ++i) {
      split($i, pair, "=")
      if (pair[1] == key) print pair[2]
    }
  }' "${log}"
}

run_one() {
  local row="$1" block="$2" position="$3" allocator="$4" bin="$5"
  local lcg=0 log throughput post peak trace trim_result
  local raw_vm raw_anon raw_file raw_dirty raw_anonymous
  local settled_vm settled_anon settled_dirty settled_anonymous
  local trim_vm trim_anon trim_dirty trim_anonymous
  if [[ "${row}" == "lcg_wide" ]]; then
    lcg=1
  fi
  log="${OUTDIR}/${row}_b${block}_${position}_${allocator}.log"
  "${bin}" --runs 1 --threads 8 --iters "$((20000 * WORK_SCALE))" \
    --min-size 16 --max-size 1024 --remote-pct 0 --interleaved 0 \
    --working-set-ring 1 --working-set-lcg "${lcg}" --live-window 16384 \
    >"${log}" 2>&1
  throughput="$(awk '/^throughput / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  post="$(awk '/^post_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  peak="$(awk '/^peak_rss / { for (i=1;i<=NF;i++) if ($i ~ /^median=/) {split($i,a,"="); print a[2]} }' "${log}")"
  trace="$(awk '/^working_set_trace / { for (i=1;i<=NF;i++) if ($i ~ /^hash=/) {split($i,a,"="); print a[2]} }' "${log}")"
  trim_result="$(control_field trim_result "${log}")"
  raw_vm="$(control_field raw_vm_rss "${log}")"
  raw_anon="$(control_field raw_rss_anon "${log}")"
  raw_file="$(control_field raw_rss_file "${log}")"
  raw_dirty="$(control_field raw_private_dirty "${log}")"
  raw_anonymous="$(control_field raw_anonymous "${log}")"
  settled_vm="$(control_field settled_vm_rss "${log}")"
  settled_anon="$(control_field settled_rss_anon "${log}")"
  settled_dirty="$(control_field settled_private_dirty "${log}")"
  settled_anonymous="$(control_field settled_anonymous "${log}")"
  trim_vm="$(control_field trim_vm_rss "${log}")"
  trim_anon="$(control_field trim_rss_anon "${log}")"
  trim_dirty="$(control_field trim_private_dirty "${log}")"
  trim_anonymous="$(control_field trim_anonymous "${log}")"
  [[ -n "${throughput}" && -n "${post}" && -n "${peak}" && -n "${trim_vm}" ]]
  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "${row}" "${block}" "${position}" "${allocator}" "${throughput}" \
    "${post}" "${peak}" "${trace}" "${trim_result}" "${raw_vm}" \
    "${raw_anon}" "${raw_file}" "${raw_dirty}" "${raw_anonymous}" \
    "${settled_vm}" "${settled_anon}" "${settled_dirty}" \
    "${settled_anonymous}" "${trim_vm}" "${trim_anon}" "${trim_dirty}" \
    "${trim_anonymous}" "${log}" >>"${csv}"
}

for block in $(seq 1 "${BLOCKS}"); do
  for row in xorshift_wide lcg_wide; do
    run_one "${row}" "${block}" A1 default "${BASELINE}"
    run_one "${row}" "${block}" B1 inventory "${CANDIDATE}"
    run_one "${row}" "${block}" B2 inventory "${CANDIDATE}"
    run_one "${row}" "${block}" A2 default "${BASELINE}"
  done
done

python3 - "${csv}" "${OUTDIR}/summary.md" <<'PY'
import csv
import statistics
import sys
from collections import defaultdict

src, dst = sys.argv[1:]
groups = defaultdict(lambda: defaultdict(list))
for item in csv.DictReader(open(src, newline="")):
    groups[item["row"]][item["allocator"]].append(item)

def median(items, field, cast=float):
    return statistics.median(cast(x[field]) for x in items)

def mib(value):
    return value / 1048576

def mix_u32(h, value):
    for shift in range(0, 32, 8):
        h ^= (value >> shift) & 0xff
        h = (h * 1099511628211) & 0xffffffffffffffff
    return h

def wide_lcg_oracle():
    h = 14695981039346656037
    for thread in range(8):
        occupied = bytearray(16384)
        state = 1234 + thread
        for _ in range(65536):
            state = (state * 1664525 + 1013904223) & 0xffffffff
            index = state % 16384
            is_free = int(bool(occupied[index]))
            size = 0 if is_free else 16 + state % (1024 - 16 + 1)
            occupied[index] = 0 if is_free else 1
            class_id = 9 if size == 0 or size > 4096 else next(
                i for i, bound in enumerate((16, 32, 64, 128, 256, 512, 1024, 2048, 4096))
                if size <= bound
            )
            for value in (thread, state, index, is_free, size, class_id):
                h = mix_u32(h, value)
    return f"{h:016x}"

observed = {
    x["trace_hash"]
    for allocator in ("default", "inventory")
    for x in groups["lcg_wide"][allocator]
}
expected = wide_lcg_oracle()
if observed != {expected}:
    raise SystemExit(f"LCG trace mismatch observed={sorted(observed)} expected={expected}")

with open(dst, "w") as out:
    out.write("# HZ8 Transition Inventory Final Linux Gate\n\n")
    out.write("Fresh-process ABBA blocks; post-RSS control is outside throughput timing.\n\n")
    out.write("| row | allocator | ops/s | raw post | settled post | trim post | peak | raw anon | trim anon | raw private dirty | trim private dirty |\n")
    out.write("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n")
    verdicts = []
    for row in ("xorshift_wide", "lcg_wide"):
        values = {}
        for allocator in ("default", "inventory"):
            items = groups[row][allocator]
            values[allocator] = {
                "ops": median(items, "throughput"),
                "raw": median(items, "raw_vm_rss", int),
                "settled": median(items, "settled_vm_rss", int),
                "trim": median(items, "trim_vm_rss", int),
                "peak": median(items, "peak_rss", int),
                "raw_anon": median(items, "raw_rss_anon", int),
                "trim_anon": median(items, "trim_rss_anon", int),
                "raw_dirty": median(items, "raw_private_dirty", int),
                "trim_dirty": median(items, "trim_private_dirty", int),
            }
            v = values[allocator]
            out.write(
                f"| {row} | {allocator} | {v['ops']/1e6:.3f}M | {mib(v['raw']):.2f} MiB | "
                f"{mib(v['settled']):.2f} MiB | {mib(v['trim']):.2f} MiB | {mib(v['peak']):.2f} MiB | "
                f"{mib(v['raw_anon']):.2f} MiB | {mib(v['trim_anon']):.2f} MiB | "
                f"{mib(v['raw_dirty']):.2f} MiB | {mib(v['trim_dirty']):.2f} MiB |\n"
            )
        base, candidate = values["default"], values["inventory"]
        limit = max(base["trim"] * 1.05, base["trim"] + 131072)
        trim_pass = candidate["trim"] <= limit
        verdicts.append(trim_pass)
        out.write(
            f"\n- {row}: throughput `{(candidate['ops']/base['ops']-1)*100:+.2f}%`, "
            f"peak `{(candidate['peak']/base['peak']-1)*100:+.2f}%`, "
            f"raw post `{(candidate['raw']/base['raw']-1)*100:+.2f}%`, "
            f"trim post `{(candidate['trim']/base['trim']-1)*100:+.2f}%`, "
            f"trim guard `{'PASS' if trim_pass else 'HOLD'}`\n"
        )
    out.write(f"\n- LCG standalone oracle: `{expected}` (`PASS`)\n")
    out.write(f"- post-RSS trim-control gate: `{'PASS' if all(verdicts) else 'HOLD'}`\n")

print(open(dst).read(), end="")
PY

echo "results=${OUTDIR}"
