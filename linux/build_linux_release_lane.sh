#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HZ3_TARGET="all_ldpreload_scale"
HZ4_TARGET="all_stable"
BUILD_HZ3=1
BUILD_HZ4=1
ARCH="auto"
HZ4_EXTRA=""

usage() {
  cat <<'EOF'
Usage:
  ./linux/build_linux_release_lane.sh [options]

Options:
  --hz3-target <target>  hz3 make target (default: all_ldpreload_scale)
  --hz4-target <target>  hz4 make target (default: all_stable)
  --hz4-extra <defs>     extra hz4 compile-time defs for the hz4 lane
  --arch <arch>          override detected arch (default: auto)
  --skip-hz3             skip hz3 build
  --skip-hz4             skip hz4 build
  --help                 show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --hz3-target)
      [[ $# -ge 2 ]] || { echo "missing value for --hz3-target" >&2; exit 1; }
      HZ3_TARGET="$2"
      shift 2
      ;;
    --hz4-target)
      [[ $# -ge 2 ]] || { echo "missing value for --hz4-target" >&2; exit 1; }
      HZ4_TARGET="$2"
      shift 2
      ;;
    --hz4-extra)
      [[ $# -ge 2 ]] || { echo "missing value for --hz4-extra" >&2; exit 1; }
      HZ4_EXTRA="$2"
      shift 2
      ;;
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --skip-hz3)
      BUILD_HZ3=0
      shift
      ;;
    --skip-hz4)
      BUILD_HZ4=0
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

command -v make >/dev/null 2>&1 || {
  echo "make not found in PATH" >&2
  exit 1
}

if [[ "$ARCH" == "auto" ]]; then
  UNAME_M="$(uname -m)"
  case "$UNAME_M" in
    aarch64|arm64)
      ARCH="arm64"
      ;;
    x86_64|amd64)
      ARCH="x86_64"
      ;;
    *)
      ARCH="$UNAME_M"
      ;;
  esac
fi

echo "[linux] arch: $ARCH"

if [[ "$BUILD_HZ3" -eq 1 ]]; then
  echo "[linux] building hz3 target: $HZ3_TARGET"
  make -C "$ROOT_DIR/hakozuna" clean "$HZ3_TARGET"
  echo "[linux] hz3 output: $ROOT_DIR/libhakozuna_hz3_scale.so"
fi

if [[ "$BUILD_HZ4" -eq 1 ]]; then
  if [[ "$HZ4_TARGET" == "all_stable" && ! -f "$ROOT_DIR/hakozuna-mt/tests/hz4_segq_test.c" ]]; then
    echo "[linux] hz4 all_stable is unavailable in this checkout; falling back to all_perf_lib" >&2
    HZ4_TARGET="all_perf_lib"
  fi
  echo "[linux] building hz4 target: $HZ4_TARGET"
  HZ4_DEFS_PERF_LANE_EXTRA="$HZ4_EXTRA" make -C "$ROOT_DIR/hakozuna-mt" clean "$HZ4_TARGET"
  echo "[linux] hz4 output: $ROOT_DIR/hakozuna-mt/libhakozuna_hz4.so"
fi

echo "[linux] done"
