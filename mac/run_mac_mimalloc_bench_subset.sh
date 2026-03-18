#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/bench/lib/bench_common.sh"

RUNS="${RUNS:-3}"
SKIP_BUILD="${SKIP_BUILD:-0}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,mimalloc,tcmalloc}"

PROCS="${PROCS:-}"
if [[ -z "${PROCS}" ]]; then
  if command -v sysctl >/dev/null 2>&1; then
    PROCS="$(sysctl -n hw.ncpu 2>/dev/null || echo 8)"
  else
    PROCS="8"
  fi
fi

DO_CACHE_THRASH="${DO_CACHE_THRASH:-1}"
DO_CACHE_SCRATCH="${DO_CACHE_SCRATCH:-1}"
DO_MALLOC_LARGE="${DO_MALLOC_LARGE:-1}"

CACHE_ITERS="${CACHE_ITERS:-200}"
CACHE_OBJSIZE="${CACHE_OBJSIZE:-1}"
CACHE_REPS="${CACHE_REPS:-2000000}"
CACHE_CONCURRENCY="${CACHE_CONCURRENCY:-${PROCS}}"
CACHE_THREADS_LIST="${CACHE_THREADS_LIST:-1 ${PROCS}}"

BUILD_DIR="${BUILD_DIR:-/tmp/mimalloc_bench_build_subset_mac}"
BENCH_SRC_DIR="${BENCH_SRC_DIR:-/Volumes/PublicShare/hakmem/mimalloc-bench/bench}"

TS="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${LOG_DIR:-/tmp/mimalloc_bench_subset_mac_${TS}}"
OUTDIR="${OUTDIR:-}"

CC="${CC:-clang}"
CXX="${CXX:-clang++}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_mimalloc_bench_subset.sh [options]

Options:
  --allocators LIST   Comma-separated allocator list (default: system,hz3,hz4,mimalloc,tcmalloc)
  --runs N            Number of runs per case (default: 3)
  --bench-src-dir DIR mimalloc-bench/bench path
  --build-dir DIR     Build output directory
  --out-dir DIR       Copy logs to DIR after run
  --skip-build        Skip bench compilation
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
    --runs)
      [[ $# -ge 2 ]] || { echo "missing value for --runs" >&2; exit 1; }
      RUNS="$2"
      shift 2
      ;;
    --bench-src-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --bench-src-dir" >&2; exit 1; }
      BENCH_SRC_DIR="$2"
      shift 2
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --build-dir" >&2; exit 1; }
      BUILD_DIR="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --out-dir" >&2; exit 1; }
      OUTDIR="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
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
      echo "unexpected argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ ! -d "${BENCH_SRC_DIR}" ]]; then
  echo "[mimalloc-bench] missing bench source dir: ${BENCH_SRC_DIR}" >&2
  exit 2
fi

mkdir -p "${BUILD_DIR}" "${LOG_DIR}"

ns_now() {
  python3 - <<'PY'
import time
print(time.time_ns())
PY
}

copy_dir_to_outdir() {
  local d="$1"
  if [[ -n "${OUTDIR}" ]]; then
    mkdir -p "${OUTDIR}"
    rm -rf "${OUTDIR}/$(basename "${d}")"
    cp -a "${d}" "${OUTDIR}/"
  fi
}

write_readme() {
  {
    echo "[BENCH-HEADER] mimalloc-bench subset (mac)"
    echo "ts=${TS} runs=${RUNS} procs=${PROCS} skip_build=${SKIP_BUILD}"
    echo "allocators=${ALLOCATORS}"
    echo "build_dir=${BUILD_DIR} bench_src_dir=${BENCH_SRC_DIR}"
    echo "do_cache_thrash=${DO_CACHE_THRASH} do_cache_scratch=${DO_CACHE_SCRATCH} do_malloc_large=${DO_MALLOC_LARGE}"
    echo "cache_threads_list=${CACHE_THREADS_LIST}"
    echo "cache_args: iters=${CACHE_ITERS} obj_size=${CACHE_OBJSIZE} reps=${CACHE_REPS} concurrency=${CACHE_CONCURRENCY}"
    echo "[BENCH-SYS] uname=$(uname -a)"
  } | tee "${LOG_DIR}/README.log"
}

build_benches() {
  local cxxflags="-O3 -DNDEBUG -std=c++17 -pthread"
  if [[ "${DO_CACHE_THRASH}" == "1" ]]; then
    "${CXX}" ${cxxflags} -o "${BUILD_DIR}/cache-thrash" \
      "${BENCH_SRC_DIR}/cache-thrash/cache-thrash.cpp"
  fi
  if [[ "${DO_CACHE_SCRATCH}" == "1" ]]; then
    "${CXX}" ${cxxflags} -o "${BUILD_DIR}/cache-scratch" \
      "${BENCH_SRC_DIR}/cache-scratch/cache-scratch.cpp"
  fi
  if [[ "${DO_MALLOC_LARGE}" == "1" ]]; then
    "${CXX}" ${cxxflags} -o "${BUILD_DIR}/malloc-large" \
      "${BENCH_SRC_DIR}/malloc-large/malloc-large.cpp"
  fi
}

run_one_timed() {
  local label="$1"
  local alloc="$2"
  shift 2
  local bin="$1"
  shift 1

  local lib_path=""
  if [[ "${alloc}" != "system" ]]; then
    lib_path="$(bench_find_allocator_library "${alloc}" || true)"
    if [[ -z "${lib_path}" ]]; then
      echo "[SKIP] ${label} (missing lib for ${alloc})"
      return 0
    fi
  fi

  local start end dt_ns
  start="$(ns_now)"
  if [[ "${alloc}" == "system" ]]; then
    "${bin}" "$@" >/dev/null 2>&1
  else
    bench_run_with_allocator "${alloc}" "${lib_path}" "${bin}" "$@" >/dev/null 2>&1
  fi
  end="$(ns_now)"
  dt_ns="$((end - start))"
  python3 - <<PY
label="${label}"
dt_ns=${dt_ns}
print(f"[BENCH] {label} time_ns={dt_ns}")
PY
}

run_cache_bench() {
  local bench_name="$1"
  local bin="${BUILD_DIR}/${bench_name}"
  if [[ ! -x "${bin}" ]]; then
    echo "[ERROR] bench not found: ${bin}" >&2
    exit 1
  fi

  local log="${LOG_DIR}/${bench_name}.log"
  {
    echo "[BENCH-HEADER] bench=${bench_name} runs=${RUNS}"
    echo "[BENCH-ARGS] iters=${CACHE_ITERS} obj_size=${CACHE_OBJSIZE} reps=${CACHE_REPS} concurrency=${CACHE_CONCURRENCY} threads_list=${CACHE_THREADS_LIST}"
    echo ""
  } > "${log}"

  local t name
  for t in ${CACHE_THREADS_LIST}; do
    for name in system hz3 hz4 mimalloc tcmalloc; do
      if [[ ",${ALLOCATORS}," != *",${name},"* ]]; then
        continue
      fi
      for i in $(seq 1 "${RUNS}"); do
        run_one_timed "name=${name} bench=${bench_name} threads=${t} run=${i}/${RUNS}" "${name}" "${bin}" \
          "${t}" "${CACHE_ITERS}" "${CACHE_OBJSIZE}" "${CACHE_REPS}" "${CACHE_CONCURRENCY}" \
          >> "${log}"
      done
    done
  done
}

run_malloc_large() {
  local bench_name="malloc-large"
  local bin="${BUILD_DIR}/${bench_name}"
  if [[ ! -x "${bin}" ]]; then
    echo "[ERROR] bench not found: ${bin}" >&2
    exit 1
  fi

  local log="${LOG_DIR}/${bench_name}.log"
  {
    echo "[BENCH-HEADER] bench=${bench_name} runs=${RUNS}"
    echo ""
  } > "${log}"

  local name
  for name in system hz3 hz4 mimalloc tcmalloc; do
    if [[ ",${ALLOCATORS}," != *",${name},"* ]]; then
      continue
    fi
    for i in $(seq 1 "${RUNS}"); do
      run_one_timed "name=${name} bench=${bench_name} run=${i}/${RUNS}" "${name}" "${bin}" \
        >> "${log}"
    done
  done
}

write_readme

if [[ "${SKIP_BUILD}" != "1" ]]; then
  build_benches
fi

if [[ "${DO_CACHE_THRASH}" == "1" ]]; then
  run_cache_bench "cache-thrash"
fi
if [[ "${DO_CACHE_SCRATCH}" == "1" ]]; then
  run_cache_bench "cache-scratch"
fi
if [[ "${DO_MALLOC_LARGE}" == "1" ]]; then
  run_malloc_large
fi

copy_dir_to_outdir "${LOG_DIR}"

echo "logs: ${LOG_DIR}"
copy_dir_to_outdir "${LOG_DIR}"
