#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
source "${ROOT_DIR}/bench/lib/bench_common.sh"
source "${HZ6_DIR}/linux/hz6_preload_aliases.sh"

ARCH="${ARCH:-x86_64}"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_REMOTE_ALLOCATOR_COMPARE_TIMEOUT:-90}"
ALLOCATORS_CSV="${ALLOCATORS:-system,hz3,hz4,hz6,transfer_presence,owner_inbox_external,small_class_shard,toy2_split,route_before_maps,mimalloc,tcmalloc}"
ROWS_CSV="${ROWS:-remote50,remote90,cross128_r90}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_remote_allocator_compare_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0
SKIP_PREPARE_ALLOCATORS=0
KEEP_GOING=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_remote_allocator_compare.sh [options]

Options:
  --arch ARCH        target arch for legacy builds/prep (default: x86_64)
  --runs N           repeat count per row/allocator (default: 3)
  --allocators CSV   system,hz3,hz4,hz6,transfer_presence,owner_inbox_external,small_class_shard,toy2_split,route_before_maps,mimalloc,tcmalloc
  --rows CSV         local0,remote50,remote90,remote90_short,cross128_r90
  --outdir DIR       output directory
  --skip-builds      reuse existing HZ and benchmark builds
  --skip-prepare     reuse existing MIMALLOC_SO/TCMALLOC_SO
  --keep-going       keep collecting after an allocator/run failure
  --help             show this message

This is an observe-only MT remote allocator frontier.  HZ6 profile aliases are
measured through their real preload DSOs; external allocators use bench_common
lookup.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --allocators) ALLOCATORS_CSV="$2"; shift 2 ;;
    --rows) ROWS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-builds) SKIP_BUILDS=1; shift ;;
    --skip-prepare) SKIP_PREPARE_ALLOCATORS=1; shift ;;
    --keep-going) KEEP_GOING=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][remote-allocator-compare] missing bench: $BENCH" >&2
  exit 2
fi

mkdir -p "$OUTDIR"

allocators=()
IFS=',' read -r -a raw_allocators <<< "$ALLOCATORS_CSV"
for allocator in "${raw_allocators[@]}"; do
  case "$allocator" in
    system|hz3|hz4|hz6|transfer_presence|owner_inbox_external|small_class_shard|toy2_split|route_before_maps|mimalloc|tcmalloc)
      allocators+=("$allocator")
      ;;
    "") ;;
    *) echo "unknown allocator: $allocator" >&2; exit 2 ;;
  esac
done

rows=()
IFS=',' read -r -a raw_rows <<< "$ROWS_CSV"
for row_name in "${raw_rows[@]}"; do
  case "$row_name" in
    local0) rows+=("local0 16 10000 100 16 32768 0 65536") ;;
    remote50) rows+=("remote50 16 10000 100 16 32768 50 65536") ;;
    remote90) rows+=("remote90 16 120000 100 16 131072 90 65536") ;;
    remote90_short) rows+=("remote90_short 16 20000 100 16 131072 90 65536") ;;
    cross128_r90) rows+=("cross128_r90 16 120000 100 128 128 90 65536") ;;
    "") ;;
    *) echo "unknown row: $row_name" >&2; exit 2 ;;
  esac
done

lookup_name() {
  case "$1" in
    transfer_presence) echo "hz6-high-remote-transfer-presence-target" ;;
    owner_inbox_external) echo "hz6-high-remote-owner-inbox-target" ;;
    small_class_shard) echo "hz6-transfer-small-class-shard-target" ;;
    toy2_split) echo "hz6-cross128-toy2-split-target" ;;
    route_before_maps) echo "hz6-cross128-toy2-route-before-maps-target" ;;
    *) echo "$1" ;;
  esac
}

if [[ "$SKIP_BUILDS" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_release_lane.sh" --arch "$ARCH"
  "${HZ6_DIR}/linux/build_hz6_preload.sh"
  hz6_preload_build_requested_aliases \
    "${ALLOCATORS_CSV/transfer_presence/hz6-high-remote-transfer-presence-target}" \
    "$ROOT_DIR"
  hz6_preload_build_requested_aliases \
    "${ALLOCATORS_CSV/owner_inbox_external/hz6-high-remote-owner-inbox-target}" \
    "$ROOT_DIR"
  hz6_preload_build_requested_aliases \
    "${ALLOCATORS_CSV/small_class_shard/hz6-transfer-small-class-shard-target}" \
    "$ROOT_DIR"
  hz6_preload_build_requested_aliases \
    "${ALLOCATORS_CSV/toy2_split/hz6-cross128-toy2-split-target}" \
    "$ROOT_DIR"
  hz6_preload_build_requested_aliases \
    "${ALLOCATORS_CSV/route_before_maps/hz6-cross128-toy2-route-before-maps-target}" \
    "$ROOT_DIR"
fi

if [[ "$SKIP_PREPARE_ALLOCATORS" -ne 1 ]]; then
  env_file="${OUTDIR}/allocator_env.sh"
  "${ROOT_DIR}/linux/prepare_linux_bench_allocators.sh" --arch "$ARCH" \
    > "$env_file"
  # shellcheck disable=SC1090
  source "$env_file"
fi

{
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "allocators=${ALLOCATORS_CSV}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH}"
  echo "keep_going=${KEEP_GOING}"
  echo "git_sha=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo nogit)"
  echo "uname=$(uname -a)"
} > "${OUTDIR}/README.log"

printf 'allocator\trow\trun\tstatus\tops_s\tpeak_kb\tfallback_rate\toverflow_sent\toverflow_received\tlog\n' \
  > "${OUTDIR}/runs.tsv"

run_one() {
  local allocator="$1"
  local row="$2"
  local run="$3"
  shift 3
  local lookup
  lookup="$(lookup_name "$allocator")"
  local lib_path=""
  if [[ "$lookup" != "system" ]]; then
    lib_path="$(bench_find_allocator_library "$lookup" || true)"
    if [[ -z "$lib_path" ]]; then
      echo "[hz6][remote-allocator-compare] missing allocator library: $allocator" >&2
      bench_print_allocator_hints "$lookup"
      exit 3
    fi
  fi

  local log="${OUTDIR}/${allocator}_${row}_r${run}.log"
  local time_log="${OUTDIR}/${allocator}_${row}_r${run}.time"
  local status=0
  if [[ "$lookup" == "system" ]]; then
    timeout "$RUN_TIMEOUT" /usr/bin/time -f 'peak_kb=%M' -o "$time_log" \
      "$BENCH" "$@" > "$log" 2>&1 || status=$?
  else
    timeout "$RUN_TIMEOUT" /usr/bin/time -f 'peak_kb=%M' -o "$time_log" \
      env LD_PRELOAD="$lib_path" "$BENCH" "$@" \
      > "$log" 2>&1 || status=$?
  fi

  local ops peak fallback overflow_sent overflow_received
  ops="$(awk -F'ops/s=' '/bench_random_mixed_mt_remote/ { print $2; exit }' "$log")"
  peak="$(awk -F= '/^peak_kb=/ { print $2; exit }' "$time_log")"
  fallback="$(awk -F'fallback_rate=' '/fallback_rate=/ { split($2, a, "%"); print a[1]; exit }' "$log")"
  overflow_sent="$(awk -F'overflow_sent=' '/overflow_sent=/ { split($2, a, " "); print a[1]; exit }' "$log")"
  overflow_received="$(awk -F'overflow_received=' '/overflow_received=/ { split($2, a, " "); print a[1]; exit }' "$log")"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$allocator" "$row" "$run" "$status" "${ops:-NA}" "${peak:-NA}" \
    "${fallback:-NA}" "${overflow_sent:-NA}" "${overflow_received:-NA}" "$log" \
    >> "${OUTDIR}/runs.tsv"
  echo "[hz6][remote-allocator-compare] ${allocator} ${row} run=${run} status=${status} ops/s=${ops:-NA} peak_kb=${peak:-NA}" >&2
  if [[ "$status" -ne 0 && "$KEEP_GOING" -ne 1 ]]; then
    exit "$status"
  fi
}

for allocator in "${allocators[@]}"; do
  for row in "${rows[@]}"; do
    read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
    for run in $(seq 1 "$RUNS"); do
      run_one "$allocator" "$name" "$run" \
        "$threads" "$iters" "$ws" "$min_size" "$max_size" "$remote_pct" "$ring_slots"
    done
  done
done

python3 - "$OUTDIR/runs.tsv" <<'PY' | tee "${OUTDIR}/summary.tsv"
import csv
import statistics
import sys

rows = {}
with open(sys.argv[1], newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        if row["status"] != "0" or row["ops_s"] == "NA" or row["peak_kb"] == "NA":
            continue
        key = (row["allocator"], row["row"])
        rows.setdefault(key, {"ops": [], "peak": []})
        rows[key]["ops"].append(float(row["ops_s"]))
        rows[key]["peak"].append(float(row["peak_kb"]) / 1024.0)

print("allocator\trow\truns\tops_min\tops_median\tops_max\tpeak_mib_median")
for key in sorted(rows):
    ops = rows[key]["ops"]
    peak = rows[key]["peak"]
    print(
        f"{key[0]}\t{key[1]}\t{len(ops)}\t"
        f"{min(ops):.2f}\t{statistics.median(ops):.2f}\t{max(ops):.2f}\t"
        f"{statistics.median(peak):.2f}"
    )
PY

echo "[DONE] ${OUTDIR}" >&2
