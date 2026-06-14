#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Alias for scripts that still want a named MidPage-target DSO.  The selected
# preload bundle now includes the same outer-guard noinline boundary.
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_midpage_target}" \
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
