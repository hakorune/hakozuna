#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-180000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-fixed_mid}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,hz6,hz6-small-boundary-trusted-toy-map8192-target,hz6-small-boundary-trusted-toy-map8192-external-target,mimalloc,tcmalloc}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_fixed_gap_matrix_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_fixed_gap_matrix.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per allocator/row (default: 3)
  --iters N         iterations per run (default: 180000)
  --ws N            working set (default: 4096)
  --rows CSV        row groups: fixed_mid,large_span (default: fixed_mid)
  --allocators CSV  allocator/profile aliases
                    (default: system,hz3,hz4,hz6,HZ6 Toy-map8192 RSS profiles,mimalloc,tcmalloc)
  --outdir DIR      output directory
  --skip-builds     reuse existing HZ/profile/bench builds
  --skip-prepare    reuse existing mimalloc/tcmalloc environment variables
  --help            show this message

This is the focused fixed-size gap runner. It keeps selected HZ6, the current
HZ6 fixed RSS profiles, HZ3/HZ4, and external allocators in one matrix so the
remaining fixed_4k/fixed_8k gap can be refreshed without changing selected
default flags.
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

args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --ws "$WS"
  --rows "$ROWS_CSV"
  --allocators "$ALLOCATORS"
  --outdir "$OUTDIR"
)

if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  args+=(--skip-builds)
fi
if [[ "$SKIP_PREPARE_ALLOCATORS" -eq 1 ]]; then
  args+=(--skip-prepare)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_ubuntu_size_slices_matrix.sh" \
  "${args[@]}"
