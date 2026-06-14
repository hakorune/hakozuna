#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-7}"
ITERS="${ITERS:-500000}"
WS="${WS:-4096}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_midpage_preload_boundary_$(date +%Y%m%d_%H%M%S)}"

BENCH="${BENCH:-${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt}"
SELECTED_SO="${SELECTED_SO:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so}"
CANDIDATE_SO="${CANDIDATE_SO:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_midpage_target/libhakozuna_hz6_preload.so}"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_midpage_boundary_ab.sh [options]

Options:
  --runs N       runs per row/lane (default: 7)
  --iters N      iterations per run (default: 500000)
  --ws N         working set (default: 4096)
  --outdir DIR   output directory
  --skip-build   reuse existing preload DSOs and benchmark binary
  --help         show this message
EOF
}

SKIP_BUILD=0
while [[ $# -gt 0 ]]; do
  case "$1" in
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
    --outdir)
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
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_midpage_target.sh"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$CANDIDATE_SO" ]] || { echo "missing candidate DSO: $CANDIDATE_SO" >&2; exit 2; }

mkdir -p "$OUTDIR"
{
  echo "bench=${BENCH}"
  echo "selected=${SELECTED_SO}"
  echo "candidate=${CANDIDATE_SO}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
} > "${OUTDIR}/README.log"

run_row() {
  local row="$1"
  local min_size="$2"
  local max_size="$3"

  for lane in selected candidate; do
    local so="$SELECTED_SO"
    [[ "$lane" == candidate ]] && so="$CANDIDATE_SO"
    mkdir -p "${OUTDIR}/${row}/${lane}"
    for run in $(seq 1 "$RUNS"); do
      env LD_PRELOAD="$so" "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
        > "${OUTDIR}/${row}/${lane}/${run}.log" 2>&1
    done
  done
}

run_row 4096_16384 4096 16384
run_row 16_256 16 256
run_row 16_4096 16 4096
run_row 1024_4096 1024 4096

python3 - <<'PY' "$OUTDIR" | tee "${OUTDIR}/summary.txt"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
rows = ["4096_16384", "16_256", "16_4096", "1024_4096"]
print(root)
for row in rows:
    medians = {}
    peaks = {}
    for lane in ["selected", "candidate"]:
        ops = []
        peak = []
        for path in sorted((root / row / lane).glob("*.log")):
            text = path.read_text()
            m_ops = re.search(r"ops/s=([0-9.]+)", text)
            m_peak = re.search(r"peak_kb=([0-9]+)", text)
            if m_ops:
                ops.append(float(m_ops.group(1)))
            if m_peak:
                peak.append(int(m_peak.group(1)))
        medians[lane] = statistics.median(ops)
        peaks[lane] = statistics.median(peak)
    delta = (medians["candidate"] / medians["selected"] - 1.0) * 100.0
    print(
        f"{row}: selected={medians['selected']:.3f} "
        f"candidate={medians['candidate']:.3f} delta={delta:+.2f}% "
        f"peak_kb={peaks['selected']:.0f}->{peaks['candidate']:.0f}"
    )
PY
