#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_preload_aliases.sh"
source "${ROOT_DIR}/hakozuna-hz6/linux/hz6_workload_profile_ladder_common.sh"
ARCH="${ARCH:-x86_64}"
RUNS="${RUNS:-3}"
ITERS="${ITERS:-200000}"
ALLOCATORS="${ALLOCATORS:-system,hz3,hz4,hz6,hz6-workload-capacity-narrow-target,hz6-workload-capacity-hybrid-target,hz6-small-boundary-trusted-toy-map8192-target,hz6-small-boundary-trusted-toy-map8192-external-target,mimalloc,tcmalloc}"
ROWS_CSV="${ROWS:-small_proxy,cache_proxy}"
OUTDIR="${OUTDIR:-${ROOT_DIR}/hakozuna-hz6/private/raw-results/linux/hz6_workload_proxy_matrix_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0

allocator_list_contains() {
  local needle="$1"
  [[ ",${ALLOCATORS}," == *",${needle},"* ]]
}

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_workload_proxy_matrix.sh [options]

Options:
  --arch ARCH        target arch (default: x86_64)
  --runs N           runs per allocator/row (default: 3)
  --iters N          iterations per run (default: 200000)
  --allocators CSV   allocator/profile aliases to compare
  --rows CSV         row groups: small_proxy,cache_proxy,all
                     (default: small_proxy,cache_proxy)
  --outdir DIR       output directory
  --skip-builds      reuse existing HZ3/HZ4/HZ5/HZ6/profile/bench builds
  --skip-prepare     reuse existing mimalloc/tcmalloc environment variables
  --help             show this message

Notes:
  These rows are workload proxies over bench_mixed_ws_crt, not app benchmarks.
  They are intended as broad guard evidence before promoting HZ6 preload
  profile/default changes.
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
    --allocators)
      ALLOCATORS="$2"
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
    --skip-prepare)
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

mkdir -p "$OUTDIR"

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
  "${ROOT_DIR}/linux/build_linux_hz5_preload_full.sh" --arch "$ARCH"
  "${ROOT_DIR}/hakozuna-hz6/linux/build_hz6_preload.sh"
  hz6_preload_build_requested_aliases "$ALLOCATORS" "$ROOT_DIR"
  "${ROOT_DIR}/linux/build_linux_bench_compare.sh" --arch "$ARCH" \
    --out-dir "${ROOT_DIR}/bench/out/linux/${ARCH}"
fi

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  if allocator_list_contains mimalloc || allocator_list_contains tcmalloc; then
    env_file="${OUTDIR}/allocator_env.sh"
    "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" \
      > "$env_file"
    # shellcheck disable=SC1090
    source "$env_file"
  else
    SKIP_PREPARE_ALLOCATORS=1
  fi
fi

BENCH_BIN="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_ws_crt"
[[ -x "$BENCH_BIN" ]] || { echo "missing benchmark: $BENCH_BIN" >&2; exit 2; }

rows=()
hz6_workload_append_proxy_rows rows "$ITERS" "$ROWS_CSV"

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "iters=${ITERS}"
  echo "allocators=${ALLOCATORS}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH_BIN}"
  echo "note=workload proxies over bench_mixed_ws_crt, not app benchmarks"
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

print("# HZ6 Workload Proxy Matrix\n")
print(f"root: `{root}`\n")
print("Rows are workload proxies over `bench_mixed_ws_crt`, not app benchmarks.\n")
print("| row | allocator | median ops/s | median peak MiB | ops/s per MiB | median alloc_fail |")
print("| --- | --- | ---: | ---: | ---: | ---: |")
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
    for alloc in sorted(by_alloc):
        ops = statistics.median(by_alloc[alloc]["ops"])
        peak = statistics.median(by_alloc[alloc]["peak"])
        fail = statistics.median(by_alloc[alloc]["fail"])
        efficiency = ops / peak if peak else 0.0
        print(f"| `{row}` | `{alloc}` | {ops:.3f} | {peak:.2f} | {efficiency:.3f} | {fail:.0f} |")
PY
