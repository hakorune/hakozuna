#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-80000}"
PROFILE="${PROFILE:-desc10k_source1280_route40k}"
DEPOTS_CSV="${DEPOTS:-1024,2048,4096,8192}"
ROWS_CSV="${ROWS:-small_proxy,cache_proxy}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_descriptor_hybrid_depot_ladder_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_descriptor_hybrid_depot_ladder.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per row/variant (default: 3)
  --iters N          iterations per run (default: 80000)
  --profile NAME     hybrid static-capacity profile (default: desc10k_source1280_route40k)
  --depots CSV       elastic descriptor depot capacities (default: 1024,2048,4096,8192)
  --rows CSV         row groups: small_proxy,cache_proxy,all
  --outdir DIR       output directory
  --skip-builds      pass through to the child hybrid ladder
  --help             show this message
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
      DEPOTS_CSV="$2"
      shift 2
      ;;
    --rows)
      ROWS_CSV="$2"
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

mkdir -p "$OUTDIR"
IFS=',' read -r -a depots <<< "$DEPOTS_CSV"

for depot in "${depots[@]}"; do
  child_args=(
    --arch "$ARCH"
    --runs "$RUNS"
    --iters "$ITERS"
    --profiles "$PROFILE"
    --depot "$depot"
    --rows "$ROWS_CSV"
    --outdir "${OUTDIR}/depot_${depot}"
  )
  if [[ "$SKIP_BUILDS" -eq 1 ]]; then
    child_args+=(--skip-builds)
  fi
  "${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_descriptor_hybrid_ladder.sh" \
    "${child_args[@]}"
done

python3 - <<'PY' "$OUTDIR" "$DEPOTS_CSV" "$PROFILE" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
depots = [item for item in sys.argv[2].split(",") if item]
profile = sys.argv[3]
hybrid_name = f"hybrid_{profile}"
log_re = re.compile(r"^(.+)_(selected|descriptor_overflow|capacity_lite|hybrid_.+)_([0-9]+)\.log$")
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
fail_re = re.compile(r"\balloc_fail=([0-9]+)")

rows = {}
for depot in depots:
    logs_dir = root / f"depot_{depot}" / "logs"
    for log in sorted(logs_dir.glob("*.log")):
        match = log_re.match(log.name)
        if not match:
            continue
        row, variant, _run = match.groups()
        if variant != hybrid_name:
            continue
        text = log.read_text(errors="replace")
        ops = ops_re.search(text)
        peak = peak_re.search(text)
        if not ops or not peak:
            continue
        fail = fail_re.search(text)
        bucket = rows.setdefault(row, {}).setdefault(
            depot, {"ops": [], "peak": [], "fail": []}
        )
        bucket["ops"].append(float(ops.group(1)))
        bucket["peak"].append(float(peak.group(1)) / 1024.0)
        bucket["fail"].append(float(fail.group(1)) if fail else 0.0)

print("# HZ6 Workload Descriptor Hybrid Depot Ladder\n")
print(f"root: `{root}`\n")
print(f"profile: `{profile}`\n")
print("| row | depot | median ops/s | median peak MiB | ops/s per MiB | median alloc_fail |")
print("| --- | ---: | ---: | ---: | ---: | ---: |")
for row in sorted(rows):
    for depot in depots:
        data = rows[row].get(depot)
        if not data:
            continue
        ops = statistics.median(data["ops"])
        peak = statistics.median(data["peak"])
        fail = statistics.median(data["fail"])
        eff = ops / peak if peak else 0.0
        print(f"| `{row}` | {depot} | {ops:.3f} | {peak:.2f} | {eff:.3f} | {fail:.0f} |")
PY
