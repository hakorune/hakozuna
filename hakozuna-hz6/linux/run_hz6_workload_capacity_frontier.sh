#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
ROWS_CSV="${ROWS:-all}"
ALLOCATORS="${ALLOCATORS:-hz6,hz6-workload-capacity-lite-target,hz6-workload-capacity-lite-map8192-target,hz6-workload-capacity-mid-target,hz6-workload-capacity-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_frontier_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_frontier.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 3)
  --iters N          iterations per run (default: 200000)
  --rows CSV         row groups for run_hz6_workload_proxy_matrix.sh
                     (default: all)
  --allocators CSV   capacity frontier allocator aliases
  --outdir DIR       output directory
  --skip-builds      reuse existing HZ6/profile/bench builds
  --skip-prepare     reuse existing external allocator environment
  --help             show this message

This runner keeps the workload-capacity profile ladder separate from the broad
cross-allocator workload-proxy guard.  It compares selected HZ6 against
capacity-lite, capacity-lite-map8192, capacity-mid, and capacity-full profiles.
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
      SKIP_PREPARE=1
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
  --rows "$ROWS_CSV"
  --allocators "$ALLOCATORS"
  --outdir "$OUTDIR"
)

if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  args+=(--skip-builds)
fi
if [[ "$SKIP_PREPARE" -eq 1 ]]; then
  args+=(--skip-prepare)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_proxy_matrix.sh" \
  "${args[@]}"
