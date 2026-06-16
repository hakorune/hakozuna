#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-80000}"
PROFILE="${PROFILE:-desc10k_source1280_route40k}"
DEPOTS="${DEPOTS:-256,512,1024,1536}"
ROWS="${ROWS:-small_proxy,cache_proxy}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_hybrid_depot_ladder_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_hybrid_depot_ladder.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per row/variant (default: 3)
  --iters N          iterations per run (default: 80000)
  --profile NAME     hybrid static-capacity profile
                     (default: desc10k_source1280_route40k)
  --depots CSV       elastic descriptor depot capacities
                     (default: 256,512,1024,1536)
  --rows CSV         row groups: small_proxy,cache_proxy,all
  --outdir DIR       output directory
  --skip-builds      pass through to the child ladder
  --help             show this message

This is the capacity-hybrid naming wrapper for the historical
descriptor-hybrid depot ladder. It keeps the benchmark implementation shared
while making current workload-profile reads use the capacity-hybrid name.
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
    --profile)
      PROFILE="$2"
      shift 2
      ;;
    --depots)
      DEPOTS="$2"
      shift 2
      ;;
    --rows)
      ROWS="$2"
      shift 2
      ;;
    --outdir)
      OUTDIR="$2"
      shift 2
      ;;
    --skip-builds)
      SKIP_BUILDS=1
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

args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --profile "$PROFILE"
  --depots "$DEPOTS"
  --rows "$ROWS"
  --outdir "$OUTDIR"
)

if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  args+=(--skip-builds)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_descriptor_hybrid_depot_ladder.sh" \
  "${args[@]}"

if [[ -f "${OUTDIR}/summary.md" ]]; then
  python3 - <<'PY' "${OUTDIR}/summary.md"
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
text = path.read_text()
text = text.replace(
    "# HZ6 Workload Descriptor Hybrid Depot Ladder",
    "# HZ6 Workload Capacity Hybrid Depot Ladder",
)
path.write_text(text)
PY
fi
