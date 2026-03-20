#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
ALLOCATORS="system,hz3,hz4,mimalloc,tcmalloc"
BENCH_ARGS="4 1000000 8192 16 1024"
RUNS=3
OUTDIR="${ROOT_DIR}/private/raw-results/linux/compare_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0
SKIP_PREPARE_ALLOCATORS=0

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_bench_compare.sh [options]

Options:
  --arch <arch>                 override detected arch (default: auto)
  --allocators LIST             comma-separated allocator list
  --bench-args ARGS             benchmark arguments passed to the benchmark binary
  --runs N                      number of runs per allocator
  --outdir DIR                  output directory for logs
  --skip-build                  skip the benchmark binary build step
  --skip-prepare-allocators     skip local mimalloc/tcmalloc cache preparation
  --help                        show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --allocators)
      [[ $# -ge 2 ]] || { echo "missing value for --allocators" >&2; exit 1; }
      ALLOCATORS="$2"
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
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --skip-prepare-allocators)
      SKIP_PREPARE_ALLOCATORS=1
      shift
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

BENCH_BIN="${BENCH_BIN:-${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt}"

mkdir -p "$OUTDIR"

echo "[linux] arch: $ARCH"
echo "[linux] bench_bin: $BENCH_BIN"
echo "[linux] bench_args: $BENCH_ARGS"
echo "[linux] allocators: $ALLOCATORS"
echo "[linux] runs: $RUNS"
echo "[linux] outdir: $OUTDIR"

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  eval "$("${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH")"
fi

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

exec "${ROOT_DIR}/bench/run_compare.sh" \
  --allocators "$ALLOCATORS" \
  --bench-bin "$BENCH_BIN" \
  --bench-args "$BENCH_ARGS" \
  --runs "$RUNS" \
  --outdir "$OUTDIR"
