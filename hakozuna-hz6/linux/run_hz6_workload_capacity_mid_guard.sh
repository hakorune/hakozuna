#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-5}"
ITERS="${ITERS:-200000}"
ROWS="${ROWS:-small_proxy,cache_proxy}"
ALLOCATORS="${ALLOCATORS:-hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target,hz6-workload-capacity-mid-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_mid_guard_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_mid_guard.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 5)
  --iters N          iterations per run (default: 200000)
  --rows CSV         row groups for workload proxy matrix
                     (default: small_proxy,cache_proxy)
  --allocators CSV   allocator/profile aliases to compare
  --outdir DIR       output directory
  --skip-builds      reuse existing allocator/profile/bench builds
  --skip-prepare     reuse existing external allocator environment
  --help             show this message

This guard compares the normal workload-capacity recommendation pair against
capacity-mid:
  - hz6-workload-capacity-narrow-target
  - hz6-workload-capacity-hybrid-target
  - hz6-workload-capacity-mid-target

Use it after WS16384 cliff-frontier runs to decide whether capacity-mid should
remain an explicit high-live-set profile or deserves broader workload evidence.
It is proxy evidence only; it does not promote selected/default by itself.
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
    --rows)
      ROWS="$2"
      shift 2
      ;;
    --allocators)
      ALLOCATORS="$2"
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
    --skip-prepare)
      SKIP_PREPARE=1
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

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "rows=${ROWS}"
  echo "allocators=${ALLOCATORS}"
  echo "note=capacity-narrow/capacity-hybrid/capacity-mid workload proxy guard"
  echo "decision=profile recommendation stability only; no selected/default promotion"
} > "${OUTDIR}/README.log"

args=(
  --arch "$ARCH"
  --runs "$RUNS"
  --iters "$ITERS"
  --rows "$ROWS"
  --allocators "$ALLOCATORS"
  --outdir "${OUTDIR}/workload_proxy"
)

if [[ "$SKIP_BUILDS" -eq 1 ]]; then
  args+=(--skip-builds)
fi
if [[ "$SKIP_PREPARE" -eq 1 ]]; then
  args+=(--skip-prepare)
fi

"${ROOT_DIR}/hakozuna-hz6/linux/run_hz6_workload_proxy_matrix.sh" \
  "${args[@]}"

{
  echo "# HZ6 Workload Capacity Mid Guard"
  echo
  echo "root: \`${OUTDIR}\`"
  echo
  echo "This guard compares capacity-narrow, capacity-hybrid, and capacity-mid."
  echo "Use it to check whether the WS16384 capacity-mid profile pays too much"
  echo "RSS or speed cost on normal workload proxy rows."
  echo
  if [[ -f "${OUTDIR}/workload_proxy/summary.md" ]]; then
    cat "${OUTDIR}/workload_proxy/summary.md"
  else
    echo "missing workload proxy summary: ${OUTDIR}/workload_proxy/summary.md"
  fi
  python3 - <<'PY' "${OUTDIR}/workload_proxy"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
alloc_re = re.compile(r"^\d+_(.+)\.log$")
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")
pair_allocs = {
    "hz6-workload-capacity-narrow-target",
    "hz6-workload-capacity-hybrid-target",
}
mid_alloc = "hz6-workload-capacity-mid-target"

rows = []
for row_dir in sorted(p for p in root.iterdir() if p.is_dir()):
    by_alloc = {}
    for log in sorted(row_dir.glob("*.log")):
        match = alloc_re.match(log.name)
        if not match:
            continue
        text = log.read_text(errors="replace")
        ops_match = ops_re.search(text)
        peak_match = peak_re.search(text)
        if not ops_match or not peak_match:
            continue
        alloc = match.group(1)
        by_alloc.setdefault(alloc, {"ops": [], "peak": []})
        by_alloc[alloc]["ops"].append(float(ops_match.group(1)))
        by_alloc[alloc]["peak"].append(float(peak_match.group(1)) / 1024.0)
    if mid_alloc not in by_alloc:
        continue
    pair_stats = []
    for alloc in sorted(pair_allocs & by_alloc.keys()):
        ops = statistics.median(by_alloc[alloc]["ops"])
        peak = statistics.median(by_alloc[alloc]["peak"])
        eff = ops / peak if peak else 0.0
        pair_stats.append((eff, ops, peak, alloc))
    if not pair_stats:
        continue
    _, pair_ops, pair_peak, pair_alloc = max(pair_stats)
    mid_ops = statistics.median(by_alloc[mid_alloc]["ops"])
    mid_peak = statistics.median(by_alloc[mid_alloc]["peak"])
    mid_eff = mid_ops / mid_peak if mid_peak else 0.0
    pair_eff = pair_ops / pair_peak if pair_peak else 0.0
    speed_delta = ((mid_ops / pair_ops) - 1.0) * 100.0 if pair_ops else 0.0
    peak_delta = mid_peak - pair_peak
    eff_delta = ((mid_eff / pair_eff) - 1.0) * 100.0 if pair_eff else 0.0
    rows.append((row_dir.name, pair_alloc, speed_delta, peak_delta, eff_delta))

if rows:
    print()
    print("## Capacity Mid Delta")
    print()
    print("Positive speed/efficiency means capacity-mid beats the best normal pair row.")
    print()
    print("| row | best normal pair | mid speed delta | mid peak MiB delta | mid efficiency delta | read |")
    print("| --- | --- | ---: | ---: | ---: | --- |")
    for row, pair_alloc, speed_delta, peak_delta, eff_delta in rows:
        read = "mid-candidate" if speed_delta >= 0.0 and eff_delta >= 0.0 else "keep-explicit-high-live-set"
        print(
            f"| `{row}` | `{pair_alloc}` | {speed_delta:+.2f}% | "
            f"{peak_delta:+.2f} | {eff_delta:+.2f}% | `{read}` |"
        )
PY
} | tee "${OUTDIR}/summary.md"
