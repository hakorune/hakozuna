#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-9}"
ITERS="${ITERS:-200000}"
ROWS="${ROWS:-small_proxy,cache_proxy}"
ALLOCATORS="${ALLOCATORS:-hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_pair_focus_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_pair_focus.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 9)
  --iters N          iterations per run (default: 200000)
  --rows CSV         row groups for workload proxy matrix
                     (default: small_proxy,cache_proxy)
  --allocators CSV   allocator/profile aliases to compare
  --outdir DIR       output directory
  --skip-builds      reuse existing allocator/profile/bench builds
  --skip-prepare     reuse existing external allocator environment
  --help             show this message

This focused guard compares the paired workload-capacity profiles only:
  - hz6-workload-capacity-narrow-target
  - hz6-workload-capacity-hybrid-target

It is proxy evidence for profile recommendation stability, not selected/default
promotion evidence by itself.
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
      ROWS="$2"
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

mkdir -p "$OUTDIR"

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "rows=${ROWS}"
  echo "allocators=${ALLOCATORS}"
  echo "note=focused capacity-narrow vs capacity-hybrid workload proxy guard"
  echo "decision=profile recommendation stability only; no selected/default promotion"
} > "${OUTDIR}/README.log"

args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --rows "$ROWS"
  --allocators "$ALLOCATORS"
  --outdir "${OUTDIR}/workload_proxy"
)

if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  args+=(--skip-builds)
fi
if [[ "$SKIP_PREPARE" -eq 1 ]]; then
  args+=(--skip-prepare)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_proxy_matrix.sh" \
  "${args[@]}"

{
  echo "# HZ6 Workload Capacity Pair Focus"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "This focused guard compares capacity-narrow and capacity-hybrid only."
  echo "Use it to check row-level stability before updating workload profile"
  echo "recommendations; do not promote selected/default from this proxy alone."
  echo
  if [[ -f "${OUTDIR}/workload_proxy/summary.md" ]]; then
    cat "${OUTDIR}/workload_proxy/summary.md"
  else
    echo "missing workload proxy summary: ${OUTDIR}/workload_proxy/summary.md"
  fi
} | tee "${OUTDIR}/summary.md"
