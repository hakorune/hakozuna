#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-80000}"
PROFILES_CSV="${PROFILES:-desc10k_source1280_route40k,desc12k_source1536_route48k,desc14k_source1792_route56k,desc16k_source2048_route64k}"
ROWS_CSV="${ROWS:-small_proxy,cache_proxy}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_narrow_ladder_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"
source "${HZ6_DIR}/linux/hz6_workload_profile_ladder_common.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_narrow_ladder.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per row/variant (default: 3)
  --iters N          iterations per run (default: 80000)
  --profiles CSV     static-capacity profiles
  --rows CSV         row groups: small_proxy,cache_proxy,all
  --outdir DIR       output directory
  --skip-builds      reuse existing per-profile builds and benchmark
  --help             show this message

Profile names are desc<size>_source<size>_route<size>, for example:
  desc12k_source1536_route48k

This runner intentionally does not enable elastic descriptor overflow. Use it
to test whether the workload-capacity profile can shrink below lite capacity.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --iters)
      ITERS="$2"
      shift 2
      ;;
    --profiles)
      PROFILES_CSV="$2"
      shift 2
      ;;
    --rows)
      ROWS_CSV="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-builds)
      SKIP_BUILDS=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

build_capacity_profile() {
  local profile="$1"
  local desc_capacity source_capacity route_capacity
  hz6_workload_parse_static_profile "$profile" desc_capacity source_capacity route_capacity

  local flags=()
  hz6_preload_profile_selected_cflags flags 1
  hz6_preload_replace_define flags HZ6_ROUTE_TABLE_CAPACITY "$route_capacity"
  hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY "$desc_capacity"
  hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY "$source_capacity"

  hz6_preload_profile_build flags "${OUTDIR}/build/${profile}"
}

mkdir -p "$OUTDIR/build" "$OUTDIR/logs"
IFS=',' read -r -a profiles <<< "$PROFILES_CSV"

SELECTED_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so"
LITE_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_capacity_lite_target/libhakozuna_hz6_preload.so"
HYBRID_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_capacity_hybrid_target/libhakozuna_hz6_preload.so"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lite_target.sh"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_hybrid_target.sh"
  for profile in "${profiles[@]}"; do
    build_capacity_profile "$profile"
  done
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$LITE_SO" ]] || { echo "missing capacity-lite DSO: $LITE_SO" >&2; exit 2; }
[[ -f "$HYBRID_SO" ]] || { echo "missing hybrid DSO: $HYBRID_SO" >&2; exit 2; }
for profile in "${profiles[@]}"; do
  so="${OUTDIR}/build/${profile}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing capacity DSO: $so" >&2; exit 2; }
done

rows=()
hz6_workload_append_proxy_rows rows "$ITERS" "$ROWS_CSV"

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "profiles=${PROFILES_CSV}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH_BIN}"
  echo "selected_so=${SELECTED_SO}"
  echo "capacity_lite_so=${LITE_SO}"
  echo "capacity_hybrid_so=${HYBRID_SO}"
  for profile in "${profiles[@]}"; do
    echo "capacity_${profile}_so=${OUTDIR}/build/${profile}/libhakozuna_hz6_preload.so"
  done
  for row_spec in "${rows[@]}"; do
    echo "row=${row_spec}"
  done
} > "${OUTDIR}/README.log"

run_one() {
  local row="$1"
  local variant="$2"
  local so="$3"
  local run="$4"
  local threads="$5"
  local iters="$6"
  local ws="$7"
  local min_size="$8"
  local max_size="$9"
  local log="${OUTDIR}/logs/${row}_${variant}_${run}.log"
  env LD_PRELOAD="$so" "$BENCH_BIN" "$threads" "$iters" "$ws" \
    "$min_size" "$max_size" > "$log" 2>&1
}

for row_spec in "${rows[@]}"; do
  read -r row threads iters ws min_size max_size <<< "$row_spec"
  for run in $(seq 1 "$RUNS"); do
    run_one "$row" selected "$SELECTED_SO" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    run_one "$row" capacity_lite "$LITE_SO" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    run_one "$row" capacity_hybrid "$HYBRID_SO" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    for profile in "${profiles[@]}"; do
      so="${OUTDIR}/build/${profile}/libhakozuna_hz6_preload.so"
      run_one "$row" "capacity_${profile}" "$so" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    done
  done
done

python3 - <<'PY' "$OUTDIR" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
row_specs = sys.argv[2:]
rows = [spec.split()[0] for spec in row_specs]
log_re = re.compile(r"^(.+)_(selected|capacity_lite|capacity_hybrid|capacity_.+)_([0-9]+)\.log$")
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
fail_re = re.compile(r"\balloc_fail=([0-9]+)")

by_row = {}
for log in sorted((root / "logs").glob("*.log")):
    match = log_re.match(log.name)
    if not match:
        continue
    row, variant, _run = match.groups()
    text = log.read_text(errors="replace")
    ops = ops_re.search(text)
    peak = peak_re.search(text)
    if not ops or not peak:
        continue
    fail = fail_re.search(text)
    bucket = by_row.setdefault(row, {}).setdefault(
        variant, {"ops": [], "peak": [], "fail": []}
    )
    bucket["ops"].append(float(ops.group(1)))
    bucket["peak"].append(float(peak.group(1)) / 1024.0)
    bucket["fail"].append(float(fail.group(1)) if fail else 0.0)

def variant_key(name):
    order = {
        "selected": 0,
        "capacity_lite": 1,
        "capacity_hybrid": 2,
    }
    return (order.get(name, 3), name)

print("# HZ6 Workload Capacity Narrow Ladder\n")
print(f"root: `{root}`\n")
print("Rows are workload proxies over `bench_mixed_ws_crt`, not app benchmarks.\n")
print("| row | variant | median ops/s | median peak MiB | ops/s per MiB | median alloc_fail |")
print("| --- | --- | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in sorted(by_row.get(row, {}), key=variant_key):
        data = by_row[row][variant]
        ops = statistics.median(data["ops"])
        peak = statistics.median(data["peak"])
        fail = statistics.median(data["fail"])
        eff = ops / peak if peak else 0.0
        print(f"| `{row}` | `{variant}` | {ops:.3f} | {peak:.2f} | {eff:.3f} | {fail:.0f} |")
PY
