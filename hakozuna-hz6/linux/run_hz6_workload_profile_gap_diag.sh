#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
ARCH="${ARCH:-x86_64}"
ITERS="${ITERS:-20000}"
ROWS_CSV="${ROWS:-small_object_cache,mixed_small_cache,mixed_object_cache,wide_midpage_cache}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_profile_gap_diag_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
HYBRID_VARIANT_LABEL="${HZ6_WORKLOAD_PROFILE_GAP_HYBRID_LABEL:-descriptor_hybrid}"
HYBRID_DISPLAY_NAME="${HZ6_WORKLOAD_PROFILE_GAP_HYBRID_DISPLAY_NAME:-descriptor-hybrid}"
HYBRID_BUILD_DIR="${HZ6_WORKLOAD_PROFILE_GAP_HYBRID_BUILD_DIR:-descriptor_hybrid_diag}"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"
source "${HZ6_DIR}/linux/hz6_workload_profile_ladder_common.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_profile_gap_diag.sh [options]

Options:
  --arch ARCH      target arch (default: x86_64)
  --iters N        iterations per diagnostic row (default: 20000)
  --rows CSV       rows: small_object_cache,mixed_small_cache,mixed_object_cache,
                   wide_midpage_cache,small_ws16384,object_ws16384,
                   mixed_ws16384,midpage_ws16384,cliff16384,all
  --outdir DIR     output directory
  --skip-builds    reuse diagnostic preload builds and benchmark
  --help           show this message

This diagnostic compares selected HZ6, workload-capacity-narrow, and the
configured hybrid workload profile with HZ6_PRELOAD_STATS=1 and
HZ6_DIAGNOSTIC_PROBES=1.
It is for attribution, not throughput ranking.
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
CAPACITY_NARROW_DIAG_SO="${OUTDIR}/build/capacity_narrow_diag/libhakozuna_hz6_preload.so"
HYBRID_DIAG_SO="${OUTDIR}/build/${HYBRID_BUILD_DIR}/libhakozuna_hz6_preload.so"
DIAG_CFLAGS="-DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1"

apply_capacity_narrow_flags() {
  local flags_name="$1"
  hz6_preload_profile_selected_cflags "$flags_name" 1
  hz6_preload_replace_define "$flags_name" HZ6_ROUTE_TABLE_CAPACITY 40960
  hz6_preload_replace_define "$flags_name" HZ6_OBJECT_DESCRIPTOR_CAPACITY 10240
  hz6_preload_replace_define "$flags_name" HZ6_SOURCE_BLOCK_CAPACITY 1280
}

build_capacity_narrow_diag() {
  local flags=()
  apply_capacity_narrow_flags flags
  HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
  HZ6_EXTRA_CFLAGS="$DIAG_CFLAGS" \
    hz6_preload_profile_build flags "${OUTDIR}/build/capacity_narrow_diag"
}

build_hybrid_diag() {
  local flags=()
  apply_capacity_narrow_flags flags
  hz6_preload_replace_define flags HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1 1
  hz6_preload_replace_define flags HZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY \
    "${HZ6_WORKLOAD_DESCRIPTOR_HYBRID_DEPOT_CAPACITY:-1024}"
  HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
  HZ6_EXTRA_CFLAGS="$DIAG_CFLAGS" \
    hz6_preload_profile_build flags "${OUTDIR}/build/${HYBRID_BUILD_DIR}"
}

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  OUT_DIR="${OUTDIR}/build/selected_diag" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_diag.sh"
  build_capacity_narrow_diag
  build_hybrid_diag
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }
[[ -f "$SELECTED_DIAG_SO" ]] || { echo "missing selected diag DSO: $SELECTED_DIAG_SO" >&2; exit 2; }
[[ -f "$CAPACITY_NARROW_DIAG_SO" ]] || { echo "missing capacity-narrow diag DSO: $CAPACITY_NARROW_DIAG_SO" >&2; exit 2; }
[[ -f "$HYBRID_DIAG_SO" ]] || { echo "missing ${HYBRID_DISPLAY_NAME} diag DSO: $HYBRID_DIAG_SO" >&2; exit 2; }

rows=()
hz6_workload_append_gap_diag_rows rows "$ITERS" "$ROWS_CSV"

{
  echo "arch=${ARCH}"
  echo "iters=${ITERS}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH_BIN}"
  echo "selected_diag_so=${SELECTED_DIAG_SO}"
  echo "capacity_narrow_diag_so=${CAPACITY_NARROW_DIAG_SO}"
  echo "hybrid_variant=${HYBRID_VARIANT_LABEL}"
  echo "hybrid_display=${HYBRID_DISPLAY_NAME}"
  echo "hybrid_diag_so=${HYBRID_DIAG_SO}"
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
  run_case "$row" capacity_narrow "$CAPACITY_NARROW_DIAG_SO" "$threads" "$iters" "$ws" "$min_size" "$max_size"
  run_case "$row" "$HYBRID_VARIANT_LABEL" "$HYBRID_DIAG_SO" "$threads" "$iters" "$ws" "$min_size" "$max_size"
done

python3 - <<'PY' "$OUTDIR" "$HYBRID_VARIANT_LABEL" "$HYBRID_DISPLAY_NAME" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
hybrid_variant = sys.argv[2]
hybrid_display = sys.argv[3]
row_specs = sys.argv[4:]
rows = [spec.split()[0] for spec in row_specs]

ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
kv_re = re.compile(r"\b([A-Za-z0-9_]+)=([0-9]+)")

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

print("# HZ6 Workload Profile Gap Diagnostic\n")
print(f"root: `{root}`\n")
print("| row | variant | ops/s | peak MiB | alloc_fail | desc_exh | elastic_alloc | elastic_reset | elastic_exh | source_exh | prefill_fb | real_fb | route_after_maps | route_probe_total | route_probe_max | toy_hit_pct | mid_hit_pct | desc_live_max | source_active_max | frontcache_max | active_source_blocks | static MiB | payload MiB | attributed MiB |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in ("selected", "capacity_narrow", hybrid_variant):
        data = parse_log(root / f"{row}_{variant}.log")
        display_variant = hybrid_display if variant == hybrid_variant else variant
        def get(key):
            return data.get(key, 0)
        def mib(key):
            return get(key) / (1024.0 * 1024.0)
        def hit_pct(prefix):
            attempts = get(f"{prefix}_attempt")
            hits = get(f"{prefix}_hit")
            return (100.0 * hits / attempts) if attempts else 0.0
        print(
            f"| `{row}` | `{display_variant}` | {data['ops']:.3f} | {data['peak_mib']:.2f} | "
            f"{get('alloc_fail')} | {get('descriptor_exhausted')} | "
            f"{get('elastic_descriptor_overflow_alloc')} | "
            f"{get('elastic_descriptor_overflow_reset')} | "
            f"{get('elastic_descriptor_overflow_exhausted')} | "
            f"{get('source_block_exhausted')} | {get('source_prefill_fallback')} | "
            f"{get('malloc_real_fallback')} | {get('free_route_lookup_after_maps')} | "
            f"{get('route_lookup_probe_total')} | {get('route_lookup_probe_max')} | "
            f"{hit_pct('free_toy_active_map'):.2f} | "
            f"{hit_pct('free_midpage_active_map'):.2f} | "
            f"{get('descriptor_live_max')} | {get('source_block_active_max')} | "
            f"{get('frontcache_total_max')} | {get('active_source_blocks')} | "
            f"{mib('static_table_bytes'):.2f} | {mib('source_block_payload_bytes'):.2f} | "
            f"{mib('preload_attributed_bytes'):.2f} |"
        )
PY
