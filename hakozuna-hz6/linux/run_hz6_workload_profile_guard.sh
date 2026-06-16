#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
ROWS="${ROWS:-small_proxy,cache_proxy}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,hz6,hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target,hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target,mimalloc,tcmalloc}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_profile_guard_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_profile_guard.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 3)
  --iters N          iterations per run (default: 200000)
  --rows CSV         row groups for workload proxy matrix
                     (default: small_proxy,cache_proxy)
  --allocators CSV   allocator/profile aliases to compare
  --outdir DIR       output directory
  --skip-builds      reuse existing allocator/profile/bench builds
  --skip-prepare     reuse existing external allocator environment
  --help             show this message

This is the narrow workload-profile decision guard. It intentionally reuses
run_hz6_workload_proxy_matrix.sh and keeps selected/default separate from the
explicit workload profiles:
  - hz6 selected/default
  - hz6-workload-capacity-narrow-target
  - hz6-workload-capacity-hybrid-target
  - hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target
  - external allocators for scale comparison
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
  echo "note=workload profile guard over bench_mixed_ws_crt proxy rows"
  echo "decision=compare selected/default against explicit workload profiles; do not promote default from proxy evidence alone"
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
  echo "# HZ6 Workload Profile Guard"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "This guard compares selected/default against explicit workload profiles."
  echo "Proxy rows are decision evidence for profile recommendations, not"
  echo "selected/default promotion by themselves."
  echo
  if [[ -f "${OUTDIR}/workload_proxy/summary.md" ]]; then
    cat "${OUTDIR}/workload_proxy/summary.md"
  else
    echo "missing workload proxy summary: ${OUTDIR}/workload_proxy/summary.md"
  fi
} | tee "${OUTDIR}/summary.md"
