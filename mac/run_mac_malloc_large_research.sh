#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BENCH_SRC_DIR="${BENCH_SRC_DIR:-/Volumes/PublicShare/hakmem/mimalloc-bench/bench}"
BUILD_DIR="${BUILD_DIR:-/tmp/mimalloc_bench_build_subset_mac}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/mac/out/malloc_large_research_$(date +%Y%m%d_%H%M%S)}"
RUNS="${RUNS:-5}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,mimalloc,tcmalloc}"
SKIP_BUILD="${SKIP_BUILD:-0}"
SKIP_OBSERVE_BUILD="${SKIP_OBSERVE_BUILD:-0}"
HZ4_OBSERVE_LIB="${HZ4_OBSERVE_LIB:-${ROOT_DIR}/mac/out/observe/libhakozuna_hz4_malloc_large_obs.so}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_malloc_large_research.sh [options]

Options:
  --allocators LIST   Comma-separated allocator list (default: system,hz3,hz4,mimalloc,tcmalloc)
  --runs N            Number of runs per allocator (default: 5)
  --bench-src-dir DIR mimalloc-bench/bench path
  --build-dir DIR     Build output directory for the benchmark binary
  --out-dir DIR       Output directory for logs
  --skip-build        Skip benchmark compilation in the subset runner
  --skip-observe-build
                      Reuse an existing stats-enabled hz4 observe library
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
      OUT_DIR="$2"
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --skip-observe-build)
      SKIP_OBSERVE_BUILD=1
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
  echo "[malloc-large] missing bench source dir: ${BENCH_SRC_DIR}" >&2
  exit 2
fi

mkdir -p "${OUT_DIR}"

"${ROOT_DIR}/mac/check_mac_env.sh"

if [[ "${SKIP_OBSERVE_BUILD}" != "1" ]]; then
  echo "[mac] building stats-enabled hz4 observe lib -> ${HZ4_OBSERVE_LIB}"
  "${ROOT_DIR}/mac/build_mac_observe_lane.sh" \
    --skip-hz3 \
    --hz4-os-stats \
    --hz4-route-stats \
    --hz4-lib "${HZ4_OBSERVE_LIB}"
fi

if [[ -z "${HZ4_SO:-}" ]]; then
  HZ4_SO="${HZ4_OBSERVE_LIB}"
fi

echo "[mac] malloc-large research box"
echo "[mac] allocators: ${ALLOCATORS}"
echo "[mac] runs: ${RUNS}"
echo "[mac] out_dir: ${OUT_DIR}"
echo "[mac] bench_src_dir: ${BENCH_SRC_DIR}"
echo "[mac] hz4_so: ${HZ4_SO}"

DO_CACHE_THRASH=0 \
DO_CACHE_SCRATCH=0 \
DO_MALLOC_LARGE=1 \
CAPTURE_ALLOCATOR_OUTPUT=1 \
HZ4_SO="${HZ4_SO}" \
SKIP_BUILD="${SKIP_BUILD}" \
ALLOCATORS="${ALLOCATORS}" \
RUNS="${RUNS}" \
BENCH_SRC_DIR="${BENCH_SRC_DIR}" \
BUILD_DIR="${BUILD_DIR}" \
OUTDIR="${OUT_DIR}" \
"${ROOT_DIR}/mac/run_mac_mimalloc_bench_subset.sh" \
  --capture-allocator-output \
  --allocators "${ALLOCATORS}" \
  --runs "${RUNS}" \
  --bench-src-dir "${BENCH_SRC_DIR}" \
  --build-dir "${BUILD_DIR}" \
  --out-dir "${OUT_DIR}"

echo "[mac] malloc-large research logs: ${OUT_DIR}"
