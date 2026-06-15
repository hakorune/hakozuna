#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-focused}"
VARIANTS_CSV="${VARIANTS:-selected,floor_prev,route65536,frontcache8192,route131072,desc32768,source4096,wide_l0}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_static_table_trim_ab_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0

source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_static_table_trim_ab.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per variant/row (default: 3)
  --iters N         iterations per run (default: 200000)
  --ws N            working set (default: 4096)
  --rows CSV        row groups: focused,fixed_mid (default: focused)
  --variants CSV    variants to build/run
  --outdir DIR      output directory
  --skip-bench      reuse existing benchmark binary
  --help            show this message

Variants:
  selected, floor_prev, route65536, route131072, desc8192_only, desc12288_only,
  desc32768, source1024_only, source4096, desc8192_source1024,
  desc12288_source1024,
  toy_map16384, midpage_map8192, frontcache8192, wide_l0
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
    --ws)
      WS="$2"
      shift 2
      ;;
    --rows)
      ROWS_CSV="$2"
      shift 2
      ;;
    --variants)
      VARIANTS_CSV="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-bench)
      SKIP_BENCH_BUILD=1
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

variant_flags() {
  local variant="$1"
  local flags=()
  hz6_preload_effective_selected_cflags flags 1
  hz6_preload_preserve_phase_counters flags
  case "$variant" in
    selected)
      ;;
    floor_prev)
      hz6_preload_replace_define flags HZ6_ROUTE_TABLE_CAPACITY 32768
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 16384
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 2048
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_BIN_CAPACITY 4096
      ;;
    frontcache8192)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_BIN_CAPACITY 8192
      ;;
    route65536)
      hz6_preload_replace_define flags HZ6_ROUTE_TABLE_CAPACITY 65536
      ;;
    route131072)
      hz6_preload_replace_define flags HZ6_ROUTE_TABLE_CAPACITY 131072
      ;;
    desc8192)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 8192
      ;;
    desc8192_only)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 8192
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 2048
      ;;
    desc12288)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 12288
      ;;
    desc12288_only)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 12288
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 2048
      ;;
    desc32768)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 32768
      ;;
    source1024)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 1024
      ;;
    source1024_only)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 16384
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 1024
      ;;
    source4096)
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 4096
      ;;
    desc8192_source1024)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 8192
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 1024
      ;;
    desc12288_source1024)
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 12288
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 1024
      ;;
    toy_map16384)
      hz6_preload_replace_define flags HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY 16384
      ;;
    midpage_map8192)
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY 8192
      ;;
    wide_l0)
      hz6_preload_replace_define flags HZ6_ROUTE_TABLE_CAPACITY 131072
      hz6_preload_replace_define flags HZ6_OBJECT_DESCRIPTOR_CAPACITY 32768
      hz6_preload_replace_define flags HZ6_SOURCE_BLOCK_CAPACITY 4096
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_BIN_CAPACITY 8192
      ;;
    *)
      echo "unknown variant: ${variant}" >&2
      exit 2
      ;;
  esac
  hz6_preload_join_flags "${flags[@]}"
}

build_variant() {
  local variant="$1"
  local out_dir="${OUTDIR}/build/${variant}"
  OUT_DIR="$out_dir" \
  HZ6_PRELOAD_DEFAULT_CFLAGS="$(variant_flags "$variant")" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/${variant}_build.log" 2>&1
}

run_once() {
  local variant="$1"
  local row="$2"
  local min_size="$3"
  local max_size="$4"
  local run_id="$5"
  local so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
  local log="${OUTDIR}/${row}/${run_id}_${variant}.log"
  mkdir -p "${OUTDIR}/${row}"
  env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$so" \
    "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
    > "$log" 2>&1
}

if [[ "$SKIP_BENCH_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }

mkdir -p "$OUTDIR"
{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "rows=${ROWS_CSV}"
  echo "variants=${VARIANTS_CSV}"
  echo "bench=${BENCH}"
} > "${OUTDIR}/README.log"

IFS=',' read -r -a variants <<< "$VARIANTS_CSV"
for variant in "${variants[@]}"; do
  build_variant "$variant"
done

rows=()
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    focused)
      rows+=(
        "16_4096 16 4096"
        "1024_4096 1024 4096"
        "4096_16384 4096 16384"
      )
      ;;
    fixed_mid)
      rows+=(
        "fixed_4k 4096 4096"
        "fixed_8k 8192 8192"
        "fixed_16k 16384 16384"
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

for row_spec in "${rows[@]}"; do
  read -r row min_size max_size <<< "$row_spec"
  for variant in "${variants[@]}"; do
    for ((run = 1; run <= RUNS; ++run)); do
      run_once "$variant" "$row" "$min_size" "$max_size" "$run"
    done
  done
done

python3 - <<'PY' "$OUTDIR" "${ROWS_CSV}" "${VARIANTS_CSV}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
rows_csv = sys.argv[2]
variants_csv = sys.argv[3]
rows = []
for group in [part for part in rows_csv.split(",") if part]:
    if group == "focused":
        rows.extend(["16_4096", "1024_4096", "4096_16384"])
    elif group == "fixed_mid":
        rows.extend(["fixed_4k", "fixed_8k", "fixed_16k"])
    else:
        raise SystemExit(f"unknown row group: {group}")
variants = [part for part in variants_csv.split(",") if part]
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
kv_re = re.compile(r"([A-Za-z0-9_]+)=([0-9]+)")

def parse_stats(text):
    out = {}
    for line in text.splitlines():
        if line.startswith("[HZ6_PRELOAD_STATS]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
        elif line.startswith("[HZ6_PRELOAD_ROUTE_DETAIL]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
        elif line.startswith("[HZ6_PRELOAD_FRONT_DETAIL]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
        elif line.startswith("[HZ6_PRELOAD_PHASE_STATS]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
    return out

print("# HZ6 Static Table Trim A/B\n")
print(f"root: `{root}`\n")
print("| row | variant | median ops/s | median peak MiB | route_fail | desc_exh | source_exh | front_overflow | real_fallback | route_valid | source_alloc |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in variants:
        ops_values = []
        peak_values = []
        route_fail = 0
        desc_exh = 0
        source_exh = 0
        front_overflow = 0
        real_fallback = 0
        route_valid = 0
        source_alloc = 0
        for log in sorted((root / row).glob(f"*_{variant}.log")):
            text = log.read_text(errors="replace")
            ops = ops_re.search(text)
            peak = peak_re.search(text)
            if ops:
                ops_values.append(float(ops.group(1)))
            if peak:
                peak_values.append(int(peak.group(1)) / 1024.0)
            stats = parse_stats(text)
            route_fail += stats.get("route_register_fail", 0)
            desc_exh += stats.get("descriptor_exhausted", 0)
            source_exh += stats.get("source_block_exhausted", 0)
            front_overflow += stats.get("route_unregister_reason_frontcache_overflow", 0)
            real_fallback += stats.get("malloc_real_fallback", 0)
            route_valid += stats.get("route_valid", 0)
            source_alloc += stats.get("source_alloc", 0)
        median_ops = statistics.median(ops_values) if ops_values else 0.0
        median_peak = statistics.median(peak_values) if peak_values else 0.0
        print(
            f"| `{row}` | `{variant}` | {median_ops:.3f} | {median_peak:.2f} | "
            f"{route_fail} | {desc_exh} | {source_exh} | {front_overflow} | "
            f"{real_fallback} | {route_valid} | {source_alloc} |"
        )
PY
