#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_aliases.sh"

ARCH="${ARCH:-x86_64}"
THREADS="${THREADS:-4}"
ITERS="${ITERS:-10000}"
RUNS="${RUNS:-3}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,hz6,hz6-calloc-large-real-target,mimalloc,tcmalloc}"
ROWS_CSV="${ROWS:-calloc64k,calloc128k,calloc256k}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_preload_calloc_cross_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_calloc_cross_matrix.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --threads N       worker threads (default: 4)
  --iters N         iterations per thread (default: 10000)
  --runs N          runs per allocator/row (default: 3)
  --allocators CSV  allocator/profile list
  --rows CSV        rows: calloc64k,calloc128k,calloc256k
  --outdir DIR      output directory
  --skip-builds     reuse existing HZ3/HZ4/HZ6/profile/bench builds
  --skip-prepare    reuse existing mimalloc/tcmalloc environment variables
  --help            show this message
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
    --allocators)
      ALLOCATORS="$2"
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
    --skip-prepare)
      SKIP_PREPARE_ALLOCATORS=1
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

allocator_list_contains() {
  local needle="$1"
  [[ ",${ALLOCATORS}," == *",${needle},"* ]]
}

mkdir -p "$OUTDIR"

BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_calloc_ws"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  hz6_preload_build_requested_aliases "$ALLOCATORS" "$ROOT_DIR"
  mkdir -p "$(dirname "$BENCH")"
  cc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
    -pthread \
    "${ROOT_DIR}/bench/bench_calloc_ws.c" \
    -o "$BENCH"
fi

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  if allocator_list_contains mimalloc || allocator_list_contains tcmalloc; then
    env_file="${OUTDIR}/allocator_env.sh"
    "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" \
      > "$env_file"
    # shellcheck disable=SC1090
    source "$env_file"
  fi
fi

[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }

rows=()
IFS=',' read -r -a row_names <<< "$ROWS_CSV"
for row in "${row_names[@]}"; do
  case "$row" in
    calloc64k)
      rows+=("calloc64k 1 65536")
      ;;
    calloc128k)
      rows+=("calloc128k 1 131072")
      ;;
    calloc256k)
      rows+=("calloc256k 1 262144")
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
  echo "allocators=${ALLOCATORS}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH}"
} > "${OUTDIR}/README.log"

for row_spec in "${rows[@]}"; do
  read -r row nmemb elem_size <<< "$row_spec"
  "${ROOT_DIR}/bench/run_compare.sh" \
    --allocators "$ALLOCATORS" \
    --bench-bin "$BENCH" \
    --bench-args "${THREADS} ${ITERS} ${nmemb} ${elem_size} 1" \
    --runs "$RUNS" \
    --outdir "${OUTDIR}/${row}"
done

python3 - <<'PY' "$OUTDIR" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
row_specs = sys.argv[2:]
rows = [spec.split()[0] for spec in row_specs]
alloc_re = re.compile(r"^\d+_(.+)\.log$")
ops_re = re.compile(r"ops/s=([0-9.]+)")
current_re = re.compile(r"current_kb=([0-9]+)")

print("# HZ6 Preload Calloc Cross Matrix\n")
print(f"root: `{root}`\n")
print("| row | allocator | median ops/s | median current MiB | ops/s per MiB |")
print("| --- | --- | ---: | ---: | ---: |")
for row in rows:
    by_alloc = {}
    for log in sorted((root / row).glob("*.log")):
        match = alloc_re.match(log.name)
        if not match:
            continue
        alloc = match.group(1)
        text = log.read_text(errors="replace")
        ops_match = ops_re.search(text)
        current_match = current_re.search(text)
        if not ops_match or not current_match:
            continue
        by_alloc.setdefault(alloc, {"ops": [], "current": []})
        by_alloc[alloc]["ops"].append(float(ops_match.group(1)))
        by_alloc[alloc]["current"].append(float(current_match.group(1)) / 1024.0)
    for alloc in sorted(by_alloc):
        ops = statistics.median(by_alloc[alloc]["ops"])
        current = statistics.median(by_alloc[alloc]["current"])
        efficiency = ops / current if current else 0.0
        print(f"| `{row}` | `{alloc}` | {ops:.3f} | {current:.2f} | {efficiency:.3f} |")
PY
