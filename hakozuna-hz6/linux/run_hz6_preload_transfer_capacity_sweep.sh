#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-90}"
CAPACITIES_CSV="${CAPACITIES:-128,256,512,1024}"
STATS="${STATS:-0}"
DIAGNOSTIC="${DIAGNOSTIC:-0}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_transfer_capacity_sweep_$(date +%Y%m%d_%H%M%S)}"

source "${HZ6_DIR}/linux/hz6_preload_profile_builder.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_transfer_capacity_sweep.sh [options]

Options:
  --runs N          repeat count per row/capacity (default: 3)
  --capacities CSV transfer cache capacities (default: 128,256,512,1024)
  --stats          pass HZ6_PRELOAD_STATS=1 for counter logs
  --diagnostic     build with HZ6_DIAGNOSTIC_PROBES=1 and enable stats
  --outdir DIR     output directory
  --help           show this message

Rows:
  local0   16 threads, remote_pct=0,  16..32768
  remote50 16 threads, remote_pct=50, 16..32768
  remote90 16 threads, remote_pct=90, 16..131072

This is an observation runner. It keeps selected/default behavior and only
changes transfer cache/profile capacity macros.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2 ;;
    --capacities) CAPACITIES_CSV="$2"; shift 2 ;;
    --stats) STATS=1; shift ;;
    --diagnostic) DIAGNOSTIC=1; STATS=1; shift ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][transfer-capacity] missing bench: $BENCH" >&2
  exit 2
fi
if [[ ! -x /usr/bin/time ]]; then
  echo "[hz6][transfer-capacity] missing /usr/bin/time" >&2
  exit 2
fi

capacities=()
IFS=',' read -r -a raw_capacities <<< "$CAPACITIES_CSV"
for capacity in "${raw_capacities[@]}"; do
  [[ -n "$capacity" ]] || continue
  if [[ ! "$capacity" =~ ^[0-9]+$ || "$capacity" -eq 0 ]]; then
    echo "invalid capacity: $capacity" >&2
    exit 2
  fi
  capacities+=("$capacity")
done
[[ "${#capacities[@]}" -gt 0 ]] || { echo "no capacities" >&2; exit 2; }

mkdir -p "$OUTDIR/build"
{
  echo "runs=${RUNS}"
  echo "capacities=${CAPACITIES_CSV}"
  echo "stats=${STATS}"
  echo "diagnostic=${DIAGNOSTIC}"
  echo "bench=${BENCH}"
  echo "git_sha=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo nogit)"
  echo "uname=$(uname -a)"
} > "${OUTDIR}/README.log"

build_capacity() {
  local capacity="$1"
  local flags=()
  hz6_preload_profile_selected_cflags flags 1
  hz6_preload_replace_define flags HZ6_TRANSFER_CACHE_CAPACITY "$capacity"
  hz6_preload_replace_define flags HZ6_PROFILE_SPEED_TRANSFER_CAPACITY "$capacity"
  hz6_preload_replace_define flags HZ6_PROFILE_REMOTE_TRANSFER_CAPACITY "$capacity"
  local extra_cflags=""
  local preserve_phase=0
  if [[ "$DIAGNOSTIC" -ne 0 ]]; then
    extra_cflags="-DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1"
    preserve_phase=1
  fi
  OUT_DIR="${OUTDIR}/build/cap${capacity}" \
  HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${flags[@]}")" \
    HZ6_EXTRA_CFLAGS="$extra_cflags" \
    HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS="$preserve_phase" \
    "${HZ6_DIR}/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/cap${capacity}_build.log" 2>&1
}

rows=(
  "local0 16 10000 100 16 32768 0 65536"
  "remote50 16 10000 100 16 32768 50 65536"
  "remote90 16 120000 100 16 131072 90 65536"
)

printf 'capacity\trow\trun\tstatus\tops_s\tpeak_kb\tlog\n' > "${OUTDIR}/runs.tsv"

for capacity in "${capacities[@]}"; do
  echo "[hz6][transfer-capacity] building cap${capacity}" >&2
  build_capacity "$capacity"
  so="${OUTDIR}/build/cap${capacity}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing preload: $so" >&2; exit 3; }
  for row in "${rows[@]}"; do
    read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
    for run in $(seq 1 "$RUNS"); do
      log="${OUTDIR}/cap${capacity}_${name}_r${run}.log"
      time_log="${OUTDIR}/cap${capacity}_${name}_r${run}.time"
      status=0
      timeout "$RUN_TIMEOUT" /usr/bin/time -f 'peak_kb=%M' -o "$time_log" \
        env -i \
          PATH="${PATH:-/usr/bin:/bin}" \
          HOME="${HOME:-/tmp}" \
          HZ6_PRELOAD_STATS="$STATS" \
          LD_PRELOAD="$so" \
          "$BENCH" "$threads" "$iters" "$ws" "$min_size" "$max_size" \
          "$remote_pct" "$ring_slots" \
          > "$log" 2>&1 || status=$?
      if [[ "$status" -eq 0 ]] &&
         ! grep -q 'fallback_rate=0.00%' "$log"; then
        status=30
      fi
      if [[ "$status" -eq 0 ]] &&
         ! grep -q 'overflow_sent=0 overflow_received=0' "$log"; then
        status=40
      fi
      ops="$(awk -F'ops/s=' '/bench_random_mixed_mt_remote/ { print $2; exit }' "$log")"
      peak="$(awk -F= '/^peak_kb=/ { print $2; exit }' "$time_log")"
      printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$capacity" "$name" "$run" "$status" "${ops:-NA}" \
        "${peak:-NA}" "$log" >> "${OUTDIR}/runs.tsv"
      echo "[hz6][transfer-capacity] cap${capacity} ${name} run=${run} status=${status} ops/s=${ops:-NA} peak_kb=${peak:-NA}" >&2
      [[ "$status" -eq 0 ]] || exit "$status"
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
        key = (int(row["capacity"]), row["row"])
        rows.setdefault(key, {"ops": [], "peak": []})
        rows[key]["ops"].append(float(row["ops_s"]))
        rows[key]["peak"].append(float(row["peak_kb"]))

print("capacity\trow\tmedian_ops_s\tmedian_peak_mib\truns")
for key in sorted(rows):
    ops = rows[key]["ops"]
    peak = rows[key]["peak"]
    print(f"{key[0]}\t{key[1]}\t{statistics.median(ops):.2f}\t{statistics.median(peak) / 1024.0:.2f}\t{len(ops)}")
PY

python3 - "$OUTDIR/runs.tsv" <<'PY' > "${OUTDIR}/counters.tsv"
import csv
import re
import sys

counters = [
    "remote_free_foreign_candidate",
    "remote_free_status_backpressure",
    "remote_free_returned_backpressure",
    "transfer_reserve_attempt",
    "transfer_reserve_success",
    "transfer_reserve_full",
    "transfer_reserve_full_transfer_count_total",
    "transfer_reserve_full_class_count_total",
    "transfer_reserve_full_class_count_max",
    "remote_free_backpressure_origin_transfer_success",
    "remote_free_backpressure_origin_transfer_full",
    "remote_free_backpressure_origin_full_transfer_count_total",
    "remote_free_backpressure_origin_full_class_count_total",
    "remote_free_backpressure_origin_full_class_count_max",
    "route_rehome_commit_success",
]
patterns = {
    name: re.compile(rf"(?:^| ){re.escape(name)}=([0-9]+)(?: |$)")
    for name in counters
}

print("\t".join(["capacity", "row", "run"] + counters))
with open(sys.argv[1], newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        values = {name: "NA" for name in counters}
        try:
            text = open(row["log"], encoding="utf-8", errors="replace").read()
        except OSError:
            text = ""
        for name, pattern in patterns.items():
            matches = pattern.findall(text)
            if matches:
                values[name] = matches[-1]
        print("\t".join([
            row["capacity"],
            row["row"],
            row["run"],
            *[values[name] for name in counters],
        ]))
PY

echo "[DONE] ${OUTDIR}"
