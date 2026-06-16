#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_aliases.sh"

ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-5}"
ITERS="${ITERS:-100000}"
THREADS="${THREADS:-4}"
WS_LIST="${WS_LIST:-2000,4096,8192,16384}"
BANDS="${BANDS:-small,mixed,midpage}"
ALLOCATORS="${ALLOCATORS:-hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_capacity_shape_sweep_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_capacity_shape_sweep.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 5)
  --iters N          iterations per run/thread (default: 100000)
  --threads N        benchmark threads (default: 4)
  --ws-list CSV      working-set slot counts (default: 2000,4096,8192,16384)
  --bands CSV        size bands: small,object,mixed,midpage,all
                     (default: small,mixed,midpage)
  --allocators CSV   allocator/profile aliases to compare
  --outdir DIR       output directory
  --skip-builds      reuse existing allocator/profile/bench builds
  --skip-prepare     reuse existing external allocator environment
  --help             show this message

This shape sweep is a diagnostic surface for workload-capacity profiles. It
varies live working-set size and allocation size band around the current proxy
rows. Use it to find row-shape boundaries; do not promote selected/default from
this proxy evidence alone.
EOF
}

allocator_list_contains() {
  local needle="$1"
  [[ ",${ALLOCATORS}," == *",${needle},"* ]]
}

append_band_rows() {
  local -n rows_out="$1"
  local band="$2"
  local ws="$3"
  case "$band" in
    small)
      rows_out+=("small_ws${ws} ${THREADS} ${ITERS} ${ws} 16 256")
      ;;
    object)
      rows_out+=("object_ws${ws} ${THREADS} ${ITERS} ${ws} 16 1024")
      ;;
    mixed)
      rows_out+=("mixed_ws${ws} ${THREADS} ${ITERS} ${ws} 64 8192")
      ;;
    midpage)
      rows_out+=("midpage_ws${ws} ${THREADS} ${ITERS} ${ws} 4096 32768")
      ;;
    *)
      echo "unknown band: ${band}" >&2
      return 2
      ;;
  esac
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
    --threads)
      THREADS="$2"
      shift 2
      ;;
    --ws-list)
      WS_LIST="$2"
      shift 2
      ;;
    --bands)
      BANDS="$2"
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

rows=()
IFS=',' read -r -a ws_values <<< "$WS_LIST"
IFS=',' read -r -a band_values <<< "$BANDS"
for ws in "${ws_values[@]}"; do
  for band in "${band_values[@]}"; do
    if [[ "$band" == "all" ]]; then
      append_band_rows rows small "$ws"
      append_band_rows rows object "$ws"
      append_band_rows rows mixed "$ws"
      append_band_rows rows midpage "$ws"
    elif [[ -n "$band" ]]; then
      append_band_rows rows "$band" "$ws"
    fi
  done
done

mkdir -p "$OUTDIR"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  if allocator_list_contains hz3 || allocator_list_contains hz4; then
    "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
  fi
  if allocator_list_contains hz5; then
    "${ROOT_DIR}/linux/build_linux_hz5_preload_full.sh" --arch "$ARCH"
  fi
  if allocator_list_contains hz6; then
    "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  fi
  hz6_preload_build_requested_aliases "$ALLOCATORS" "$ROOT_DIR"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

if [[ "$SKIP_PREPARE" -ne 1 ]]; then
  if allocator_list_contains mimalloc || allocator_list_contains tcmalloc; then
    env_file="${OUTDIR}/allocator_env.sh"
    "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" \
      > "$env_file"
    # shellcheck disable=SC1090
    source "$env_file"
  else
    SKIP_PREPARE=1
  fi
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "threads=${THREADS}"
  echo "ws_list=${WS_LIST}"
  echo "bands=${BANDS}"
  echo "allocators=${ALLOCATORS}"
  echo "bench=${BENCH_BIN}"
  echo "note=shape sweep over bench_mixed_ws_crt; proxy evidence only"
  for row_spec in "${rows[@]}"; do
    echo "row=${row_spec}"
  done
} > "${OUTDIR}/README.log"

run_row() {
  local row="$1"
  local threads="$2"
  local iters="$3"
  local ws="$4"
  local min_size="$5"
  local max_size="$6"
  local row_out="${OUTDIR}/${row}"
  mkdir -p "$row_out"
  "${ROOT_DIR}/bench/run_compare.sh" \
    --allocators "$ALLOCATORS" \
    --bench-bin "$BENCH_BIN" \
    --bench-args "${threads} ${iters} ${ws} ${min_size} ${max_size}" \
    --runs "$RUNS" \
    --outdir "$row_out"
}

for row_spec in "${rows[@]}"; do
  read -r row threads iters ws min_size max_size <<< "$row_spec"
  run_row "$row" "$threads" "$iters" "$ws" "$min_size" "$max_size"
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
fail_re = re.compile(r"\balloc_fail=([0-9]+)")

print("# HZ6 Workload Capacity Shape Sweep\n")
print(f"root: `{root}`\n")
print("Rows sweep working-set and size-band shapes over `bench_mixed_ws_crt`; this is proxy evidence only.\n")
print("| row | allocator | median ops/s | median peak MiB | ops/s per MiB | median alloc_fail |")
print("| --- | --- | ---: | ---: | ---: | ---: |")
row_data = {}
for row in rows:
    row_dir = root / row
    by_alloc = {}
    for log in sorted(row_dir.glob("*.log")):
        match = alloc_re.match(log.name)
        if not match:
            continue
        alloc = match.group(1)
        text = log.read_text(errors="replace")
        ops_match = ops_re.search(text)
        peak_match = peak_re.search(text)
        if not ops_match or not peak_match:
            continue
        fail_match = fail_re.search(text)
        by_alloc.setdefault(alloc, {"ops": [], "peak": [], "fail": []})
        by_alloc[alloc]["ops"].append(float(ops_match.group(1)))
        by_alloc[alloc]["peak"].append(float(peak_match.group(1)) / 1024.0)
        by_alloc[alloc]["fail"].append(float(fail_match.group(1)) if fail_match else 0.0)
    row_data[row] = {}
    for alloc in sorted(by_alloc):
        ops = statistics.median(by_alloc[alloc]["ops"])
        peak = statistics.median(by_alloc[alloc]["peak"])
        fail = statistics.median(by_alloc[alloc]["fail"])
        efficiency = ops / peak if peak else 0.0
        row_data[row][alloc] = {
            "ops": ops,
            "peak": peak,
            "efficiency": efficiency,
            "fail": fail,
        }
        print(f"| `{row}` | `{alloc}` | {ops:.3f} | {peak:.2f} | {efficiency:.3f} | {fail:.0f} |")

print("\n## Row Winners\n")
print("| row | speed winner | RSS winner | efficiency winner |")
print("| --- | --- | --- | --- |")
for row in rows:
    data = row_data.get(row, {})
    if not data:
        continue
    speed = max(data, key=lambda alloc: data[alloc]["ops"])
    rss = min(data, key=lambda alloc: data[alloc]["peak"])
    efficiency = max(data, key=lambda alloc: data[alloc]["efficiency"])
    print(f"| `{row}` | `{speed}` | `{rss}` | `{efficiency}` |")

narrow_name = "hz6-workload-capacity-narrow-target"
hybrid_name = "hz6-workload-capacity-hybrid-target"
pair_rows = [
    row for row in rows
    if narrow_name in row_data.get(row, {}) and hybrid_name in row_data.get(row, {})
]
if pair_rows:
    print("\n## Capacity Pair Delta\n")
    print("Positive speed/efficiency delta favors capacity-hybrid; negative peak delta means capacity-hybrid uses less RSS.\n")
    print("| row | hybrid speed delta | hybrid peak delta MiB | hybrid efficiency delta | read |")
    print("| --- | ---: | ---: | ---: | --- |")
    for row in pair_rows:
        narrow = row_data[row][narrow_name]
        hybrid = row_data[row][hybrid_name]
        speed_delta = ((hybrid["ops"] / narrow["ops"]) - 1.0) * 100.0 if narrow["ops"] else 0.0
        peak_delta = hybrid["peak"] - narrow["peak"]
        efficiency_delta = (
            ((hybrid["efficiency"] / narrow["efficiency"]) - 1.0) * 100.0
            if narrow["efficiency"] else 0.0
        )
        if abs(speed_delta) < 1.0 and abs(peak_delta) < 0.5:
            read = "tie"
        elif speed_delta >= 1.0 and peak_delta <= 0.5:
            read = "hybrid"
        elif speed_delta <= -1.0 and peak_delta >= -0.5:
            read = "narrow"
        elif efficiency_delta >= 1.0:
            read = "hybrid-eff"
        elif efficiency_delta <= -1.0:
            read = "narrow-eff"
        else:
            read = "mixed"
        print(
            f"| `{row}` | {speed_delta:+.2f}% | {peak_delta:+.2f} | "
            f"{efficiency_delta:+.2f}% | `{read}` |"
        )
PY
