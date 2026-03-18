#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,mimalloc,tcmalloc}"
BENCH_BIN="${BENCH_BIN:-${ROOT_DIR}/hakozuna/out/bench_random_mixed_malloc_args}"
BENCH_ARGS="${BENCH_ARGS:-20000000 400 16 2048}"
RUNS="${RUNS:-3}"
RUN_FROM="${RUN_FROM:-1}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/bench_results/compare_$(date +%Y%m%d_%H%M%S)}"

usage() {
  cat <<'EOF'
Usage:
  ./bench/run_compare.sh [options]

Options:
  --allocators LIST   Comma-separated allocator list
  --bench-bin PATH    Benchmark binary
  --bench-args ARGS   Benchmark arguments
  --runs N            Number of runs per allocator (default: 3)
  --outdir DIR        Output directory
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
    --bench-bin)
      [[ $# -ge 2 ]] || { echo "missing value for --bench-bin" >&2; exit 1; }
      BENCH_BIN="$2"
      shift 2
      ;;
    --bench-args)
      [[ $# -ge 2 ]] || { echo "missing value for --bench-args" >&2; exit 1; }
      BENCH_ARGS="$2"
      shift 2
      ;;
    --runs)
      [[ $# -ge 2 ]] || { echo "missing value for --runs" >&2; exit 1; }
      RUNS="$2"
      shift 2
      ;;
    --outdir)
      [[ $# -ge 2 ]] || { echo "missing value for --outdir" >&2; exit 1; }
      OUTDIR="$2"
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

mkdir -p "${OUTDIR}"

if [[ ! -x "${BENCH_BIN}" ]]; then
  echo "[ERR] benchmark binary not found or not executable: ${BENCH_BIN}" >&2
  echo "hint: build the allocator bench binary first, then rerun this wrapper" >&2
  exit 2
fi

IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"
read -r -a bench_args <<< "${BENCH_ARGS}"

TS="$(date +%Y%m%d_%H%M%S)"
{
  echo "[BENCH] ts=${TS}"
  echo "[BENCH] root=${ROOT_DIR}"
  echo "[BENCH] os=$(bench_os_name)"
  echo "[BENCH] bench_bin=${BENCH_BIN}"
  echo "[BENCH] bench_args=${BENCH_ARGS}"
  echo "[BENCH] runs=${RUNS} run_from=${RUN_FROM}"
  echo "[BENCH] allocators=${ALLOCATORS}"
  echo ""
} | tee "${OUTDIR}/README.log"

for alloc in "${allocator_list[@]}"; do
  lib_path="$(bench_find_allocator_library "${alloc}" || true)"
  if [[ "${alloc}" != "system" && -z "${lib_path}" ]]; then
    echo "[ERR] missing allocator library for ${alloc}" >&2
    bench_print_allocator_hints "${alloc}"
    exit 2
  fi
  if [[ "${alloc}" != "system" ]]; then
    echo "[BENCH] ${alloc} -> ${lib_path}" | tee -a "${OUTDIR}/README.log"
  else
    echo "[BENCH] ${alloc} -> system allocator" | tee -a "${OUTDIR}/README.log"
  fi
done

run_case() {
  local alloc="$1"
  local lib_path="$2"
  local run_id="$3"
  local log="${OUTDIR}/${run_id}_${alloc}.log"

  {
    echo "[CASE] alloc=${alloc}"
    echo "[CASE] lib=${lib_path:-system}"
    echo "[CASE] cmd=${BENCH_BIN} ${BENCH_ARGS}"
    echo ""
  } | tee "${log}"

  if [[ "${alloc}" == "system" ]]; then
    "${BENCH_BIN}" "${bench_args[@]}" 2>&1 | tee -a "${log}"
  else
    bench_run_with_allocator "${alloc}" "${lib_path}" "${BENCH_BIN}" "${bench_args[@]}" 2>&1 | tee -a "${log}"
  fi
}

for run_id in $(seq "${RUN_FROM}" "${RUNS}"); do
  for alloc in "${allocator_list[@]}"; do
    lib_path="$(bench_find_allocator_library "${alloc}" || true)"
    run_case "${alloc}" "${lib_path}" "${run_id}"
  done
done

echo "[DONE] logs saved to ${OUTDIR}"
