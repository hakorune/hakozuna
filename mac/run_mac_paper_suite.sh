#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,mimalloc,tcmalloc}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/mac/out/paper_suite_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILD="${SKIP_BUILD:-0}"
INCLUDE_SEGREG="${INCLUDE_SEGREG:-0}"
REDIS_ARGS=(4 500 2000 16 256)
LARSON_ARGS=(5 4096 32768 1000 1000 0 4)
MT_REMOTE_ARGS=(4 2000000 400 16 2048 50 262144)
MT_REMOTE_SEGREG_ARGS=(16 2000000 400 16 2048 90 262144)
MIMALLOC_BENCH_RUNS="${MIMALLOC_BENCH_RUNS:-3}"

usage() {
  cat <<'EOF'
Usage:
  ./mac/run_mac_paper_suite.sh [options]

Options:
  --allocators LIST     Comma-separated allocator list (default: system,hz3,hz4,mimalloc,tcmalloc)
  --out-dir DIR         Output directory for logs (default: mac/out/paper_suite_<timestamp>)
  --skip-build          Skip the Mac release build step
  --include-segreg      Also run the current high-remote segment-registry MT lane
  --help                Show this message
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
      shift 2
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --include-segreg)
      INCLUDE_SEGREG=1
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

mkdir -p "${OUT_DIR}"

log="${OUT_DIR}/paper_suite.log"
{
  echo "[paper-suite] ts=$(date +%Y-%m-%dT%H:%M:%S%z)"
  echo "[paper-suite] allocators=${ALLOCATORS}"
  echo "[paper-suite] skip_build=${SKIP_BUILD}"
  echo "[paper-suite] include_segreg=${INCLUDE_SEGREG}"
  echo "[paper-suite] out_dir=${OUT_DIR}"
  echo "[paper-suite] root_dir=${ROOT_DIR}"
  echo ""
} | tee "${log}"

if [[ "${SKIP_BUILD}" != "1" ]]; then
  echo "[paper-suite] building Mac release lane" | tee -a "${log}"
  "${ROOT_DIR}/mac/build_mac_release_lane.sh" 2>&1 | tee -a "${log}"
  echo "" | tee -a "${log}"
fi

run_larson() {
  echo "[paper-suite] larson canonical" | tee -a "${log}"
  ALLOCATORS="${ALLOCATORS}" \
    "${ROOT_DIR}/mac/run_mac_larson_compare.sh" \
    --out-dir "${OUT_DIR}/larson" \
    "${LARSON_ARGS[@]}" 2>&1 | tee -a "${log}"
  echo "" | tee -a "${log}"
}

run_mt_remote() {
  echo "[paper-suite] mt_remote canonical" | tee -a "${log}"
  ALLOCATORS="${ALLOCATORS}" \
    "${ROOT_DIR}/mac/run_mac_mt_remote_compare.sh" \
    --out-dir "${OUT_DIR}/mt_remote" \
    "${MT_REMOTE_ARGS[@]}" 2>&1 | tee -a "${log}"
  echo "" | tee -a "${log}"
}

run_redis() {
  echo "[paper-suite] redis-like" | tee -a "${log}"
  IFS=',' read -r -a allocator_list <<< "${ALLOCATORS}"
  for alloc in "${allocator_list[@]}"; do
    local redis_log="${OUT_DIR}/redis_${alloc}.log"
    echo "[paper-suite] redis alloc=${alloc}" | tee -a "${log}"
    "${ROOT_DIR}/mac/run_mac_redis_compare.sh" "${alloc}" "${REDIS_ARGS[@]}" \
      > "${redis_log}" 2>&1
    tail -n 20 "${redis_log}" | sed 's/^/[paper-suite][redis] /' | tee -a "${log}"
    echo "" | tee -a "${log}"
  done
}

run_mimalloc_bench() {
  echo "[paper-suite] mimalloc-bench subset" | tee -a "${log}"
  ALLOCATORS="${ALLOCATORS}" \
    "${ROOT_DIR}/mac/run_mac_mimalloc_bench_subset.sh" \
    --runs "${MIMALLOC_BENCH_RUNS}" \
    --out-dir "${OUT_DIR}/mimalloc_bench_subset" 2>&1 | tee -a "${log}"
  echo "" | tee -a "${log}"
}

run_mt_remote_segreg() {
  echo "[paper-suite] mt_remote segment-registry high-remote" | tee -a "${log}"
  ALLOCATORS=hz4 \
    "${ROOT_DIR}/mac/run_mac_mt_remote_compare.sh" \
    --out-dir "${OUT_DIR}/mt_remote_segreg" \
    "${MT_REMOTE_SEGREG_ARGS[@]}" 2>&1 | tee -a "${log}"
  echo "" | tee -a "${log}"
}

run_redis
run_larson
run_mt_remote
run_mimalloc_bench

if [[ "${INCLUDE_SEGREG}" == "1" ]]; then
  run_mt_remote_segreg
fi

echo "[paper-suite] done; logs in ${OUT_DIR}" | tee -a "${log}"
