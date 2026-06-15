#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
ARCH="${ARCH:-x86_64}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-100000}"
RUNS="${RUNS:-5}"
ROWS_CSV="${ROWS:-calloc4k,calloc8k,calloc32k,calloc64k}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_preload_calloc_audit_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILD=0

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_calloc_audit.sh [options]

Options:
  --arch ARCH     target arch (default: x86_64)
  --threads N     worker threads (default: 4)
  --iters N       iterations per thread (default: 100000)
  --runs N        repeat count (default: 5)
  --rows CSV      rows: calloc4k,calloc8k,calloc32k,calloc64k
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
      exit 2
      ;;
  esac
done

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_calloc_ws"
SELECTED_SO="${OUTDIR}/build/selected/libhakozuna_hz6_preload.so"
PHASE_SO="${OUTDIR}/build/phase_count_on/libhakozuna_hz6_preload.so"
REAL_CALLOC_SO="${OUTDIR}/build/real_calloc_fallback/libhakozuna_hz6_preload.so"
REAL_CALLOC_STATS_SO="${OUTDIR}/build/real_calloc_fallback_stats/libhakozuna_hz6_preload.so"
REAL_CALLOC_FREE_SKIP_SO="${OUTDIR}/build/real_calloc_fallback_free_skip/libhakozuna_hz6_preload.so"
REAL_CALLOC_FREE_SKIP_STATS_SO="${OUTDIR}/build/real_calloc_fallback_free_skip_stats/libhakozuna_hz6_preload.so"

mkdir -p "$OUTDIR"

build_bench() {
  mkdir -p "$(dirname "$BENCH")"
  cc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
    -pthread \
    "${ROOT_DIR}/bench/bench_calloc_ws.c" \
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
  local real_calloc_prod_flags=()
  hz6_preload_effective_selected_cflags real_calloc_prod_flags 1
  hz6_preload_replace_define real_calloc_prod_flags \
    HZ6_PRELOAD_CALLOC_REAL_FALLBACK_L1 1
  OUT_DIR="${OUTDIR}/build/real_calloc_fallback" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${real_calloc_prod_flags[@]}")" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/real_calloc_fallback_build.log" 2>&1
  local real_calloc_stats_flags=("${real_calloc_prod_flags[@]}")
  hz6_preload_preserve_phase_counters real_calloc_stats_flags
  OUT_DIR="${OUTDIR}/build/real_calloc_fallback_stats" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${real_calloc_stats_flags[@]}")" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/real_calloc_fallback_stats_build.log" 2>&1
  local real_calloc_free_skip_prod_flags=("${real_calloc_prod_flags[@]}")
  hz6_preload_replace_define real_calloc_free_skip_prod_flags \
    HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1 1
  OUT_DIR="${OUTDIR}/build/real_calloc_fallback_free_skip" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${real_calloc_free_skip_prod_flags[@]}")" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/real_calloc_fallback_free_skip_build.log" 2>&1
  local real_calloc_free_skip_stats_flags=("${real_calloc_free_skip_prod_flags[@]}")
  hz6_preload_preserve_phase_counters real_calloc_free_skip_stats_flags
  OUT_DIR="${OUTDIR}/build/real_calloc_fallback_free_skip_stats" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${real_calloc_free_skip_stats_flags[@]}")" \
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/real_calloc_fallback_free_skip_stats_build.log" 2>&1
}

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  build_bench
  build_preloads
fi

[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$PHASE_SO" ]] || { echo "missing phase-count DSO: $PHASE_SO" >&2; exit 2; }
[[ -f "$REAL_CALLOC_SO" ]] || { echo "missing real-calloc DSO: $REAL_CALLOC_SO" >&2; exit 2; }
[[ -f "$REAL_CALLOC_STATS_SO" ]] || { echo "missing real-calloc stats DSO: $REAL_CALLOC_STATS_SO" >&2; exit 2; }
[[ -f "$REAL_CALLOC_FREE_SKIP_SO" ]] || { echo "missing real-calloc free-skip DSO: $REAL_CALLOC_FREE_SKIP_SO" >&2; exit 2; }
[[ -f "$REAL_CALLOC_FREE_SKIP_STATS_SO" ]] || { echo "missing real-calloc free-skip stats DSO: $REAL_CALLOC_FREE_SKIP_STATS_SO" >&2; exit 2; }

rows=()
IFS=',' read -r -a row_names <<< "$ROWS_CSV"
for row in "${row_names[@]}"; do
  case "$row" in
    calloc4k)
      rows+=("calloc4k 1 4096")
      ;;
    calloc8k)
      rows+=("calloc8k 1 8192")
      ;;
    calloc32k)
      rows+=("calloc32k 1 32768")
      ;;
    calloc64k)
      rows+=("calloc64k 1 65536")
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
  echo "real_calloc_so=${REAL_CALLOC_SO}"
  echo "real_calloc_stats_so=${REAL_CALLOC_STATS_SO}"
  echo "real_calloc_free_skip_so=${REAL_CALLOC_FREE_SKIP_SO}"
  echo "real_calloc_free_skip_stats_so=${REAL_CALLOC_FREE_SKIP_STATS_SO}"
} > "${OUTDIR}/README.log"

run_one() {
  local row="$1"
  local nmemb="$2"
  local elem_size="$3"
  local variant="$4"
  local run_id="$5"
  local row_dir="${OUTDIR}/${row}"
  mkdir -p "$row_dir"
  local log="${row_dir}/${run_id}_${variant}.log"

  case "$variant" in
    system)
      "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 > "$log" 2>&1
      ;;
    selected)
      env LD_PRELOAD="$SELECTED_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 \
        > "$log" 2>&1
      ;;
    phase_count_on)
      env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$PHASE_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 \
        > "$log" 2>&1
      ;;
    real_calloc_fallback)
      env LD_PRELOAD="$REAL_CALLOC_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 \
        > "$log" 2>&1
      ;;
    real_calloc_fallback_stats)
      env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$REAL_CALLOC_STATS_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 \
        > "$log" 2>&1
      ;;
    real_calloc_fallback_free_skip)
      env LD_PRELOAD="$REAL_CALLOC_FREE_SKIP_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 \
        > "$log" 2>&1
      ;;
    real_calloc_fallback_free_skip_stats)
      env HZ6_PRELOAD_STATS=1 LD_PRELOAD="$REAL_CALLOC_FREE_SKIP_STATS_SO" \
        "$BENCH" "$THREADS" "$ITERS" "$nmemb" "$elem_size" 1 \
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
    read -r row nmemb elem_size <<< "$row_spec"
    for variant in system selected phase_count_on real_calloc_fallback real_calloc_fallback_stats real_calloc_fallback_free_skip real_calloc_fallback_free_skip_stats; do
      run_one "$row" "$nmemb" "$elem_size" "$variant" "$run_id"
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
variants = [
    "system",
    "selected",
    "phase_count_on",
    "real_calloc_fallback",
    "real_calloc_fallback_stats",
    "real_calloc_fallback_free_skip",
    "real_calloc_fallback_free_skip_stats",
]
ops_re = re.compile(r"ops/s=([0-9]+(?:\.[0-9]+)?)")
current_re = re.compile(r"current_kb=([0-9]+)")
kv_re = re.compile(r"([A-Za-z0-9_/]+)=([0-9]+(?:\.[0-9]+)?)")

def parse_line(text, tag):
    for line in text.splitlines():
        if line.startswith(tag):
            return {k: float(v) for k, v in kv_re.findall(line)}
    return {}

def median(values):
    return statistics.median(values) if values else 0.0

print("# HZ6 Preload Calloc Audit\n")
print(f"root: `{root}`\n")
print("| row | variant | median ops/s | median current MiB | calloc calls | calloc zero MiB | malloc boundary mid | free real | route miss real | calloc record | calloc skip hit | calloc skip miss |")
print("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for spec in row_specs:
    row, _nmemb, _elem_size = spec.split()
    for variant in variants:
        ops = []
        current = []
        calloc_calls = 0
        calloc_zero = 0
        malloc_mid = 0
        free_real = 0
        route_miss_real = 0
        calloc_record = 0
        calloc_skip_hit = 0
        calloc_skip_miss = 0
        for log in sorted((root / row).glob(f"*_{variant}.log")):
            text = log.read_text(errors="replace")
            m = ops_re.search(text)
            if m:
                ops.append(float(m.group(1)))
            r = current_re.search(text)
            if r:
                current.append(float(r.group(1)) / 1024.0)
            phase = parse_line(text, "[HZ6_PRELOAD_PHASE_STATS]")
            calloc_calls += int(phase.get("calloc_calls", 0))
            calloc_zero += int(phase.get("calloc_zero_bytes", 0))
            malloc_mid += int(phase.get("malloc_midpage_boundary", 0))
            free_real += int(phase.get("free_real", 0))
            route_miss_real += int(phase.get("free_route_miss_real", 0))
            wrapper = parse_line(text, "[HZ6_PRELOAD_WRAPPER_DETAIL]")
            calloc_record += int(wrapper.get("calloc_real_record_set", 0))
            calloc_skip_hit += int(wrapper.get("calloc_real_free_skip_hit", 0))
            calloc_skip_miss += int(wrapper.get("calloc_real_free_skip_miss", 0))
        print(
            f"| `{row}` | `{variant}` | {median(ops):.3f} | "
            f"{median(current):.2f} | {calloc_calls} | "
            f"{calloc_zero / (1024.0 * 1024.0):.2f} | {malloc_mid} | "
            f"{free_real} | {route_miss_real} | {calloc_record} | "
            f"{calloc_skip_hit} | {calloc_skip_miss} |"
        )
PY
