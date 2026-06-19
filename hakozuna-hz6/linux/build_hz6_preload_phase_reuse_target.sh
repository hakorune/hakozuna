#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/bench/out/linux/${ARCH}}"
CC_BIN="${CC:-cc}"
TARGET="${OUT_DIR}/bench_phase_reuse"

mkdir -p "$OUT_DIR"

command -v "$CC_BIN" >/dev/null 2>&1 || {
  echo "compiler not found in PATH: $CC_BIN" >&2
  exit 1
}

"$CC_BIN" -O3 -Wall -Wextra -Werror -std=c11 \
  -D_POSIX_C_SOURCE=200809L -pthread \
  "${ROOT_DIR}/bench/bench_phase_reuse.c" \
  -o "$TARGET"

echo "[linux][hz6] phase-reuse target: ${TARGET}"
