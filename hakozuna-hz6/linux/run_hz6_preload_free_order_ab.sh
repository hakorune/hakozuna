#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-5}"
ITERS="${ITERS:-500000}"
WS="${WS:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_preload_free_order_ab_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0

source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_free_order_ab.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per variant/row (default: 5)
  --iters N         iterations per run (default: 500000)
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
    midpage_first)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1 1
      ;;
    aligned_first)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1 1
      ;;
    current_bias)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 1
      ;;
    current_bias_2x)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR 2
      ;;
    current_bias_4x)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR 4
      ;;
    current_bias_delta64)
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 1
      hz6_preload_replace_define flags HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DELTA 64
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
} > "${OUTDIR}/config.txt"

variants=(
  selected
  midpage_first
  aligned_first
  current_bias
  current_bias_2x
  current_bias_4x
  current_bias_delta64
)
rows=(
  "16_256 16 256"
  "16_4096 16 4096"
  "1024_4096 1024 4096"
  "4096_16384 4096 16384"
)

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

def extract(name, text):
    match = re.search(r"(?:^|\s)" + re.escape(name) + r"=([0-9]+)", text)
    return int(match.group(1)) if match else 0

print("| row | variant | median ops/s | median peak MiB | toy_attempt | toy_hit | mid_attempt | mid_hit | route_after_maps | fail |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    for variant in variants:
        ops = []
        peak = []
        toy_attempt = []
        toy_hit = []
        mid_attempt = []
        mid_hit = []
        route_after_maps = []
        fail = 0
        for log in sorted((outdir / row).glob(f"*_{variant}.log")):
            text = log.read_text()
            ops_match = re.search(r"ops/s=([0-9.]+)", text)
            peak_match = re.search(r"peak_kb=([0-9]+)", text)
            if ops_match:
                ops.append(float(ops_match.group(1)))
            if peak_match:
                peak.append(int(peak_match.group(1)) / 1024.0)
            toy_attempt.append(extract("free_toy_active_map_attempt", text))
            toy_hit.append(extract("free_toy_active_map_hit", text))
            mid_attempt.append(extract("free_midpage_active_map_attempt", text))
            mid_hit.append(extract("free_midpage_active_map_hit", text))
            route_after_maps.append(extract("free_route_lookup_after_maps", text))
            fail += extract("route_miss", text)
            fail += extract("route_invalid", text)
            fail += extract("alloc_fail", text)
            fail += extract("route_register_fail", text)
        print(
            f"| {row} | {variant} | "
            f"{statistics.median(ops) / 1e6:.3f}M | "
            f"{statistics.median(peak):.2f} | "
            f"{statistics.median(toy_attempt):.0f} | "
            f"{statistics.median(toy_hit):.0f} | "
            f"{statistics.median(mid_attempt):.0f} | "
            f"{statistics.median(mid_hit):.0f} | "
            f"{statistics.median(route_after_maps):.0f} | "
            f"{fail} |"
        )
PY

echo "[linux][hz6] raw results: ${OUTDIR}"
