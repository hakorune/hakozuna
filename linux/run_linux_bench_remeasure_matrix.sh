#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
RUN_COMPARE=1
RUN_HZ6=1
SKIP_COMPARE_BUILD=0
SKIP_HZ6_BUILD=0
COMPARE_OUTDIR=""
HZ6_OUTDIR=""
COMPARE_RUNS=3
COMPARE_BENCH_ARGS=""
COMPARE_ALLOCATORS="system,hz3,hz4,hz5,mimalloc,tcmalloc"
HZ6_RUNS=5
HZ6_LOCAL_ITERS=1000000
HZ6_REMOTE_ITERS=200000
HZ6_REUSE_ITERS=100000
HZ6_LOCAL_SIZES="8192,65536"
HZ6_REMOTE_SIZES="131072"
HZ6_REUSE_SIZES="131072"
HZ6_LOCAL_PROFILES="strict,speed,rss,remote"
HZ6_REMOTE_PROFILES="speed,rss,remote"
HZ6_REUSE_PROFILES="speed,rss,remote"

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_bench_remeasure_matrix.sh [options]

Options:
  --arch <arch>                  override detected arch (default: auto)
  --skip-compare                 skip the hz3/hz4/hz5/mimalloc/tcmalloc compare matrix
  --skip-hz6                     skip the HZ6 standalone matrix
  --skip-compare-build           reuse existing compare allocator builds
  --skip-hz6-build               reuse existing HZ6 benchmark binary
  --compare-runs N               compare matrix runs per allocator
  --compare-bench-args ARGS      compare benchmark arguments
  --compare-allocators LIST      compare allocator list
  --compare-outdir DIR           compare matrix output directory
  --hz6-runs N                   HZ6 matrix runs per mode/profile/size
  --hz6-local-iters N            HZ6 local iterations
  --hz6-remote-iters N           HZ6 remote iterations
  --hz6-reuse-iters N            HZ6 reuse iterations
  --hz6-local-sizes LIST         HZ6 local size list
  --hz6-remote-sizes LIST        HZ6 remote size list
  --hz6-reuse-sizes LIST         HZ6 reuse size list
  --hz6-local-profiles LIST      HZ6 local profile list
  --hz6-remote-profiles LIST     HZ6 remote profile list
  --hz6-reuse-profiles LIST      HZ6 reuse profile list
  --hz6-outdir DIR               HZ6 matrix output directory
  --help                         show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --skip-compare)
      RUN_COMPARE=0
      shift
      ;;
    --skip-hz6)
      RUN_HZ6=0
      shift
      ;;
    --skip-compare-build)
      SKIP_COMPARE_BUILD=1
      shift
      ;;
    --skip-hz6-build)
      SKIP_HZ6_BUILD=1
      shift
      ;;
    --compare-runs)
      [[ $# -ge 2 ]] || { echo "missing value for --compare-runs" >&2; exit 1; }
      COMPARE_RUNS="$2"
      shift 2
      ;;
    --compare-bench-args)
      [[ $# -ge 2 ]] || { echo "missing value for --compare-bench-args" >&2; exit 1; }
      COMPARE_BENCH_ARGS="$2"
      shift 2
      ;;
    --compare-allocators)
      [[ $# -ge 2 ]] || { echo "missing value for --compare-allocators" >&2; exit 1; }
      COMPARE_ALLOCATORS="$2"
      shift 2
      ;;
    --compare-outdir)
      [[ $# -ge 2 ]] || { echo "missing value for --compare-outdir" >&2; exit 1; }
      COMPARE_OUTDIR="$2"
      shift 2
      ;;
    --hz6-runs)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-runs" >&2; exit 1; }
      HZ6_RUNS="$2"
      shift 2
      ;;
    --hz6-local-iters)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-local-iters" >&2; exit 1; }
      HZ6_LOCAL_ITERS="$2"
      shift 2
      ;;
    --hz6-remote-iters)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-remote-iters" >&2; exit 1; }
      HZ6_REMOTE_ITERS="$2"
      shift 2
      ;;
    --hz6-reuse-iters)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-reuse-iters" >&2; exit 1; }
      HZ6_REUSE_ITERS="$2"
      shift 2
      ;;
    --hz6-local-sizes)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-local-sizes" >&2; exit 1; }
      HZ6_LOCAL_SIZES="$2"
      shift 2
      ;;
    --hz6-remote-sizes)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-remote-sizes" >&2; exit 1; }
      HZ6_REMOTE_SIZES="$2"
      shift 2
      ;;
    --hz6-reuse-sizes)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-reuse-sizes" >&2; exit 1; }
      HZ6_REUSE_SIZES="$2"
      shift 2
      ;;
    --hz6-local-profiles)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-local-profiles" >&2; exit 1; }
      HZ6_LOCAL_PROFILES="$2"
      shift 2
      ;;
    --hz6-remote-profiles)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-remote-profiles" >&2; exit 1; }
      HZ6_REMOTE_PROFILES="$2"
      shift 2
      ;;
    --hz6-reuse-profiles)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-reuse-profiles" >&2; exit 1; }
      HZ6_REUSE_PROFILES="$2"
      shift 2
      ;;
    --hz6-outdir)
      [[ $# -ge 2 ]] || { echo "missing value for --hz6-outdir" >&2; exit 1; }
      HZ6_OUTDIR="$2"
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

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

echo "[linux] arch: $ARCH"

if [[ "$RUN_COMPARE" -eq 1 ]]; then
  compare_args=(
    --arch "$ARCH"
    --runs "$COMPARE_RUNS"
    --allocators "$COMPARE_ALLOCATORS"
  )
  if [[ -n "$COMPARE_BENCH_ARGS" ]]; then
    compare_args+=(--bench-args "$COMPARE_BENCH_ARGS")
  fi
  if [[ -n "$COMPARE_OUTDIR" ]]; then
    compare_args+=(--outdir "$COMPARE_OUTDIR")
  fi
  if [[ "$SKIP_COMPARE_BUILD" -eq 1 ]]; then
    compare_args+=(--skip-build)
  fi
  echo "[linux] compare matrix"
  "${ROOT_DIR}/linux/run_linux_bench_compare_matrix.sh" "${compare_args[@]}"
fi

if [[ "$RUN_HZ6" -eq 1 ]]; then
  hz6_args=(
    --arch "$ARCH"
    --runs "$HZ6_RUNS"
    --local-iters "$HZ6_LOCAL_ITERS"
    --remote-iters "$HZ6_REMOTE_ITERS"
    --reuse-iters "$HZ6_REUSE_ITERS"
    --local-sizes "$HZ6_LOCAL_SIZES"
    --remote-sizes "$HZ6_REMOTE_SIZES"
    --reuse-sizes "$HZ6_REUSE_SIZES"
    --local-profiles "$HZ6_LOCAL_PROFILES"
    --remote-profiles "$HZ6_REMOTE_PROFILES"
    --reuse-profiles "$HZ6_REUSE_PROFILES"
  )
  if [[ -n "$HZ6_OUTDIR" ]]; then
    hz6_args+=(--outdir "$HZ6_OUTDIR")
  fi
  if [[ "$SKIP_HZ6_BUILD" -eq 1 ]]; then
    hz6_args+=(--skip-build)
  fi
  echo "[linux] hz6 matrix"
  "${ROOT_DIR}/linux/run_linux_hz6_benchmark.sh" "${hz6_args[@]}"
fi
