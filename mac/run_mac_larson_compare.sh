#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

SRC="${ROOT_DIR}/mac/bench_larson_compare.c"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/mac/out}"
BIN="${BIN:-${OUT_DIR}/bench_larson_compare}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,mimalloc,tcmalloc}"
BUILD_ONLY="${BUILD_ONLY:-0}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_larson_compare.sh [options] [runtime_sec] [min_size] [max_size] [chunks_per_thread] [rounds] [seed] [threads]

Options:
  --allocators LIST   Comma-separated allocator list (default: system,hz3,hz4,mimalloc,tcmalloc)
  --out-dir DIR       Build/output directory (default: mac/out)
  --bin PATH          Benchmark binary path
  --build-only        Build the benchmark binary and stop
  --help              Show this message
EOF
}

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
      BIN="${OUT_DIR}/bench_larson_compare"
      shift 2
      ;;
    --bin)
      [[ $# -ge 2 ]] || { echo "missing value for --bin" >&2; exit 1; }
      BIN="$2"
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

mkdir -p "${OUT_DIR}"

clang -std=c11 -O2 -pthread "${SRC}" -o "${BIN}"

if [[ "${BUILD_ONLY}" == "1" ]]; then
  echo "[larson] built ${BIN}"
  exit 0
fi

if [[ $# -eq 0 ]]; then
  set -- 3 8 1024 10000 1 12345 4
fi

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"

run_case() {
  local alloc="$1"
  shift
  local log="${OUT_DIR}/larson_${alloc}.log"
  local lib_path=""
  if [[ "${alloc}" != "system" ]]; then
    lib_path="$(bench_find_allocator_library "${alloc}" || true)"
    if [[ -z "${lib_path}" ]]; then
      echo "[larson] missing allocator library for ${alloc}" >&2
      bench_print_allocator_hints "${alloc}"
      exit 2
    fi
  fi

  {
    echo "[larson] alloc=${alloc}"
    echo "[larson] bin=${BIN}"
    echo "[larson] args=$*"
    [[ -n "${lib_path}" ]] && echo "[larson] lib=${lib_path}"
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

echo "[larson] logs in ${OUT_DIR}"
