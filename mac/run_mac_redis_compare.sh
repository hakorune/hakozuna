#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${ROOT_DIR}/mac/bench_redis_workload_compare.c"
BUILD_DIR="${BUILD_DIR:-/tmp/hakozuna-mac-redis}"
BIN="${BUILD_DIR}/bench_redis_workload_compare"
CC="${CC:-clang}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_redis_compare.sh [allocator|system|/abs/path/to/lib.dylib] [threads cycles ops_per_cycle min_size max_size]

Examples:
  ./mac/run_mac_redis_compare.sh system 4 500 2000 16 256
  ./mac/run_mac_redis_compare.sh hz3 4 500 2000 16 256
  ./mac/run_mac_redis_compare.sh mimalloc 4 500 2000 16 256
EOF
}

ALLOCATOR="${1:-system}"
if [[ "${ALLOCATOR}" == "--help" || "${ALLOCATOR}" == "-h" ]]; then
  usage
  exit 0
fi
if [[ $# -gt 0 ]]; then
  shift
fi
if [[ $# -eq 0 ]]; then
  set -- 4 500 2000 16 256
fi

mkdir -p "${BUILD_DIR}"

if [[ ! -x "${BIN}" || "${SRC}" -nt "${BIN}" ]]; then
  "${CC}" -O2 -std=c11 -pthread -Wall -Wextra -o "${BIN}" "${SRC}"
fi

PRELOAD=""
case "${ALLOCATOR}" in
  system)
    PRELOAD=""
    ;;
  hz3|hz4|mimalloc|tcmalloc|/*)
    # Reuse the shared allocator lookup logic.
    source "${ROOT_DIR}/bench/lib/bench_common.sh"
    PRELOAD="$(bench_find_allocator_library "${ALLOCATOR}")"
    if [[ -z "${PRELOAD}" ]]; then
      echo "allocator library not found for selector: ${ALLOCATOR}" >&2
      exit 1
    fi
    ;;
  *)
    echo "unknown allocator selector: ${ALLOCATOR}" >&2
    usage >&2
    exit 1
    ;;
esac

if [[ -n "${PRELOAD}" ]]; then
  echo "[mac] DYLD_INSERT_LIBRARIES=${PRELOAD}"
  exec env DYLD_INSERT_LIBRARIES="${PRELOAD}" "${BIN}" "$@"
fi

echo "[mac] CRT/system allocator"
exec "${BIN}" "$@"
