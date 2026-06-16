#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
PROFILE_ITERS="${PROFILE_ITERS:-300000}"
FIXED_ITERS="${FIXED_ITERS:-180000}"
WORKLOAD_ITERS="${WORKLOAD_ITERS:-200000}"
WS="${WS:-4096}"
PROFILE_ROWS="${PROFILE_ROWS:-focused,fixed_mid}"
FIXED_ROWS="${FIXED_ROWS:-fixed_mid}"
WORKLOAD_ROWS="${WORKLOAD_ROWS:-small_proxy,cache_proxy}"
PROFILE_ALLOCATORS="${PROFILE_ALLOCATORS:-hz6,hz6-toy-trusted-target,hz6-small-boundary-trusted-target,hz6-realloc-boundary-4k-target,hz6-realloc-boundary-8k-target,hz6-realloc-boundary-adaptive-4k-target,hz6-realloc-boundary-adaptive-8k-target,hz6-realloc-boundary-adaptive-target,hz6-aligned-target,hz6-calloc-direct-target,hz6-calloc-real-target,hz6-calloc-large-real-target}"
FIXED_ALLOCATORS="${FIXED_ALLOCATORS:-system,hz3,hz4,hz6,hz6-small-boundary-trusted-toy-map8192-target,hz6-small-boundary-trusted-toy-map8192-external-target,mimalloc,tcmalloc}"
WORKLOAD_ALLOCATORS="${WORKLOAD_ALLOCATORS:-system,hz3,hz4,hz6,hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target,hz6-small-boundary-trusted-toy-map8192-target,hz6-small-boundary-trusted-toy-map8192-external-target,mimalloc,tcmalloc}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_broad_guard_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE=0
RUN_PROFILE=1
RUN_FIXED=1
RUN_WORKLOAD=1

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_broad_guard.sh [options]

Options:
  --arch ARCH                  target arch (default: x86_64)
  --runs N                     runs per allocator/row (default: 3)
  --profile-iters N            profile-frontier iterations (default: 300000)
  --fixed-iters N              fixed-gap iterations (default: 180000)
  --workload-iters N           workload-proxy iterations (default: 200000)
  --ws N                       profile/fixed working set (default: 4096)
  --profile-rows CSV           profile row groups (default: focused,fixed_mid)
  --fixed-rows CSV             fixed-gap row groups (default: fixed_mid)
  --workload-rows CSV          workload-proxy row groups (default: small_proxy,cache_proxy)
  --profile-allocators CSV     profile-frontier allocator aliases
  --fixed-allocators CSV       fixed-gap allocator aliases
  --workload-allocators CSV    workload-proxy allocator aliases
  --outdir DIR                 output directory
  --skip-builds                reuse existing allocator/profile/bench builds
  --skip-prepare               reuse existing external allocator environment
  --skip-profile               do not run profile frontier
  --skip-fixed                 do not run fixed-gap matrix
  --skip-workload              do not run workload-proxy matrix
  --help                       show this message

This is the broad HZ6 guard lane. It intentionally composes existing runners
instead of adding new benchmark logic:
  1. profile frontier for selected/default and profile controls
  2. fixed-gap matrix for HZ3/HZ4/external allocator comparison
  3. workload-proxy matrix for real-workload-shaped capacity guards
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
    --profile-iters)
      PROFILE_ITERS="$2"
      shift 2
      ;;
    --fixed-iters)
      FIXED_ITERS="$2"
      shift 2
      ;;
    --workload-iters)
      WORKLOAD_ITERS="$2"
      shift 2
      ;;
    --ws)
      WS="$2"
      shift 2
      ;;
    --profile-rows)
      PROFILE_ROWS="$2"
      shift 2
      ;;
    --fixed-rows)
      FIXED_ROWS="$2"
      shift 2
      ;;
    --workload-rows)
      WORKLOAD_ROWS="$2"
      shift 2
      ;;
    --profile-allocators)
      PROFILE_ALLOCATORS="$2"
      shift 2
      ;;
    --fixed-allocators)
      FIXED_ALLOCATORS="$2"
      shift 2
      ;;
    --workload-allocators)
      WORKLOAD_ALLOCATORS="$2"
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
    --skip-profile)
      RUN_PROFILE=0
      shift
      ;;
    --skip-fixed)
      RUN_FIXED=0
      shift
      ;;
    --skip-workload)
      RUN_WORKLOAD=0
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
  echo "profile_iters=${PROFILE_ITERS}"
  echo "fixed_iters=${FIXED_ITERS}"
  echo "workload_iters=${WORKLOAD_ITERS}"
  echo "ws=${WS}"
  echo "profile_rows=${PROFILE_ROWS}"
  echo "fixed_rows=${FIXED_ROWS}"
  echo "workload_rows=${WORKLOAD_ROWS}"
  echo "profile_allocators=${PROFILE_ALLOCATORS}"
  echo "fixed_allocators=${FIXED_ALLOCATORS}"
  echo "workload_allocators=${WORKLOAD_ALLOCATORS}"
  echo "run_profile=${RUN_PROFILE}"
  echo "run_fixed=${RUN_FIXED}"
  echo "run_workload=${RUN_WORKLOAD}"
} > "${OUTDIR}/README.log"

common_passthrough=(
  --arch "$ARCH"
  --runs "$RUNS"
)
if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  common_passthrough+=(--skip-builds)
fi
if [[ "$SKIP_PREPARE" -eq 1 ]]; then
  common_passthrough+=(--skip-prepare)
fi

if [[ "$RUN_PROFILE" -eq 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh" \
    "${common_passthrough[@]}" \
    --iters "$PROFILE_ITERS" \
    --ws "$WS" \
    --rows "$PROFILE_ROWS" \
    --allocators "$PROFILE_ALLOCATORS" \
    --outdir "${OUTDIR}/profile_frontier"
fi

if [[ "$RUN_FIXED" -eq 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_fixed_gap_matrix.sh" \
    "${common_passthrough[@]}" \
    --iters "$FIXED_ITERS" \
    --ws "$WS" \
    --rows "$FIXED_ROWS" \
    --allocators "$FIXED_ALLOCATORS" \
    --outdir "${OUTDIR}/fixed_gap"
fi

if [[ "$RUN_WORKLOAD" -eq 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_proxy_matrix.sh" \
    "${common_passthrough[@]}" \
    --iters "$WORKLOAD_ITERS" \
    --rows "$WORKLOAD_ROWS" \
    --allocators "$WORKLOAD_ALLOCATORS" \
    --outdir "${OUTDIR}/workload_proxy"
fi

{
  echo "# HZ6 Broad Guard"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "This guard composes existing profile, fixed-gap, and workload-proxy runners."
  echo
  if [[ -f "${OUTDIR}/profile_frontier/summary.md" ]]; then
    echo "## Profile Frontier"
    echo
    cat "${OUTDIR}/profile_frontier/summary.md"
    echo
  fi
  if [[ -f "${OUTDIR}/fixed_gap/summary.md" ]]; then
    echo "## Fixed Gap"
    echo
    cat "${OUTDIR}/fixed_gap/summary.md"
    echo
  fi
  if [[ -f "${OUTDIR}/workload_proxy/summary.md" ]]; then
    echo "## Workload Proxy"
    echo
    cat "${OUTDIR}/workload_proxy/summary.md"
  fi
} > "${OUTDIR}/summary.md"

cat "${OUTDIR}/summary.md"
