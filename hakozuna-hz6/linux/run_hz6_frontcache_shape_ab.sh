#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
WS="${WS:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_frontcache_shape_ab_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0
ENABLE_STATS="${ENABLE_STATS:-1}"
ENABLE_DIAGNOSTICS="${ENABLE_DIAGNOSTICS:-1}"
VARIANTS="${VARIANTS:-}"

source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_frontcache_shape_ab.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per variant/row (default: 3)
  --iters N         iterations per run (default: 200000)
  --ws N            working set (default: 4096)
  --outdir DIR      output directory
  --skip-bench      reuse existing benchmark binary
  --stats           enable HZ6_PRELOAD_STATS during runs (default)
  --no-stats        disable HZ6_PRELOAD_STATS for speed/RSS ranking
  --diagnostics     build with HZ6_DIAGNOSTIC_PROBES=1 (default)
  --no-diagnostics  build without diagnostic probe counters
  --variants LIST   comma-separated variants to run
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
  hz6_preload_preserve_phase_counters_when_observing flags \
    "$ENABLE_STATS" "$ENABLE_DIAGNOSTICS"
  if [[ "$ENABLE_DIAGNOSTICS" -ne 0 ]]; then
    flags+=("-DHZ6_DIAGNOSTIC_PROBES=1")
  fi
  case "$variant" in
    selected)
      ;;
    mid32k_cap3072)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY 3072
      ;;
    mid32k_cap2560)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY 2560
      ;;
    mid32k_cap2048)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY 2048
      ;;
    midpage_cap3072)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY 3072
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY 3072
      ;;
    storage_trim)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      ;;
    storage_trim_cold32)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY 32
      ;;
    storage_trim_cold16)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY 16
      ;;
    storage_trim_c1_512_c3_512_cold32)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY 512
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY 512
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY 32
      ;;
    storage_trim_c0_64_c1_512_c3_512_cold32)
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1 1
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS0_STORAGE_CAPACITY 64
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY 512
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY 512
      hz6_preload_replace_define flags HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY 32
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

mkdir -p "$OUTDIR"
{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "bench=${BENCH}"
  echo "stats=${ENABLE_STATS}"
  echo "diagnostics=${ENABLE_DIAGNOSTICS}"
  echo "variants=${VARIANTS}"
} > "${OUTDIR}/README.log"

variants=(selected mid32k_cap3072 mid32k_cap2560 mid32k_cap2048 midpage_cap3072 storage_trim)
if [[ -n "$VARIANTS" ]]; then
  IFS=',' read -r -a variants <<< "$VARIANTS"
fi
for variant in "${variants[@]}"; do
  build_variant "$variant"
done

rows=(
  "16_256 16 256"
  "16_4096 16 4096"
  "1024_4096 1024 4096"
  "4096_16384 4096 16384"
)

for row_spec in "${rows[@]}"; do
  read -r row min_size max_size <<< "$row_spec"
  for variant in "${variants[@]}"; do
    for ((run = 1; run <= RUNS; ++run)); do
      run_once "$variant" "$row" "$min_size" "$max_size" "$run"
    done
  done
done

python3 - <<'PY' "$OUTDIR" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
readme = (root / "README.log").read_text(errors="replace")
rows = ["16_256", "16_4096", "1024_4096", "4096_16384"]
variants_line = ""
if "variants=" in readme:
    variants_line = readme.split("variants=", 1)[1].splitlines()[0]
if variants_line:
    variants = [v.strip() for v in variants_line.split(",") if v.strip()]
else:
    variants = [
        "selected",
        "mid32k_cap3072",
        "mid32k_cap2560",
        "mid32k_cap2048",
        "midpage_cap3072",
        "storage_trim",
    ]
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
kv_re = re.compile(r"([A-Za-z0-9_]+)=([0-9]+)")

def parse_stats(text):
    out = {}
    for line in text.splitlines():
        if line.startswith("[HZ6_PRELOAD_STATS]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
        elif line.startswith("[HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
        elif line.startswith("[HZ6_PRELOAD_PHASE_STATS]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
        elif line.startswith("[HZ6_PRELOAD_MEMORY_ATTR]"):
            out.update({k: int(v) for k, v in kv_re.findall(line)})
    return out

def median(values):
    return statistics.median(values) if values else None

print("# HZ6 Frontcache Shape A/B")
print()
stats_mode = readme.split("stats=", 1)[1].splitlines()[0] if "stats=" in readme else "unknown"
print(f"stats: `{stats_mode}`")
diagnostics_mode = readme.split("diagnostics=", 1)[1].splitlines()[0] if "diagnostics=" in readme else "unknown"
print(f"diagnostics: `{diagnostics_mode}`")
print()
print("| row | variant | med ops/s | med peak KB | frontcache table KB | static table KB | alloc_fail | route_register_fail | source_block_exhausted | malloc_real_fallback | c4_max | c5_max | c4_empty | c5_empty |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in variants:
        ops = []
        peaks = []
        stats_list = []
        for path in sorted((root / row).glob(f"*_{variant}.log")):
            text = path.read_text(errors="replace")
            m = ops_re.search(text)
            p = peak_re.search(text)
            if m:
                ops.append(float(m.group(1)))
            if p:
                peaks.append(int(p.group(1)))
            stats_list.append(parse_stats(text))
        merged = {}
        for key in [
            "alloc_fail",
            "route_register_fail",
            "source_block_exhausted",
            "malloc_real_fallback",
            "frontcache_table_bytes",
            "static_table_bytes",
            "c4_max",
            "c5_max",
            "c4_empty",
            "c5_empty",
        ]:
            values = [s.get(key, 0) for s in stats_list]
            merged[key] = max(values) if values else 0
        med_ops = median(ops)
        med_peak = median(peaks)
        print(
            f"| {row} | {variant} | "
            f"{med_ops:.3f} | {med_peak:.0f} | "
            f"{merged['frontcache_table_bytes'] / 1024:.0f} | "
            f"{merged['static_table_bytes'] / 1024:.0f} | "
            f"{merged['alloc_fail']} | {merged['route_register_fail']} | "
            f"{merged['source_block_exhausted']} | {merged['malloc_real_fallback']} | "
            f"{merged['c4_max']} | {merged['c5_max']} | "
            f"{merged['c4_empty']} | {merged['c5_empty']} |"
        )
PY

echo "[linux][hz6] frontcache shape A/B output: ${OUTDIR}"
