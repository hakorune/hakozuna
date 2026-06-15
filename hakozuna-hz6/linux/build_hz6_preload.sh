#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload}"
CC_BIN="${CC:-cc}"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-}"
HZ6_PRELOAD_DEFAULT_CFLAGS="${HZ6_PRELOAD_DEFAULT_CFLAGS:-}"
HZ6_PRELOAD_ENABLE_MIDPAGE_BOUNDARY="${HZ6_PRELOAD_ENABLE_MIDPAGE_BOUNDARY:-1}"
HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS="${HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS:-0}"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"
HZ6_PRELOAD_SELECTED_CFLAGS=()
hz6_preload_effective_selected_cflags HZ6_PRELOAD_SELECTED_CFLAGS \
  "$HZ6_PRELOAD_ENABLE_MIDPAGE_BOUNDARY"
if [[ "$HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS" -ne 0 ]]; then
  hz6_preload_preserve_phase_counters HZ6_PRELOAD_SELECTED_CFLAGS
fi

mkdir -p "$OUT_DIR"

source "${HZ6_DIR}/linux/hz6_sources.sh"
HZ6_INCLUDES=("${HZ6_COMMON_INCLUDES[@]}" "${HZ6_DIR}/preload")

HZ6_PRELOAD_SOURCES=(
  "${HZ6_DIR}/preload/hz6_preload.c"
  "${HZ6_DIR}/preload/hz6_preload_hooks.c"
  "${HZ6_DIR}/preload/hz6_preload_real.c"
  "${HZ6_DIR}/preload/hz6_preload_stats.c"
)
PRELOAD_SO="${OUT_DIR}/libhakozuna_hz6_preload.so"

HZ6_INCLUDE_FLAGS=()
for include_dir in "${HZ6_INCLUDES[@]}"; do
  HZ6_INCLUDE_FLAGS+=("-I${include_dir}")
done

HZ6_EXTRA_CFLAGS_ARRAY=()
if [[ -n "${HZ6_PRELOAD_DEFAULT_CFLAGS}" ]]; then
  # shellcheck disable=SC2206
  HZ6_EXTRA_CFLAGS_ARRAY=(${HZ6_PRELOAD_DEFAULT_CFLAGS})
else
  HZ6_EXTRA_CFLAGS_ARRAY=("${HZ6_PRELOAD_SELECTED_CFLAGS[@]}")
fi
if [[ -n "${HZ6_EXTRA_CFLAGS}" ]]; then
  # shellcheck disable=SC2206
  HZ6_EXTRA_CFLAGS_ARRAY+=(${HZ6_EXTRA_CFLAGS})
fi

command -v "$CC_BIN" >/dev/null 2>&1 || {
  echo "compiler not found in PATH: $CC_BIN" >&2
  exit 1
}

echo "[linux][hz6] building preload: ${PRELOAD_SO}"
"$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 -fPIC -shared \
  -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -ftls-model=initial-exec \
  -fno-builtin \
  "${HZ6_EXTRA_CFLAGS_ARRAY[@]}" \
  "${HZ6_INCLUDE_FLAGS[@]}" \
  "${HZ6_LIB_SOURCES[@]}" \
  "${HZ6_PRELOAD_SOURCES[@]}" \
  -ldl -pthread \
  -o "$PRELOAD_SO"
echo "[linux][hz6] preload output: ${PRELOAD_SO}"
