#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-60}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_owner_inbox_profile_guard_$(date +%Y%m%d_%H%M%S)}"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_guard.sh [options]

Options:
  --runs N      repeat count per row (default: 3)
  --outdir DIR  output directory
  --help        show this message

Rows:
  local0   16 threads, remote_pct=0,  16..32768
  remote50 16 threads, remote_pct=50, 16..32768
  remote90 16 threads, remote_pct=90, 16..131072

This builds the explicit owner-inbox external high-remote profile DSO.  The
profile is intentionally separate from selected/default so promotion can be
judged independently.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][owner-inbox-guard] missing bench: $BENCH" >&2
  exit 2
fi
if [[ ! -x /usr/bin/time ]]; then
  echo "[hz6][owner-inbox-guard] missing /usr/bin/time" >&2
  exit 2
fi

mkdir -p "$OUTDIR"
"${HZ6_DIR}/linux/build_hz6_preload_high_remote_owner_inbox_target.sh" \
  > "${OUTDIR}/build.log" 2>&1
DSO="${HZ6_DIR}/out/linux/hz6_preload_high_remote_owner_inbox/libhakozuna_hz6_preload.so"

rows=(
  "local0 16 10000 100 16 32768 0 65536"
  "remote50 16 10000 100 16 32768 50 65536"
  "remote90 16 120000 100 16 131072 90 65536"
)

printf 'row\trun\tstatus\tops_s\tpeak_kb\tlog\n' > "${OUTDIR}/runs.tsv"

for row in "${rows[@]}"; do
  read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
  for run in $(seq 1 "$RUNS"); do
    log="${OUTDIR}/${name}_r${run}.log"
    time_log="${OUTDIR}/${name}_r${run}.time"
    status=0
    timeout "$RUN_TIMEOUT" /usr/bin/time -f 'peak_kb=%M' -o "$time_log" \
      env -i \
        PATH="${PATH:-/usr/bin:/bin}" \
        HOME="${HOME:-/tmp}" \
        LD_PRELOAD="$DSO" \
        "$BENCH" "$threads" "$iters" "$ws" "$min_size" "$max_size" \
        "$remote_pct" "$ring_slots" \
        > "$log" 2>&1 || status=$?
    if [[ "$status" -eq 0 ]] && ! grep -q 'fallback_rate=0.00%' "$log"; then
      status=30
    fi
    if [[ "$status" -eq 0 ]] &&
       ! grep -q 'overflow_sent=0 overflow_received=0' "$log"; then
      status=40
    fi
    ops="$(awk -F'ops/s=' '/bench_random_mixed_mt_remote/ { print $2; exit }' "$log")"
    peak="$(awk -F= '/^peak_kb=/ { print $2; exit }' "$time_log")"
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$name" "$run" "$status" "${ops:-NA}" "${peak:-NA}" "$log" \
      >> "${OUTDIR}/runs.tsv"
    echo "[hz6][owner-inbox-guard] ${name} run=${run} status=${status} ops/s=${ops:-NA} peak_kb=${peak:-NA}" >&2
    [[ "$status" -eq 0 ]] || exit "$status"
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
        rows.setdefault(row["row"], {"ops": [], "peak": []})
        rows[row["row"]]["ops"].append(float(row["ops_s"]))
        rows[row["row"]]["peak"].append(float(row["peak_kb"]))

print("row\tmedian_ops_s\tmedian_peak_mib\truns")
for name in sorted(rows):
    ops = rows[name]["ops"]
    peak = rows[name]["peak"]
    print(f"{name}\t{statistics.median(ops):.2f}\t{statistics.median(peak) / 1024.0:.2f}\t{len(ops)}")
PY

echo "[DONE] ${OUTDIR}"
