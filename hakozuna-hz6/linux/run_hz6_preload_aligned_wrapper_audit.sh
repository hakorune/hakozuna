#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-2000}"
RUNS="${RUNS:-3}"
ROWS_CSV="${ROWS:-posix64_2k,aligned4k_4k,posix8k_64k}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_preload_aligned_wrapper_audit_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILD=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_aligned_wrapper_audit.sh [options]

Options:
  --arch ARCH     target arch (default: x86_64)
  --threads N     worker threads (default: 4)
  --iters N       iterations per thread (default: 2000)
  --runs N        repeat count (default: 3)
  --rows CSV      rows: posix64_2k,aligned4k_4k,posix8k_64k
  --outdir DIR    output directory
  --skip-build    reuse existing benchmark and preload DSOs
  --help          show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --threads)
      THREADS="$2"
      shift 2
      ;;
    --iters)
      ITERS="$2"
      shift 2
      ;;
    --runs)
      RUNS="$2"
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
    --skip-build)
      SKIP_BUILD=1
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

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_aligned64k"
SELECTED_SO="${OUTDIR}/build/selected/libhakozuna_hz6_preload.so"
PHASE_SO="${OUTDIR}/build/phase_count_on/libhakozuna_hz6_preload.so"
SKIP_SO="${OUTDIR}/build/aligned_free_skip/libhakozuna_hz6_preload.so"

mkdir -p "$OUTDIR"

build_bench() {
  mkdir -p "$(dirname "$BENCH")"
  cc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
    -pthread \
    "${ROOT_DIR}/bench/bench_aligned64k.c" \
    -o "$BENCH"
}

build_preloads() {
  OUT_DIR="${OUTDIR}/build/selected" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/selected_build.log" 2>&1
  OUT_DIR="${OUTDIR}/build/phase_count_on" \
    HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/phase_count_on_build.log" 2>&1
  OUT_DIR="${OUTDIR}/build/aligned_free_skip" \
    HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS=1 \
    HZ6_EXTRA_CFLAGS="-DHZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1=1" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/aligned_free_skip_build.log" 2>&1
}

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  build_bench
  build_preloads
fi

[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$PHASE_SO" ]] || { echo "missing phase-count DSO: $PHASE_SO" >&2; exit 2; }
[[ -f "$SKIP_SO" ]] || { echo "missing aligned-free-skip DSO: $SKIP_SO" >&2; exit 2; }

rows=()
IFS=',' read -r -a row_names <<< "$ROWS_CSV"
for row in "${row_names[@]}"; do
  case "$row" in
    posix64_2k)
      rows+=("posix64_2k 2048 64 posix")
      ;;
    aligned4k_4k)
      rows+=("aligned4k_4k 4096 4096 aligned")
      ;;
    posix8k_64k)
      rows+=("posix8k_64k 65536 8192 posix")
      ;;
    *)
      echo "unknown row: $row" >&2
      exit 2
      ;;
  esac
done

{
  echo "arch=${ARCH}"
  echo "threads=${THREADS}"
  echo "iters=${ITERS}"
  echo "runs=${RUNS}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH}"
  echo "selected_so=${SELECTED_SO}"
  echo "phase_so=${PHASE_SO}"
  echo "skip_so=${SKIP_SO}"
} > "${OUTDIR}/README.log"

run_one() {
  local row="$1"
  local size="$2"
  local align="$3"
  local mode="$4"
  local variant="$5"
  local run_id="$6"
  local row_dir="${OUTDIR}/${row}"
  mkdir -p "$row_dir"
  local log="${row_dir}/${run_id}_${variant}.log"

  case "$variant" in
    system)
      "$BENCH" "$THREADS" "$ITERS" "$size" "$align" "$mode" > "$log" 2>&1
      ;;
    selected)
      env LD_PRELOAD="$SELECTED_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$size" "$align" "$mode" \
        > "$log" 2>&1
      ;;
    phase_count_on)
      env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$PHASE_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$size" "$align" "$mode" \
        > "$log" 2>&1
      ;;
    aligned_free_skip)
      env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$SKIP_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$size" "$align" "$mode" \
        > "$log" 2>&1
      ;;
    *)
      echo "unknown variant: $variant" >&2
      exit 2
      ;;
  esac
}

for run_id in $(seq 1 "$RUNS"); do
  for row_spec in "${rows[@]}"; do
    read -r row size align mode <<< "$row_spec"
    for variant in system selected phase_count_on aligned_free_skip; do
      run_one "$row" "$size" "$align" "$mode" "$variant" "$run_id"
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
variants = ["system", "selected", "phase_count_on", "aligned_free_skip"]
ops_re = re.compile(r"ops/s=([0-9]+(?:\.[0-9]+)?)")
kv_re = re.compile(r"([A-Za-z0-9_/]+)=([0-9]+(?:\.[0-9]+)?)")

def parse_line(text, tag):
    for line in text.splitlines():
        if line.startswith(tag):
            return {k: float(v) for k, v in kv_re.findall(line)}
    return {}

def median(values):
    return statistics.median(values) if values else 0.0

print("# HZ6 Preload Aligned Wrapper Audit\n")
print(f"root: `{root}`\n")
print("| row | variant | median ops/s | posix calls | aligned calls | real fallbacks | hz6 path | free route miss real | skip hit | skip miss | record fail | calloc bytes |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for spec in row_specs:
    row, _size, _align, _mode = spec.split()
    for variant in variants:
        ops = []
        posix_calls = 0
        aligned_calls = 0
        real_fallbacks = 0
        hz6_path = 0
        free_route_miss = 0
        skip_hit = 0
        skip_miss = 0
        record_fail = 0
        calloc_bytes = 0
        for log in sorted((root / row).glob(f"*_{variant}.log")):
            text = log.read_text(errors="replace")
            m = ops_re.search(text)
            if m:
                ops.append(float(m.group(1)))
            phase = parse_line(text, "[HZ6_PRELOAD_PHASE_STATS]")
            wrapper = parse_line(text, "[HZ6_PRELOAD_WRAPPER_DETAIL]")
            posix_calls += int(wrapper.get("posix_memalign_calls", 0))
            aligned_calls += int(wrapper.get("aligned_alloc_calls", 0))
            real_fallbacks += int(wrapper.get("posix_memalign_real_fallback", 0))
            real_fallbacks += int(wrapper.get("aligned_alloc_real_fallback", 0))
            hz6_path += int(wrapper.get("posix_memalign_hz6_path", 0))
            hz6_path += int(wrapper.get("aligned_alloc_hz6_path", 0))
            free_route_miss += int(phase.get("free_route_miss_real", 0))
            skip_hit += int(wrapper.get("real_aligned_free_skip_hit", 0))
            skip_miss += int(wrapper.get("real_aligned_free_skip_miss", 0))
            record_fail += int(wrapper.get("real_aligned_record_fail", 0))
            calloc_bytes += int(phase.get("calloc_zero_bytes", 0))
        print(
            f"| `{row}` | `{variant}` | {median(ops):.3f} | "
            f"{posix_calls} | {aligned_calls} | {real_fallbacks} | "
            f"{hz6_path} | {free_route_miss} | {skip_hit} | "
            f"{skip_miss} | {record_fail} | {calloc_bytes} |"
        )
PY
