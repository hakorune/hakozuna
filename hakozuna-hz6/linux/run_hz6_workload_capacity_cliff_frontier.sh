#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-30000}"
ROWS_CSV="${ROWS:-cliff16384}"
ALLOCATORS="${ALLOCATORS:-hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target,hz6-workload-capacity-lite-target,hz6-workload-capacity-mid-target,hz6-workload-capacity-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_cliff_frontier_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_aliases.sh"
source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_workload_profile_ladder_common.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_cliff_frontier.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 3)
  --iters N          iterations per run/thread (default: 30000)
  --rows CSV         gap-diagnostic rows, default cliff16384
  --allocators CSV   allocator/profile aliases to compare
  --outdir DIR       output directory
  --skip-builds      reuse existing allocator/profile/bench builds
  --help             show this message

This runner measures the existing workload-capacity profile frontier on the
WS16384 cliff rows. It is production-speed/RSS evidence only; use
run_hz6_workload_capacity_cliff_diag.sh for counter attribution.
EOF
}

allocator_list_contains() {
  local needle="$1"
  [[ ",${ALLOCATORS}," == *",${needle},"* ]]
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
      ROWS_CSV="$2"
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

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  if allocator_list_contains hz6; then
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  fi
  hz6_preload_build_requested_aliases "$ALLOCATORS" "$ROOT_DIR"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }

rows=()
hz6_workload_append_gap_diag_rows rows "$ITERS" "$ROWS_CSV"

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "rows=${ROWS_CSV}"
  echo "allocators=${ALLOCATORS}"
  echo "bench=${BENCH_BIN}"
  echo "note=production profile frontier for WS16384 cliff rows; stats-free"
  for row_spec in "${rows[@]}"; do
    echo "row=${row_spec}"
  done
} > "${OUTDIR}/README.log"

for row_spec in "${rows[@]}"; do
  read -r row threads iters ws min_size max_size <<< "$row_spec"
  row_out="${OUTDIR}/${row}"
  mkdir -p "$row_out"
  "${ROOT_DIR}/bench/run_compare.sh" \
    --allocators "$ALLOCATORS" \
    --bench-bin "$BENCH_BIN" \
    --bench-args "${threads} ${iters} ${ws} ${min_size} ${max_size}" \
    --runs "$RUNS" \
    --outdir "$row_out"
done

python3 - <<'PY' "$OUTDIR" "${rows[@]}" | tee "${OUTDIR}/summary.md"
import pathlib
import re
import statistics
import sys

root = pathlib.Path(sys.argv[1])
row_specs = sys.argv[2:]
rows = [spec.split()[0] for spec in row_specs]
alloc_re = re.compile(r"^\d+_(.+)\.log$")
ops_re = re.compile(r"ops/s=([0-9.]+)")
peak_re = re.compile(r"peak_kb=([0-9]+)")

print("# HZ6 Workload Capacity Cliff Frontier\n")
print(f"root: `{root}`\n")
print("Rows are WS16384 workload proxies over `bench_mixed_ws_crt`; this is production speed/RSS evidence.\n")
print("| row | allocator | median ops/s | median peak MiB | ops/s per MiB |")
print("| --- | --- | ---: | ---: | ---: |")
for row in rows:
    by_alloc = {}
    for log in sorted((root / row).glob("*.log")):
        match = alloc_re.match(log.name)
        if not match:
            continue
        alloc = match.group(1)
        text = log.read_text(errors="replace")
        ops = ops_re.search(text)
        peak = peak_re.search(text)
        if not ops or not peak:
            continue
        by_alloc.setdefault(alloc, {"ops": [], "peak": []})
        by_alloc[alloc]["ops"].append(float(ops.group(1)))
        by_alloc[alloc]["peak"].append(float(peak.group(1)) / 1024.0)
    for alloc in sorted(by_alloc):
        ops = statistics.median(by_alloc[alloc]["ops"])
        peak = statistics.median(by_alloc[alloc]["peak"])
        efficiency = ops / peak if peak else 0.0
        print(f"| `{row}` | `{alloc}` | {ops:.3f} | {peak:.2f} | {efficiency:.3f} |")

print("\n## Row Winners\n")
print("| row | speed winner | RSS winner | efficiency winner |")
print("| --- | --- | --- | --- |")
for row in rows:
    row_dir = root / row
    data = {}
    for log in sorted(row_dir.glob("*.log")):
        match = alloc_re.match(log.name)
        if not match:
            continue
        alloc = match.group(1)
        text = log.read_text(errors="replace")
        ops = ops_re.search(text)
        peak = peak_re.search(text)
        if not ops or not peak:
            continue
        data.setdefault(alloc, {"ops": [], "peak": []})
        data[alloc]["ops"].append(float(ops.group(1)))
        data[alloc]["peak"].append(float(peak.group(1)) / 1024.0)
    med = {}
    for alloc, values in data.items():
        ops = statistics.median(values["ops"])
        peak = statistics.median(values["peak"])
        med[alloc] = {"ops": ops, "peak": peak, "eff": ops / peak if peak else 0.0}
    if not med:
        continue
    speed = max(med, key=lambda alloc: med[alloc]["ops"])
    rss = min(med, key=lambda alloc: med[alloc]["peak"])
    eff = max(med, key=lambda alloc: med[alloc]["eff"])
    print(f"| `{row}` | `{speed}` | `{rss}` | `{eff}` |")
PY
