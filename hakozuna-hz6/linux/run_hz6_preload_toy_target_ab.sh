#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-7}"
ITERS="${ITERS:-300000}"
WS="${WS:-4096}"
ROWS_CSV="${ROWS:-focused,fixed}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_toy_target_preload_ab_$(date +%Y%m%d_%H%M%S)}"

BENCH="${BENCH:-${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt}"
SELECTED_SO="${SELECTED_SO:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so}"
TOY_TARGET_SO="${TOY_TARGET_SO:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload_toy_target/libhakozuna_hz6_preload.so}"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_toy_target_ab.sh [options]

Options:
  --runs N       runs per row/lane (default: 7)
  --iters N      iterations per run (default: 300000)
  --ws N         working set (default: 4096)
  --rows CSV     row groups: focused,fixed (default: focused,fixed)
  --outdir DIR   output directory
  --skip-build   reuse existing preload DSOs and benchmark binary
  --help         show this message
EOF
}

SKIP_BUILD=0
require_value() {
  local option="$1"
  if [[ $# -lt 2 || -z "${2:-}" ]]; then
    echo "missing value for ${option}" >&2
    usage >&2
    exit 1
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs)
      require_value "$1" "${2:-}"
      RUNS="$2"
      shift 2
      ;;
    --iters)
      require_value "$1" "${2:-}"
      ITERS="$2"
      shift 2
      ;;
    --ws)
      require_value "$1" "${2:-}"
      WS="$2"
      shift 2
      ;;
    --rows)
      require_value "$1" "${2:-}"
      ROWS_CSV="$2"
      shift 2
      ;;
    --outdir)
      require_value "$1" "${2:-}"
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
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload_toy_target.sh"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

[[ -x "$BENCH" ]] || { echo "missing benchmark: $BENCH" >&2; exit 2; }
[[ -f "$SELECTED_SO" ]] || { echo "missing selected DSO: $SELECTED_SO" >&2; exit 2; }
[[ -f "$TOY_TARGET_SO" ]] || { echo "missing toy target DSO: $TOY_TARGET_SO" >&2; exit 2; }

rows=()
IFS=',' read -r -a row_groups <<< "$ROWS_CSV"
for row_group in "${row_groups[@]}"; do
  case "$row_group" in
    focused)
      rows+=(
        "16_256 16 256"
        "16_4096 16 4096"
        "1024_4096 1024 4096"
        "4096_16384 4096 16384"
      )
      ;;
    fixed)
      rows+=(
        "fixed_4k 4096 4096"
        "fixed_8k 8192 8192"
        "fixed_16k 16384 16384"
      )
      ;;
    *)
      echo "unknown row group: ${row_group}" >&2
      exit 2
      ;;
  esac
done

mkdir -p "$OUTDIR"
git_sha="$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || true)"
selected_sha256="$(sha256sum "$SELECTED_SO" 2>/dev/null | awk '{print $1}' || true)"
toy_target_sha256="$(sha256sum "$TOY_TARGET_SO" 2>/dev/null | awk '{print $1}' || true)"
{
  echo "arch=${ARCH}"
  echo "git=${git_sha:-unknown}"
  echo "bench=${BENCH}"
  echo "selected=${SELECTED_SO}"
  echo "selected_sha256=${selected_sha256:-unknown}"
  echo "toy_target=${TOY_TARGET_SO}"
  echo "toy_target_sha256=${toy_target_sha256:-unknown}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "ws=${WS}"
  echo "rows=${ROWS_CSV}"
  echo "selected_builder=hakozuna-hz6/linux/build_hz6_preload.sh"
  echo "toy_target_builder=hakozuna-hz6/linux/build_hz6_preload_toy_target.sh"
} > "${OUTDIR}/README.log"

run_row() {
  local row="$1"
  local min_size="$2"
  local max_size="$3"

  for lane in selected toy_target; do
    local so="$SELECTED_SO"
    [[ "$lane" == toy_target ]] && so="$TOY_TARGET_SO"
    mkdir -p "${OUTDIR}/${row}/${lane}"
    for run in $(seq 1 "$RUNS"); do
      env LD_PRELOAD="$so" "$BENCH" 4 "$ITERS" "$WS" "$min_size" "$max_size" \
        > "${OUTDIR}/${row}/${lane}/${run}.log" 2>&1
    done
  done
}

for row_spec in "${rows[@]}"; do
  read -r row min_size max_size <<< "$row_spec"
  run_row "$row" "$min_size" "$max_size"
done

python3 - <<'PY' "$OUTDIR" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
row_specs = sys.argv[2:]
rows = [spec.split()[0] for spec in row_specs]
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")

print("# HZ6 Toy Target Preload A/B\n")
print(f"root: `{root}`\n")
print("| row | selected ops/s | toy_target ops/s | toy_target delta | selected peak MiB | toy_target peak MiB |")
print("| --- | ---: | ---: | ---: | ---: | ---: |")
for row in rows:
    values = {}
    for lane in ["selected", "toy_target"]:
        ops = []
        peak = []
        for path in sorted((root / row / lane).glob("*.log")):
            text = path.read_text(errors="replace")
            m_ops = ops_re.search(text)
            m_peak = peak_re.search(text)
            if m_ops:
                ops.append(float(m_ops.group(1)))
            if m_peak:
                peak.append(int(m_peak.group(1)) / 1024.0)
        values[lane] = {
            "ops": statistics.median(ops) if ops else 0.0,
            "peak": statistics.median(peak) if peak else 0.0,
        }
    selected_ops = values["selected"]["ops"]
    toy_ops = values["toy_target"]["ops"]
    delta = ((toy_ops / selected_ops) - 1.0) * 100.0 if selected_ops else 0.0
    print(
        f"| `{row}` | {selected_ops:.3f} | {toy_ops:.3f} | "
        f"{delta:+.2f}% | {values['selected']['peak']:.2f} | "
        f"{values['toy_target']['peak']:.2f} |"
    )
PY
