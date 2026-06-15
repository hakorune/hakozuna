#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-250000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-fixed_mid}"
ALLOCATORS="${ALLOCATORS:-hz6,hz6-small-boundary-trusted-target,hz3,hz4,mimalloc,tcmalloc}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_fixed_cost_residency_matrix_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_fixed_cost_residency_matrix.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          cross-allocator runs per row (default: 3)
  --iters N         iterations per run (default: 250000)
  --ws N            working set (default: 4096)
  --rows CSV        row groups: focused,fixed_mid,large_span
                    (default: fixed_mid)
  --allocators CSV  cross-allocator/profile aliases
                    (default: hz6,hz6-small-boundary-trusted-target,hz3,hz4,mimalloc,tcmalloc)
  --outdir DIR      output directory
  --skip-builds     reuse existing HZ/profile/bench builds
  --skip-prepare    reuse existing mimalloc/tcmalloc environment variables
  --help            show this message

This runner is diagnostic orchestration for FixedCostResidencyMatrix-L1:
  1. cross-allocator speed/RSS matrix via run_hz6_preload_profile_frontier.sh
  2. HZ6 diagnostic memory attribution via run_hz6_midpage_rss_audit.sh
  3. compact combined summary with links to both raw sub-runs
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

mkdir -p "$OUTDIR"

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "rows=${ROWS_CSV}"
  echo "allocators=${ALLOCATORS}"
} > "${OUTDIR}/README.log"

frontier_args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --ws "$WS"
  --rows "$ROWS_CSV"
  --allocators "$ALLOCATORS"
  --outdir "${OUTDIR}/profile_frontier"
)
rss_args=(
  --arch "$ARCH"
  --iters "$ITERS"
  --ws "$WS"
  --rows "$ROWS_CSV"
  --outdir "${OUTDIR}/rss_audit"
)

if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  frontier_args+=(--skip-builds)
  rss_args+=(--skip-build)
fi
if [[ "$SKIP_PREPARE_ALLOCATORS" -eq 1 ]]; then
  frontier_args+=(--skip-prepare)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh" \
  "${frontier_args[@]}"

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh" \
  "${rss_args[@]}"

{
  echo "# HZ6 Fixed Cost Residency Matrix"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "## Purpose"
  echo
  echo '```text'
  echo "FixedCostResidencyMatrix-L1 compares HZ6 speed/RSS against peer"
  echo "allocators and pairs that with HZ6 diagnostic residency attribution."
  echo "Use it to decide whether the remaining HZ6 fixed RSS cost is frontcache,"
  echo "active-map/static storage, MidPage payload residency, or source-run shape."
  echo '```'
  echo
  echo "## Inputs"
  echo
  echo '```text'
  echo "rows=${ROWS_CSV}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "allocators=${ALLOCATORS}"
  echo '```'
  echo
  echo "## Cross-Allocator Speed/RSS"
  echo
  cat "${OUTDIR}/profile_frontier/summary.md"
  echo
  echo "## HZ6 Residency Attribution"
  echo
  cat "${OUTDIR}/rss_audit/summary.md"
  echo
  echo "## Read Rules"
  echo
  echo '```text'
  echo "peak MiB:"
  echo "  authoritative process high-water RSS from benchmark output"
  echo
  echo "quiescent/current RSS:"
  echo "  measure separately with malloc_trim/scavenge runners when needed"
  echo
  echo "attributed/payload bytes:"
  echo "  diagnostic attribution only; use to choose the next lane, not as exact RSS"
  echo
  echo "defaulting:"
  echo "  this runner does not justify default behavior by itself"
  echo "  require focused/fixed/cross-allocator guard evidence before selected changes"
  echo '```'
} > "${OUTDIR}/summary.md"

cat "${OUTDIR}/summary.md"
