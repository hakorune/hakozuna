#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
WS="${WS:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_frontcache_shape_ab_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0

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
  flags+=("-DHZ6_DIAGNOSTIC_PROBES=1")
  case "$variant" in
    selected)
      ;;
    mid32k_cap3072)
      flags+=("-DHZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=3072")
      ;;
    mid32k_cap2560)
      flags+=("-DHZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=2560")
      ;;
    mid32k_cap2048)
      flags+=("-DHZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=2048")
      ;;
    midpage_cap3072)
      flags+=("-DHZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY=3072")
      flags+=("-DHZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=3072")
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
  echo "bench=${BENCH}"
} > "${OUTDIR}/README.log"

variants=(selected mid32k_cap3072 mid32k_cap2560 mid32k_cap2048 midpage_cap3072)
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
rows = ["16_256", "16_4096", "1024_4096", "4096_16384"]
variants = ["selected", "mid32k_cap3072", "mid32k_cap2560", "mid32k_cap2048", "midpage_cap3072"]
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
    return out

def median(values):
    return statistics.median(values) if values else None

print("# HZ6 Frontcache Shape A/B")
print()
print("| row | variant | med ops/s | med peak KB | alloc_fail | route_register_fail | source_block_exhausted | malloc_real_fallback | c4_max | c5_max | c4_empty | c5_empty |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
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
            f"{merged['alloc_fail']} | {merged['route_register_fail']} | "
            f"{merged['source_block_exhausted']} | {merged['malloc_real_fallback']} | "
            f"{merged['c4_max']} | {merged['c5_max']} | "
            f"{merged['c4_empty']} | {merged['c5_empty']} |"
        )
PY

echo "[linux][hz6] frontcache shape A/B output: ${OUTDIR}"
