#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-300000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-focused,fixed_mid}"
ALLOCATORS="${ALLOCATORS:-hz6,hz6-small-boundary-trusted-target,hz6-small-boundary-trusted-toy-map8192-target,hz6-realloc-boundary-4k-target,hz6-realloc-boundary-8k-target,hz6-realloc-boundary-adaptive-4k-target,hz6-realloc-boundary-adaptive-8k-target,hz6-realloc-boundary-adaptive-target,hz6-toy-trusted-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_fixed_boundary_profile_frontier_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_fixed_boundary_profile_frontier.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per allocator/row (default: 3)
  --iters N         iterations per run (default: 300000)
  --ws N            working set (default: 4096)
  --rows CSV        row groups: focused,fixed_mid,large_span
                    (default: focused,fixed_mid)
  --allocators CSV  profile aliases to compare
                    (default: selected fixed-boundary frontier profiles)
  --outdir DIR      output directory
  --skip-builds     reuse existing HZ6/profile/bench builds
  --skip-prepare    reuse existing mimalloc/tcmalloc environment variables
  --help            show this message

This is the fixed-boundary profile lane runner. It compares selected HZ6
against small-boundary trusted, the Toy-map8192 RSS variant, split
realloc-boundary, and adaptive realloc-boundary profile DSOs without changing
selected/default flags.
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

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh" \
  "${args[@]}"
