#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-300000}"
WS="${WS:-4096}"
ALLOCATORS="${ALLOCATORS:-hz6,hz6-toy-trusted-target,hz6-small-boundary-trusted-target,hz6-realloc-boundary-4k-target,hz6-realloc-boundary-8k-target,hz6-aligned-target,hz6-calloc-real-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0
ROWS_CSV="${ROWS:-focused,fixed_mid}"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_profile_frontier.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per allocator/row (default: 3)
  --iters N         iterations per run (default: 300000)
  --ws N            working set (default: 4096)
  --allocators CSV  HZ6 allocator/profile aliases to compare
  --rows CSV        row groups: focused,fixed_mid,large_span
                    (default: focused,fixed_mid)
  --outdir DIR      output directory
  --skip-builds     reuse existing HZ6/profile/bench builds
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
  echo "allocators=${ALLOCATORS}"
  echo "rows=${ROWS_CSV}"
} > "${OUTDIR}/README.log"

run_focused=0
size_rows=()
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    focused)
      run_focused=1
      ;;
    fixed_mid|large_span)
      size_rows+=("$row_group")
      ;;
    "")
      ;;
    *)
      echo "unknown row group: ${row_group}" >&2
      exit 2
      ;;
  esac
done

common_args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --ws "$WS"
  --allocators "$ALLOCATORS"
)
if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  common_args+=(--skip-builds)
fi
if [[ "$SKIP_PREPARE_ALLOCATORS" -eq 1 ]]; then
  common_args+=(--skip-prepare)
fi

if [[ "$run_focused" -eq 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_ubuntu_selected_balance_matrix.sh" \
    "${common_args[@]}" \
    --outdir "${OUTDIR}/focused"
fi

if [[ "${#size_rows[@]}" -gt 0 ]]; then
  local_rows="$(IFS=','; echo "${size_rows[*]}")"
  "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_ubuntu_size_slices_matrix.sh" \
    "${common_args[@]}" \
    --rows "$local_rows" \
    --outdir "${OUTDIR}/size_slices"
fi

{
  echo "# HZ6 Preload Profile Frontier"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  if [[ -f "${OUTDIR}/focused/summary.md" ]]; then
    echo "## Focused"
    echo
    cat "${OUTDIR}/focused/summary.md"
    echo
  fi
  if [[ -f "${OUTDIR}/size_slices/summary.md" ]]; then
    echo "## Size Slices"
    echo
    cat "${OUTDIR}/size_slices/summary.md"
  fi
} > "${OUTDIR}/summary.md"

cat "${OUTDIR}/summary.md"
