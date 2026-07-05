#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIB="${ROOT}/libhz10.so"

make -C "${ROOT}" preload >/dev/null

run_preload() {
  local name="$1"
  shift
  echo "## ${name}"
  env LD_PRELOAD="${LIB}" "$@"
}

run_preload true /bin/true
run_preload ls /bin/ls "${ROOT}/src" >/dev/null
run_preload grep /bin/grep -q "HZ10" "${ROOT}/src/hz10_public_entry.h"
run_preload python3 /usr/bin/python3 -c 'print("hz10-shim-smoke")' >/dev/null
run_preload git git -C "${ROOT}" status --short >/dev/null

echo "[hz10-shim-smoke] ok"
