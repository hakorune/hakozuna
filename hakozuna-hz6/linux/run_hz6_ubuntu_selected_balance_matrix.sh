#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-5}"
ITERS="${ITERS:-500000}"
WS="${WS:-4096}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,hz5,hz6,mimalloc,tcmalloc}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_ubuntu_selected_balance_matrix.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per allocator/row (default: 5)
  --iters N         iterations per run (default: 500000)
  --ws N            working set (default: 4096)
  --allocators CSV  allocator list
  --outdir DIR      output directory
  --skip-builds     reuse existing HZ3/HZ4/HZ5/HZ6/bench builds
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
    --allocators)
      ALLOCATORS="$2"
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
      exit 1
      ;;
  esac
done

mkdir -p "$OUTDIR"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
  "${ROOT_DIR}/linux/build_linux_hz5_preload_full.sh" --arch "$ARCH"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  if [[ ",${ALLOCATORS}," == *",hz6-small-boundary-fast-target,"* ||
        ",${ALLOCATORS}," == *",hz6_small_boundary_fast_target,"* ]]; then
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh"
  fi
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  env_file="${OUTDIR}/allocator_env.sh"
  "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" \
    > "$env_file"
  # shellcheck disable=SC1090
  source "$env_file"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "allocators=${ALLOCATORS}"
  echo "bench=${BENCH_BIN}"
} > "${OUTDIR}/README.log"

run_row() {
  local row="$1"
  local min_size="$2"
  local max_size="$3"
  local row_out="${OUTDIR}/${row}"
  mkdir -p "$row_out"
  "${ROOT_DIR}/bench/run_compare.sh" \
    --allocators "$ALLOCATORS" \
    --bench-bin "$BENCH_BIN" \
    --bench-args "4 ${ITERS} ${WS} ${min_size} ${max_size}" \
    --runs "$RUNS" \
    --outdir "$row_out"
}

run_row 16_256 16 256
run_row 16_4096 16 4096
run_row 1024_4096 1024 4096
run_row 4096_16384 4096 16384

python3 - <<'PY' "$OUTDIR" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
rows = ["16_256", "16_4096", "1024_4096", "4096_16384"]
alloc_re = re.compile(r"^\d+_(.+)\.log$")
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")

print(f"# HZ6 Ubuntu Selected Balance Matrix\n")
print(f"root: `{root}`\n")
print("| row | allocator | median ops/s | median peak MiB | ops/s per MiB |")
print("| --- | --- | ---: | ---: | ---: |")
for row in rows:
    row_dir = root / row
    by_alloc = {}
    for log in sorted(row_dir.glob("*.log")):
        match = alloc_re.match(log.name)
        if not match:
            continue
        alloc = match.group(1)
        text = log.read_text(errors="replace")
        ops_match = ops_re.search(text)
        peak_match = peak_re.search(text)
        if not ops_match or not peak_match:
            continue
        by_alloc.setdefault(alloc, {"ops": [], "peak": []})
        by_alloc[alloc]["ops"].append(float(ops_match.group(1)))
        by_alloc[alloc]["peak"].append(float(peak_match.group(1)) / 1024.0)
    for alloc in sorted(by_alloc):
        ops = statistics.median(by_alloc[alloc]["ops"])
        peak = statistics.median(by_alloc[alloc]["peak"])
        efficiency = ops / peak if peak else 0.0
        print(f"| `{row}` | `{alloc}` | {ops:.3f} | {peak:.2f} | {efficiency:.3f} |")
PY
