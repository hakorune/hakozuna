#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-80000}"
ROWS_CSV="${ROWS:-small_proxy,cache_proxy}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_narrow_map_ladder_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"
source "${HZ6_DIR}/linux/hz6_workload_profile_ladder_common.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_narrow_map_ladder.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per row/variant (default: 3)
  --iters N          iterations per run (default: 80000)
  --rows CSV         row groups: small_proxy,cache_proxy,all
  --outdir DIR       output directory
  --skip-builds      reuse existing per-variant builds and benchmark
  --help             show this message

This runner keeps workload-capacity-narrow static tables
(route40K/descriptors10K/source1280) and tests Toy active-map RSS variants.
It is runner-only evidence until a variant survives a broad guard.
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

apply_capacity_narrow_flags() {
  local flags_name="$1"
  hz6_preload_profile_selected_cflags "$flags_name" 1
  hz6_preload_replace_define "$flags_name" HZ6_ROUTE_TABLE_CAPACITY 40960
  hz6_preload_replace_define "$flags_name" HZ6_OBJECT_DESCRIPTOR_CAPACITY 10240
  hz6_preload_replace_define "$flags_name" HZ6_SOURCE_BLOCK_CAPACITY 1280
}

build_variant() {
  local variant="$1"
  local flags=()
  apply_capacity_narrow_flags flags

  case "$variant" in
    capacity_narrow)
      ;;
    capacity_narrow_toy_map8192)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY 8192
      ;;
    capacity_narrow_toy_map8192_external)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY 8192
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1 1
      ;;
    *)
      echo "unknown variant: ${variant}" >&2
      return 2
      ;;
  esac

  hz6_preload_profile_build flags "${OUTDIR}/build/${variant}"
}

mkdir -p "$OUTDIR/build" "$OUTDIR/logs"

variants=(
  capacity_narrow
  capacity_narrow_toy_map8192
  capacity_narrow_toy_map8192_external
)

SELECTED_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so"
HYBRID_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_descriptor_hybrid_target/libhakozuna_hz6_preload.so"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_descriptor_hybrid_target.sh"
  for variant in "${variants[@]}"; do
    build_variant "$variant"
  done
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$HYBRID_SO" ]] || { echo "missing hybrid DSO: $HYBRID_SO" >&2; exit 2; }
for variant in "${variants[@]}"; do
  so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing variant DSO: $so" >&2; exit 2; }
done

rows=()
hz6_workload_append_proxy_rows rows "$ITERS" "$ROWS_CSV"

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH_BIN}"
  echo "selected_so=${SELECTED_SO}"
  echo "descriptor_hybrid_so=${HYBRID_SO}"
  for variant in "${variants[@]}"; do
    echo "${variant}_so=${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
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
    run_one "$row" descriptor_hybrid "$HYBRID_SO" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    for variant in "${variants[@]}"; do
      so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
      run_one "$row" "$variant" "$so" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
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
log_re = re.compile(r"^(.+)_(selected|descriptor_hybrid|capacity_narrow(?:_toy_map8192(?:_external)?)?)_([0-9]+)\.log$")
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
        "descriptor_hybrid": 1,
        "capacity_narrow": 2,
        "capacity_narrow_toy_map8192": 3,
        "capacity_narrow_toy_map8192_external": 4,
    }
    return (order.get(name, 9), name)

print("# HZ6 Workload Capacity Narrow Map Ladder\n")
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
