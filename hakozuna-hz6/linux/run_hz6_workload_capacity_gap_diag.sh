#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
ITERS="${ITERS:-20000}"
ROWS_CSV="${ROWS:-small_object_cache,mixed_small_cache,mixed_object_cache,wide_midpage_cache}"
ELASTIC_DESCRIPTOR_DEPOT_CAPACITY="${ELASTIC_DESCRIPTOR_DEPOT_CAPACITY:-2048}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_gap_diag_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_gap_diag.sh [options]

Options:
  --arch ARCH      target arch (default: x86_64)
  --iters N        iterations per diagnostic row (default: 20000)
  --elastic-desc N descriptor overflow depot capacity (default: 2048)
  --rows CSV       rows: small_object_cache,mixed_small_cache,mixed_object_cache,
                   wide_midpage_cache,all
  --outdir DIR     output directory
  --skip-builds    reuse diagnostic preload builds and benchmark
  --help           show this message

This diagnostic compares selected HZ6, selected plus elastic descriptor
overflow, and workload-capacity-lite using HZ6_PRELOAD_STATS=1 and
HZ6_DIAGNOSTIC_PROBES=1. It is for attributing the selected-vs-capacity-lite
workload proxy gap, not for throughput ranking.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --iters)
      ITERS="$2"
      shift 2
      ;;
    --elastic-desc)
      ELASTIC_DESCRIPTOR_DEPOT_CAPACITY="$2"
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

mkdir -p "$OUTDIR/build"

SELECTED_DIAG_SO="${OUTDIR}/build/selected_diag/libhakozuna_hz6_preload.so"
DESCRIPTOR_OVERFLOW_DIAG_SO="${OUTDIR}/build/descriptor_overflow_diag/libhakozuna_hz6_preload.so"
LITE_DIAG_SO="${OUTDIR}/build/capacity_lite_diag/libhakozuna_hz6_preload.so"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  OUT_DIR="${OUTDIR}/build/selected_diag" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_diag.sh"

  OUT_DIR="${OUTDIR}/build/descriptor_overflow_diag" \
    HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
    HZ6_EXTRA_CFLAGS="-DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1 -DHZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1=1 -DHZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY=${ELASTIC_DESCRIPTOR_DEPOT_CAPACITY}" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"

  OUT_DIR="${OUTDIR}/build/capacity_lite_diag" \
    HZ6_WORKLOAD_CAPACITY_LEVEL=lite \
    HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
    HZ6_EXTRA_CFLAGS="-DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_target.sh"

  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }
[[ -f "$SELECTED_DIAG_SO" ]] || { echo "missing selected diag DSO: $SELECTED_DIAG_SO" >&2; exit 2; }
[[ -f "$DESCRIPTOR_OVERFLOW_DIAG_SO" ]] || { echo "missing descriptor-overflow diag DSO: $DESCRIPTOR_OVERFLOW_DIAG_SO" >&2; exit 2; }
[[ -f "$LITE_DIAG_SO" ]] || { echo "missing capacity-lite diag DSO: $LITE_DIAG_SO" >&2; exit 2; }

rows=()
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    small_object_cache)
      rows+=("small_object_cache 4 ${ITERS} 8192 16 1024")
      ;;
    mixed_small_cache)
      rows+=("mixed_small_cache 4 ${ITERS} 8192 16 4096")
      ;;
    mixed_object_cache)
      rows+=("mixed_object_cache 4 ${ITERS} 8192 64 8192")
      ;;
    wide_midpage_cache)
      rows+=("wide_midpage_cache 4 ${ITERS} 8192 4096 32768")
      ;;
    all)
      rows+=(
        "small_object_cache 4 ${ITERS} 8192 16 1024"
        "mixed_small_cache 4 ${ITERS} 8192 16 4096"
        "mixed_object_cache 4 ${ITERS} 8192 64 8192"
        "wide_midpage_cache 4 ${ITERS} 8192 4096 32768"
      )
      ;;
    "")
      ;;
    *)
      echo "unknown row: ${row_group}" >&2
      exit 2
      ;;
  esac
done

{
  echo "arch=${ARCH}"
  echo "iters=${ITERS}"
  echo "elastic_descriptor_depot_capacity=${ELASTIC_DESCRIPTOR_DEPOT_CAPACITY}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH_BIN}"
  echo "selected_diag_so=${SELECTED_DIAG_SO}"
  echo "descriptor_overflow_diag_so=${DESCRIPTOR_OVERFLOW_DIAG_SO}"
  echo "capacity_lite_diag_so=${LITE_DIAG_SO}"
  for row_spec in "${rows[@]}"; do
    echo "row=${row_spec}"
  done
} > "${OUTDIR}/README.log"

run_case() {
  local row="$1"
  local allocator="$2"
  local so="$3"
  local threads="$4"
  local iters="$5"
  local ws="$6"
  local min_size="$7"
  local max_size="$8"
  local log="${OUTDIR}/${row}_${allocator}.log"
  env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$so" \
    "$BENCH_BIN" "$threads" "$iters" "$ws" "$min_size" "$max_size" \
    > "$log" 2>&1
}

for row_spec in "${rows[@]}"; do
  read -r row threads iters ws min_size max_size <<< "$row_spec"
  run_case "$row" selected "$SELECTED_DIAG_SO" "$threads" "$iters" "$ws" "$min_size" "$max_size"
  run_case "$row" descriptor_overflow "$DESCRIPTOR_OVERFLOW_DIAG_SO" "$threads" "$iters" "$ws" "$min_size" "$max_size"
  run_case "$row" capacity_lite "$LITE_DIAG_SO" "$threads" "$iters" "$ws" "$min_size" "$max_size"
done

python3 - <<'PY' "$OUTDIR" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
row_specs = sys.argv[2:]
rows = [spec.split()[0] for spec in row_specs]

ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
kv_re = re.compile(r"\b([A-Za-z0-9_]+)=([0-9]+)")

keys = [
    "alloc_fail",
    "descriptor_exhausted",
    "elastic_descriptor_overflow_alloc",
    "elastic_descriptor_overflow_reset",
    "elastic_descriptor_overflow_exhausted",
    "source_block_exhausted",
    "source_prefill_fallback",
    "malloc_real_fallback",
    "free_route_lookup_after_maps",
    "route_lookup_probe_total",
    "route_lookup_probe_max",
    "descriptor_live_max",
    "source_block_active_max",
    "frontcache_total_max",
    "active_source_blocks",
    "ref_zero_source_blocks",
    "static_table_bytes",
    "source_block_payload_bytes",
    "preload_attributed_bytes",
]

def parse_log(path):
    text = path.read_text(errors="replace")
    data = {}
    ops = ops_re.search(text)
    peak = peak_re.search(text)
    data["ops"] = float(ops.group(1)) if ops else 0.0
    data["peak_mib"] = float(peak.group(1)) / 1024.0 if peak else 0.0
    for line in text.splitlines():
        if not line.startswith("[HZ6_PRELOAD_"):
            continue
        for key, value in kv_re.findall(line):
            data[key] = int(value)
    return data

print("# HZ6 Workload Capacity Gap Diagnostic\n")
print(f"root: `{root}`\n")
print("| row | variant | ops/s | peak MiB | alloc_fail | desc_exh | elastic_alloc | elastic_reset | elastic_exh | source_exh | prefill_fb | real_fb | route_after_maps | route_probe_total | route_probe_max | desc_live_max | source_active_max | static MiB | payload MiB | attributed MiB |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in ("selected", "descriptor_overflow", "capacity_lite"):
        data = parse_log(root / f"{row}_{variant}.log")
        def get(key):
            return data.get(key, 0)
        def mib(key):
            return get(key) / (1024.0 * 1024.0)
        print(
            f"| `{row}` | `{variant}` | {data['ops']:.3f} | {data['peak_mib']:.2f} | "
            f"{get('alloc_fail')} | {get('descriptor_exhausted')} | "
            f"{get('elastic_descriptor_overflow_alloc')} | "
            f"{get('elastic_descriptor_overflow_reset')} | "
            f"{get('elastic_descriptor_overflow_exhausted')} | "
            f"{get('source_block_exhausted')} | {get('source_prefill_fallback')} | "
            f"{get('malloc_real_fallback')} | {get('free_route_lookup_after_maps')} | "
            f"{get('route_lookup_probe_total')} | {get('route_lookup_probe_max')} | "
            f"{get('descriptor_live_max')} | {get('source_block_active_max')} | "
            f"{mib('static_table_bytes'):.2f} | {mib('source_block_payload_bytes'):.2f} | "
            f"{mib('preload_attributed_bytes'):.2f} |"
        )
PY
