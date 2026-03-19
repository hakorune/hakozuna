#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

SRC="${ROOT_DIR}/mac/malloc_size_probe.c"
BUILD_DIR="${BUILD_DIR:-/tmp/hakozuna-mac-malloc-size-probe}"
BIN="${BIN:-${BUILD_DIR}/malloc_size_probe}"
ALLOCATOR="${1:-hz3}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_malloc_size_probe.sh [allocator|system|/abs/path/to/lib.dylib]

Examples:
  ./mac/run_mac_malloc_size_probe.sh hz3
  ./mac/run_mac_malloc_size_probe.sh hz4
  ./mac/run_mac_malloc_size_probe.sh system
EOF
}

if [[ "${ALLOCATOR}" == "--help" || "${ALLOCATOR}" == "-h" ]]; then
  usage
  exit 0
fi

mkdir -p "${BUILD_DIR}"

if [[ ! -x "${BIN}" || "${SRC}" -nt "${BIN}" ]]; then
  clang -std=c11 -O2 -Wall -Wextra -Werror -pthread "${SRC}" -o "${BIN}"
fi

if [[ "${ALLOCATOR}" == "hz4" ]]; then
  export HKO_SIZE_SYMBOL="${HKO_SIZE_SYMBOL:-hz4_macos_debug_malloc_size}"
  if [[ -z "${HZ4_SO:-}" ]]; then
    if [[ -f "${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4_obs.so" ]]; then
      export HZ4_SO="${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4_obs.so"
    fi
  fi
fi

if [[ "${ALLOCATOR}" == "hz4" ]]; then
  export HKO_FOREIGN_MODE="${HKO_FOREIGN_MODE:-mmap}"
else
  export HKO_FOREIGN_MODE="${HKO_FOREIGN_MODE:-system}"
fi

PRELOAD=""
case "${ALLOCATOR}" in
  system)
    ;;
  hz3|hz4|mimalloc|tcmalloc|/*)
    PRELOAD="$(bench_find_allocator_library "${ALLOCATOR}")"
    if [[ -z "${PRELOAD}" ]]; then
      echo "[probe] allocator library not found for selector: ${ALLOCATOR}" >&2
      bench_print_allocator_hints "${ALLOCATOR}"
      exit 1
    fi
    ;;
  *)
    echo "[probe] unknown allocator selector: ${ALLOCATOR}" >&2
    usage >&2
    exit 1
    ;;
esac

log="${BUILD_DIR}/malloc_size_probe_${ALLOCATOR}.log"
{
  echo "[probe] allocator=${ALLOCATOR}"
  echo "[probe] bin=${BIN}"
  if [[ -n "${PRELOAD}" ]]; then
    echo "[probe] preload=${PRELOAD}"
    DYLD_INSERT_LIBRARIES="${PRELOAD}" "${BIN}"
  else
    "${BIN}"
  fi
} 2>&1 | tee "${log}"

if [[ "${ALLOCATOR}" != "system" ]]; then
  if [[ "${ALLOCATOR}" == "hz3" ]]; then
    grep -Eq '\[probe\] mode=foreign kind=system requested=144 size=[1-9][0-9]*' "${log}"
    if rg -q '\[HZ3_MACOS_MALLOC_SIZE\]' "${log}"; then
      grep -Eq '\[HZ3_MACOS_MALLOC_SIZE\].*hz3_hit=[1-9][0-9]* .*arena_unknown=0 .*zero_return=0' "${log}"
    fi
  elif [[ "${ALLOCATOR}" == "hz4" ]]; then
    grep -Eq '\[probe\] mode=hakozuna requested=96 size=[1-9][0-9]*' "${log}"
    grep -Eq '\[probe\] mode=foreign kind=mmap requested=[0-9]+ size=[0-9]+' "${log}"
    if rg -q '\[HZ4_MACOS_MALLOC_SIZE\]' "${log}"; then
      grep -Eq '\[HZ4_MACOS_MALLOC_SIZE\].*hz4_hit=[1-9][0-9]* .*system_fallback=[1-9][0-9]* .*segment_unknown=0' "${log}"
    fi
  fi
fi

echo "[probe] log=${log}"
