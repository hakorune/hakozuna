#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
OUT_DIR=""

usage() {
  cat <<'EOF'
Usage:
  ./linux/build_linux_bench_compare.sh [options]

Options:
  --arch <arch>      override detected arch (default: auto)
  --out-dir DIR      output directory for the benchmark binary
  --help             show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --out-dir" >&2; exit 1; }
      OUT_DIR="$2"
      shift 2
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

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/bench/out/linux/${ARCH}}"
SRC="${ROOT_DIR}/bench/bench_mixed_ws.c"
BIN="${OUT_DIR}/bench_mixed_ws_crt"

command -v gcc >/dev/null 2>&1 || {
  echo "gcc not found in PATH" >&2
  exit 1
}

[[ -f "$SRC" ]] || {
  echo "benchmark source not found: $SRC" >&2
  exit 1
}

mkdir -p "$OUT_DIR"

echo "[linux] arch: $ARCH"
echo "[linux] building benchmark binary: $BIN"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  -DHZ3_BENCH_USE_CRT=1 \
  -I"$ROOT_DIR/hakozuna/include" \
  -I"$ROOT_DIR/bench" \
  -pthread \
  "$SRC" -o "$BIN"
echo "[linux] bench output: $BIN"
