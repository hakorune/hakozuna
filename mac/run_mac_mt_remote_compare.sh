#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

SRC="${SRC:-/Volumes/PublicShare/hakmem/hakozuna/bench/bench_random_mixed_mt_remote.c}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/mac/out}"
BIN="${BIN:-${OUT_DIR}/bench_random_mixed_mt_remote_malloc}"
CC="${CC:-clang}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,mimalloc,tcmalloc}"
BUILD_ONLY="${BUILD_ONLY:-0}"
SELECTOR="${SELECTOR:-}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_mt_remote_compare.sh [options] [threads iters ws min max remote_pct [ring_slots]]

Options:
  --allocators LIST   Comma-separated allocator list (default: system,hz3,hz4,mimalloc,tcmalloc)
  --out-dir DIR       Build/output directory (default: mac/out)
  --bin PATH          Benchmark binary path
  --src PATH          Shared source path (default: /Volumes/PublicShare/hakmem/hakozuna/bench/bench_random_mixed_mt_remote.c)
  --build-only        Build the benchmark binary and stop
  --help              Show this message
EOF
}

if [[ $# -gt 0 ]]; then
  case "$1" in
    system|hz3|hz4|mimalloc|tcmalloc|/*)
      SELECTOR="$1"
      shift
      ;;
  esac
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    --allocators)
      [[ $# -ge 2 ]] || { echo "missing value for --allocators" >&2; exit 1; }
      ALLOCATORS="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --out-dir" >&2; exit 1; }
      OUT_DIR="$2"
      BIN="${OUT_DIR}/bench_random_mixed_mt_remote_malloc"
      shift 2
      ;;
    --bin)
      [[ $# -ge 2 ]] || { echo "missing value for --bin" >&2; exit 1; }
      BIN="$2"
      shift 2
      ;;
    --src)
      [[ $# -ge 2 ]] || { echo "missing value for --src" >&2; exit 1; }
      SRC="$2"
      shift 2
      ;;
    --build-only)
      BUILD_ONLY=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    --*)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

if [[ -n "${SELECTOR}" ]]; then
  ALLOCATORS="${SELECTOR}"
fi

mkdir -p "${OUT_DIR}"

if [[ ! -f "${SRC}" ]]; then
  echo "[mt-remote] source not found: ${SRC}" >&2
  exit 1
fi

if [[ ! -x "${BIN}" || "${SRC}" -nt "${BIN}" ]]; then
  "${CC}" \
    -O2 \
    -std=c11 \
    -pthread \
    -DHZ_BENCH_USE_HAKOZUNA=0 \
    -Wall \
    -Wextra \
    -o "${BIN}" \
    "${SRC}"
fi

if [[ "${BUILD_ONLY}" == "1" ]]; then
  echo "[mt-remote] built ${BIN}"
  exit 0
fi

if [[ $# -eq 0 ]]; then
  set -- 4 5000000 400 16 2048 90 65536
fi

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"

run_case() {
  local alloc="$1"
  shift
  local log="${OUT_DIR}/mt_remote_${alloc}.log"
  local lib_path=""

  if [[ "${alloc}" != "system" ]]; then
    lib_path="$(bench_find_allocator_library "${alloc}" || true)"
    if [[ -z "${lib_path}" ]]; then
      echo "[mt-remote] missing allocator library for ${alloc}" >&2
      bench_print_allocator_hints "${alloc}"
      exit 2
    fi
  fi

  {
    echo "[mt-remote] alloc=${alloc}"
    echo "[mt-remote] bin=${BIN}"
    echo "[mt-remote] args=$*"
    [[ -n "${lib_path}" ]] && echo "[mt-remote] lib=${lib_path}"
    echo ""
  } | tee "${log}"

  if [[ "${alloc}" == "system" ]]; then
    "${BIN}" "$@" 2>&1 | tee -a "${log}"
  else
    bench_run_with_allocator "${alloc}" "${lib_path}" "${BIN}" "$@" 2>&1 | tee -a "${log}"
  fi
}

for alloc in "${allocator_list[@]}"; do
  run_case "${alloc}" "$@"
done

echo "[mt-remote] logs in ${OUT_DIR}"
