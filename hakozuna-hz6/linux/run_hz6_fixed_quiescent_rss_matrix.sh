#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
WS="${WS:-4096}"
VARIANTS="${VARIANTS:-selected,selected_malloc_trim_before_rss,small_boundary_trusted_toy_map8192,small_boundary_trusted_toy_map8192_malloc_trim_before_rss}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_fixed_quiescent_rss_matrix_$(date +%Y%m%d_%H%M%S)}"
SKIP_BENCH_BUILD=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_fixed_quiescent_rss_matrix.sh [options]

Options:
  --arch ARCH       target arch (default: x86_64)
  --runs N          runs per variant/row (default: 3)
  --iters N         iterations per run (default: 200000)
  --ws N            working set (default: 4096)
  --variants CSV    variants passed to run_hz6_midpage_payload_trim_ab.sh
  --outdir DIR      output directory
  --skip-bench      reuse existing benchmark binary
  --help            show this message

This runner is FixedQuiescentRssMatrix-L1:
  - fixed_4k/fixed_8k/fixed_16k only
  - stats off for speed/current-RSS ranking
  - compares normal peak/current RSS with malloc_trim-before-RSS variants
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      ARCH="$2"
      shift 2
      ;;
    --runs)
      RUNS="$2"
      shift 2
      ;;
    --iters)
      ITERS="$2"
      shift 2
      ;;
    --ws)
      WS="$2"
      shift 2
      ;;
    --variants)
      VARIANTS="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-bench)
      SKIP_BENCH_BUILD=1
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

mkdir -p "$OUTDIR"

args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --ws "$WS"
  --outdir "${OUTDIR}/trim_ab"
  --no-stats
  --no-diagnostics
  --rows fixed
  --variants "$VARIANTS"
)
if [[ "$SKIP_BENCH_BUILD" -eq 1 ]]; then
  args+=(--skip-bench)
fi

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "variants=${VARIANTS}"
  echo "stats=0"
  echo "rows=fixed"
} > "${OUTDIR}/README.log"

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_midpage_payload_trim_ab.sh" \
  "${args[@]}"

{
  echo "# HZ6 Fixed Quiescent RSS Matrix"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "## Purpose"
  echo
  echo '```text'
  echo "FixedQuiescentRssMatrix-L1 checks whether quiescent release /"
  echo "malloc_trim changes current RSS after fixed-size rows."
  echo "Peak RSS remains the high-water column; current RSS is the"
  echo "post-benchmark resident set after the optional trim hook."
  echo '```'
  echo
  echo "## Inputs"
  echo
  echo '```text'
  echo "rows=fixed"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "variants=${VARIANTS}"
  echo '```'
  echo
  echo "## Results"
  echo
  cat "${OUTDIR}/trim_ab/summary.md"
  echo
  echo "## Read Rules"
  echo
  echo '```text'
  echo "peak MiB:"
  echo "  high-water RSS; malloc_trim after the workload usually cannot lower it"
  echo
  echo "current MiB:"
  echo "  post-trim resident set; use this to judge quiescent/RSS-return behavior"
  echo
  echo "selected defaulting:"
  echo "  do not default a trim behavior from current RSS alone"
  echo "  require no speed regression and a workload that observes current RSS"
  echo '```'
} > "${OUTDIR}/summary.md"

cat "${OUTDIR}/summary.md"
