#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
ALLOCATORS="system,hz3,hz4,hz5,mimalloc,tcmalloc"
SKIP_ALLOCATOR_BUILDS=0
FORWARD_ARGS=()

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_bench_compare_matrix.sh [options]

Options:
  --arch <arch>             override detected arch (default: auto)
  --allocators LIST         comma-separated allocator list
  --skip-allocator-builds   reuse existing hz3/hz4/hz5/hz6 builds
  --help                    show this message

All remaining options are forwarded to ./linux/run_linux_bench_compare.sh.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --allocators)
      [[ $# -ge 2 ]] || { echo "missing value for --allocators" >&2; exit 1; }
      ALLOCATORS="$2"
      shift 2
      ;;
    --skip-allocator-builds)
      SKIP_ALLOCATOR_BUILDS=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      FORWARD_ARGS+=("$1")
      shift
      ;;
  esac
done

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

echo "[linux] arch: $ARCH"
echo "[linux] allocators: $ALLOCATORS"

if [[ "$SKIP_ALLOCATOR_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
  "${ROOT_DIR}/linux/build_linux_hz5_preload_full.sh" --arch "$ARCH"
  if [[ ",${ALLOCATORS}," == *",hz6,"* ]]; then
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  fi
fi

exec "${ROOT_DIR}/linux/run_linux_bench_compare.sh" \
  --arch "$ARCH" \
  --allocators "$ALLOCATORS" \
  "${FORWARD_ARGS[@]}"
