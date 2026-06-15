#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-5}"
ITERS="${ITERS:-500000}"
WS="${WS:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_midpage_supply_map_ab_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0
ENABLE_STATS="${ENABLE_STATS:-1}"
ENABLE_DIAGNOSTICS="${ENABLE_DIAGNOSTICS:-1}"
VARIANTS="${VARIANTS:-}"
INCLUDE_TINY="${INCLUDE_TINY:-0}"

source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_midpage_supply_map_ab.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per variant/row (default: 5)
  --iters N         iterations per run (default: 500000)
  --ws N            working set (default: 4096)
  --outdir DIR      output directory
  --skip-bench      reuse existing benchmark binary
  --stats           enable HZ6_PRELOAD_STATS during runs (default)
  --no-stats        disable HZ6_PRELOAD_STATS for speed/RSS ranking
  --diagnostics     build with HZ6_DIAGNOSTIC_PROBES=1 (default)
  --no-diagnostics  build without diagnostic probe counters
  --variants LIST   comma-separated variants to run
  --include-tiny    also run the 16..256 tiny guard row
  --help            show this message
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
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-bench)
      SKIP_BENCH_BUILD=1
      shift
      ;;
    --stats)
      ENABLE_STATS=1
      shift
      ;;
    --no-stats)
      ENABLE_STATS=0
      shift
      ;;
    --diagnostics)
      ENABLE_DIAGNOSTICS=1
      shift
      ;;
    --no-diagnostics)
      ENABLE_DIAGNOSTICS=0
      shift
      ;;
    --variants)
      VARIANTS="$2"
      shift 2
      ;;
    --include-tiny)
      INCLUDE_TINY=1
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
  if [[ "$ENABLE_DIAGNOSTICS" -ne 0 ]]; then
    flags+=("-DHZ6_DIAGNOSTIC_PROBES=1")
  fi
  case "$variant" in
    selected)
      ;;
    run8_384k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 393216
      ;;
    run8_512k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 524288
      ;;
    run8_640k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 655360
      ;;
    run8_768k)
      hz6_preload_replace_define flags HZ6_MIDPAGE_RUN_BYTES 786432
      ;;
    lowwater)
      hz6_preload_replace_define flags HZ6_MIDPAGE_LOW_WATER_REFILL_L1 1
      ;;
    lowwater_8k256_32k128)
      hz6_preload_replace_define flags HZ6_MIDPAGE_LOW_WATER_REFILL_L1 1
      hz6_preload_replace_define flags HZ6_MIDPAGE_8K_LOW_WATER_REFILL 256
      hz6_preload_replace_define flags HZ6_MIDPAGE_32K_LOW_WATER_REFILL 128
      ;;
    prefill_cache_only)
      hz6_preload_replace_define flags HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1 1
      ;;
    sourcerun)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      ;;
    sourcerun_sameclass)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1 1
      ;;
    sourcerun_reclaim)
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1 1
      ;;
    borrow_larger)
      hz6_preload_replace_define flags HZ6_FRONTCACHE_BORROW_LARGER_ON_CLASS_MISS 1
      ;;
    mid8_borrow32)
      hz6_preload_replace_define flags HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1 1
      ;;
    amap_mask)
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1 1
      ;;
    amap32k_p4)
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY 32768
      ;;
    amap64k_p4)
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY 65536
      ;;
    amap32k_p8)
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY 32768
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT 8
      ;;
    amap64k_p8)
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY 65536
      hz6_preload_replace_define flags HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT 8
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
  if [[ "$ENABLE_STATS" -ne 0 ]]; then
    env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$so" \
      "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
      > "$log" 2>&1
  else
    env LD_PRELOAD="$so" \
      "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
      > "$log" 2>&1
  fi
}

if [[ "$SKIP_BENCH_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }

default_variants=(
  selected
  run8_384k
  run8_512k
  run8_640k
  run8_768k
  lowwater
  lowwater_8k256_32k128
  prefill_cache_only
  sourcerun
  sourcerun_sameclass
  sourcerun_reclaim
  borrow_larger
  mid8_borrow32
  amap_mask
  amap32k_p4
  amap64k_p4
  amap32k_p8
  amap64k_p8
)
if [[ -n "$VARIANTS" ]]; then
  IFS=',' read -r -a variants <<< "$VARIANTS"
else
  variants=("${default_variants[@]}")
fi

mkdir -p "$OUTDIR"
{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "stats=${ENABLE_STATS}"
  echo "diagnostics=${ENABLE_DIAGNOSTICS}"
  echo "variants=${variants[*]}"
  echo "include_tiny=${INCLUDE_TINY}"
} > "${OUTDIR}/config.txt"

rows=(
  "16_4096 16 4096"
  "1024_4096 1024 4096"
  "4096_16384 4096 16384"
)
if [[ "$INCLUDE_TINY" -ne 0 ]]; then
  rows=("16_256 16 256" "${rows[@]}")
fi

for variant in "${variants[@]}"; do
  build_variant "$variant"
done

for row_entry in "${rows[@]}"; do
  read -r row min_size max_size <<< "$row_entry"
  for run_id in $(seq 1 "$RUNS"); do
    for variant in "${variants[@]}"; do
      run_once "$variant" "$row" "$min_size" "$max_size" "$run_id"
    done
  done
done

python3 - "$OUTDIR" "${variants[*]}" "${rows[*]}" <<'PY' | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

outdir = pathlib.Path(sys.argv[1])
variants = sys.argv[2].split()
rows_blob = sys.argv[3].split()
rows = [rows_blob[i] for i in range(0, len(rows_blob), 3)]
config = (outdir / "config.txt").read_text(errors="replace")

def extract(name, text):
    match = re.search(r"(?:^|\s)" + re.escape(name) + r"=([0-9]+)", text)
    return int(match.group(1)) if match else 0

stats_mode = config.split("stats=", 1)[1].splitlines()[0] if "stats=" in config else "unknown"
diag_mode = config.split("diagnostics=", 1)[1].splitlines()[0] if "diagnostics=" in config else "unknown"
print(f"stats: `{stats_mode}`")
print(f"diagnostics: `{diag_mode}`")
print()
print("| row | variant | median ops/s | median peak MiB | source_alloc | midpage_source_alloc | route_after_maps | mid_hit | mid_miss | mid8_alloc | mid32_alloc | mid8_empty | mid32_empty | borrow_success | fail |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in variants:
        ops = []
        peak = []
        source_alloc = []
        midpage_source_alloc = []
        route_after_maps = []
        mid_hit = []
        mid_miss = []
        mid8_alloc = []
        mid32_alloc = []
        mid8_empty = []
        mid32_empty = []
        borrow_success = []
        fail = 0
        for log in sorted((outdir / row).glob(f"*_{variant}.log")):
            text = log.read_text()
            ops_match = re.search(r"ops/s=([0-9.]+)", text)
            peak_match = re.search(r"peak_kb=([0-9]+)", text)
            if ops_match:
                ops.append(float(ops_match.group(1)))
            if peak_match:
                peak.append(int(peak_match.group(1)) / 1024.0)
            source_alloc.append(extract("source_alloc", text))
            midpage_source_alloc.append(extract("midpage_source_alloc", text))
            route_after_maps.append(extract("free_route_lookup_after_maps", text))
            mid_hit.append(extract("free_midpage_active_map_hit", text))
            mid_miss.append(extract("free_midpage_active_map_miss", text))
            mid8_alloc.append(extract("midpage_8k_alloc_call", text))
            mid32_alloc.append(extract("midpage_32k_alloc_call", text))
            mid8_empty.append(extract("midpage_8k_frontcache_pop_empty", text))
            mid32_empty.append(extract("midpage_32k_frontcache_pop_empty", text))
            borrow_success.append(extract("frontcache_borrow_success", text))
            fail += extract("route_miss", text)
            fail += extract("route_invalid", text)
            fail += extract("alloc_fail", text)
            fail += extract("route_register_fail", text)
        print(
            f"| {row} | {variant} | "
            f"{statistics.median(ops) / 1e6:.3f}M | "
            f"{statistics.median(peak):.2f} | "
            f"{statistics.median(source_alloc):.0f} | "
            f"{statistics.median(midpage_source_alloc):.0f} | "
            f"{statistics.median(route_after_maps):.0f} | "
            f"{statistics.median(mid_hit):.0f} | "
            f"{statistics.median(mid_miss):.0f} | "
            f"{statistics.median(mid8_alloc):.0f} | "
            f"{statistics.median(mid32_alloc):.0f} | "
            f"{statistics.median(mid8_empty):.0f} | "
            f"{statistics.median(mid32_empty):.0f} | "
            f"{statistics.median(borrow_success):.0f} | "
            f"{fail} |"
        )
PY

echo "[linux][hz6] raw results: ${OUTDIR}"
