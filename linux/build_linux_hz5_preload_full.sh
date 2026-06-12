#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
OUT_DIR=""

usage() {
  cat <<'EOF'
Usage:
  ./linux/build_linux_hz5_preload_full.sh [options]

Options:
  --arch <arch>      override detected arch (default: auto)
  --out-dir DIR      output directory
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

OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-hz5-preload-full}"

exec "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
  --arch "$ARCH" \
  --linux-preload-full \
  --linux-local2p-speed-linkflags \
  --out-dir "$OUT_DIR"
