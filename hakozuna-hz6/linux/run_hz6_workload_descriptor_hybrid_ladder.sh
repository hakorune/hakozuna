#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-80000}"
PROFILES_CSV="${PROFILES:-desc12k_source1536_route48k,desc12k_source1536_route64k,desc16k_source1536_route64k,desc16k_source2048_route64k}"
DEPOT_CAPACITY="${DEPOT_CAPACITY:-2048}"
ROWS_CSV="${ROWS:-small_proxy,cache_proxy}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_descriptor_hybrid_ladder_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_descriptor_hybrid_ladder.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per row/variant (default: 3)
  --iters N          iterations per run (default: 80000)
  --profiles CSV     hybrid static-capacity profiles
  --depot N          elastic descriptor depot capacity (default: 2048)
  --rows CSV         row groups: small_proxy,cache_proxy,all
  --outdir DIR       output directory
  --skip-builds      reuse existing per-profile builds and benchmark
  --help             show this message

Profile names are desc<size>_source<size>_route<size>, for example:
  desc12k_source1536_route48k
  desc16k_source2048_route64k
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
    --depot)
      DEPOT_CAPACITY="$2"
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

parse_k_capacity() {
  local raw="$1"
  case "$raw" in
    *k)
      echo "$(( ${raw%k} * 1024 ))"
      ;;
    *)
      echo "$raw"
      ;;
  esac
}

parse_profile() {
  local profile="$1"
  local -n desc_out="$2"
  local -n source_out="$3"
  local -n route_out="$4"
  if [[ "$profile" =~ ^desc([0-9]+k?)_source([0-9]+k?)_route([0-9]+k?)$ ]]; then
    desc_out="$(parse_k_capacity "${BASH_REMATCH[1]}")"
    source_out="$(parse_k_capacity "${BASH_REMATCH[2]}")"
    route_out="$(parse_k_capacity "${BASH_REMATCH[3]}")"
  else
    echo "invalid profile: ${profile}" >&2
    return 2
  fi
}

build_hybrid_profile() {
  local profile="$1"
  local desc_capacity source_capacity route_capacity
  parse_profile "$profile" desc_capacity source_capacity route_capacity

  local flags=()
  hz6_preload_profile_selected_cflags flags 1
  hz6_preload_replace_define flags HZ6_ROUTE_TABLE_CAPACITY "$route_capacity"
  hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY "$desc_capacity"
  hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY "$source_capacity"
  hz6_preload_replace_define flags HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1 1
  hz6_preload_replace_define flags HZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY \
    "$DEPOT_CAPACITY"

  hz6_preload_profile_build flags "${OUTDIR}/build/${profile}"
}

mkdir -p "$OUTDIR/build" "$OUTDIR/logs"
IFS=',' read -r -a profiles <<< "$PROFILES_CSV"

SELECTED_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so"
DESC_OVERFLOW_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_descriptor_overflow_target/libhakozuna_hz6_preload.so"
LITE_SO="${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_workload_capacity_lite_target/libhakozuna_hz6_preload.so"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_descriptor_overflow_target.sh"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lite_target.sh"
  for profile in "${profiles[@]}"; do
    build_hybrid_profile "$profile"
  done
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$DESC_OVERFLOW_SO" ]] || { echo "missing descriptor-overflow DSO: $DESC_OVERFLOW_SO" >&2; exit 2; }
[[ -f "$LITE_SO" ]] || { echo "missing capacity-lite DSO: $LITE_SO" >&2; exit 2; }
for profile in "${profiles[@]}"; do
  so="${OUTDIR}/build/${profile}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing hybrid DSO: $so" >&2; exit 2; }
done

rows=()
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    small_proxy)
      rows+=(
        "redis_proxy 4 ${ITERS} 2000 16 256"
        "small_object_cache 4 ${ITERS} 8192 16 1024"
        "mixed_small_cache 4 ${ITERS} 8192 16 4096"
      )
      ;;
    cache_proxy)
      rows+=(
        "mixed_object_cache 4 ${ITERS} 8192 64 8192"
        "midpage_cache 4 ${ITERS} 4096 4096 32768"
        "wide_midpage_cache 4 ${ITERS} 8192 4096 32768"
      )
      ;;
    all)
      rows+=(
        "redis_proxy 4 ${ITERS} 2000 16 256"
        "small_object_cache 4 ${ITERS} 8192 16 1024"
        "mixed_small_cache 4 ${ITERS} 8192 16 4096"
        "mixed_object_cache 4 ${ITERS} 8192 64 8192"
        "midpage_cache 4 ${ITERS} 4096 4096 32768"
        "wide_midpage_cache 4 ${ITERS} 8192 4096 32768"
      )
      ;;
    "")
      ;;
    *)
      echo "unknown row group: ${row_group}" >&2
      exit 2
      ;;
  esac
done

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "profiles=${PROFILES_CSV}"
  echo "depot_capacity=${DEPOT_CAPACITY}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH_BIN}"
  echo "selected_so=${SELECTED_SO}"
  echo "descriptor_overflow_so=${DESC_OVERFLOW_SO}"
  echo "capacity_lite_so=${LITE_SO}"
  for profile in "${profiles[@]}"; do
    echo "hybrid_${profile}_so=${OUTDIR}/build/${profile}/libhakozuna_hz6_preload.so"
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
    run_one "$row" descriptor_overflow "$DESC_OVERFLOW_SO" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    run_one "$row" capacity_lite "$LITE_SO" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
    for profile in "${profiles[@]}"; do
      so="${OUTDIR}/build/${profile}/libhakozuna_hz6_preload.so"
      run_one "$row" "hybrid_${profile}" "$so" "$run" "$threads" "$iters" "$ws" "$min_size" "$max_size"
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
log_re = re.compile(r"^(.+)_(selected|descriptor_overflow|capacity_lite|hybrid_.+)_([0-9]+)\.log$")
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
    if name == "selected":
        return (0, "")
    if name == "descriptor_overflow":
        return (1, "")
    if name == "capacity_lite":
        return (3, "")
    return (2, name)

print("# HZ6 Workload Descriptor Hybrid Ladder\n")
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
