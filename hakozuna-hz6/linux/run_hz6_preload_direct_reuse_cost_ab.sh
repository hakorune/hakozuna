#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-60}"
VARIANTS_CSV="${VARIANTS:-p0_selected,p1_inbox,p2_gate,p3_claim,p4_source_gate}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_direct_reuse_cost_ab_$(date +%Y%m%d_%H%M%S)}"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_direct_reuse_cost_ab.sh [options]

Options:
  --runs N       repeat count per row and variant (default: 3)
  --variants CSV p0_selected,p1_inbox,p2_gate,p3_claim,p4_source_gate (default: all)
  --outdir DIR   output directory
  --help         show this message

Variants:
  p0_selected  selected/off
  p1_inbox     owner inbox + owner-local exact maintenance, DirectReuse off
  p2_gate      p1 + DirectReuse compiled, exact-key gate only, no claim
  p3_claim     p1 + DirectReuse claim/route/activation behavior
  p4_source_gate p1 + DirectReuse claim only at source-demand boundary
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2 ;;
    --variants) VARIANTS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][direct-cost] missing bench: $BENCH" >&2
  exit 2
fi

mkdir -p "$OUTDIR/build"

variants=()
IFS=',' read -r -a raw_variants <<< "$VARIANTS_CSV"
for variant in "${raw_variants[@]}"; do
  case "$variant" in
    p0_selected|p1_inbox|p2_gate|p3_claim|p4_source_gate) variants+=("$variant") ;;
    "") ;;
    *) echo "unknown variant: $variant" >&2; exit 2 ;;
  esac
done

build_variant() {
  local variant="$1"
  local flags=()
  hz6_preload_effective_selected_cflags flags 1
  case "$variant" in
    p0_selected)
      ;;
    p1_inbox)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 0
      ;;
    p2_gate)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 0
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 0
      ;;
    p3_claim)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 0
      ;;
    p4_source_gate)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_SOURCE_DEMAND_GATE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 0
      ;;
  esac
  OUT_DIR="${OUTDIR}/build/${variant}" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${flags[@]}")" \
    "${HZ6_DIR}/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/${variant}_build.log" 2>&1
}

rows=(
  "remote50 16 10000 100 16 32768 50 65536"
  "remote90 16 120000 100 16 131072 90 65536"
)

printf 'variant\trow\trun\tstatus\tops_s\tlog\n' > "${OUTDIR}/runs.tsv"

for variant in "${variants[@]}"; do
  echo "[hz6][direct-cost] building ${variant}" >&2
  build_variant "$variant"
  so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing preload: $so" >&2; exit 3; }
  for row in "${rows[@]}"; do
    read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
    for run in $(seq 1 "$RUNS"); do
      log="${OUTDIR}/${variant}_${name}_r${run}.log"
      status=0
      timeout "$RUN_TIMEOUT" env -i \
        PATH="${PATH:-/usr/bin:/bin}" \
        HOME="${HOME:-/tmp}" \
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
      ops="$(
        awk -F'ops/s=' '/bench_random_mixed_mt_remote/ { print $2; exit }' "$log"
      )"
      printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$variant" "$name" "$run" "$status" "${ops:-NA}" "$log" \
        >> "${OUTDIR}/runs.tsv"
      echo "[hz6][direct-cost] ${variant} ${name} run=${run} status=${status} ops/s=${ops:-NA}" >&2
      [[ "$status" -eq 0 ]] || exit "$status"
    done
  done
done

python3 - "$OUTDIR/runs.tsv" <<'PY' | tee "${OUTDIR}/summary.tsv"
import csv
import statistics
import sys

path = sys.argv[1]
rows = {}
with open(path, newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        if row["status"] != "0" or row["ops_s"] == "NA":
            continue
        rows.setdefault((row["variant"], row["row"]), []).append(float(row["ops_s"]))

print("variant\trow\tmedian_ops_s\truns")
for key in sorted(rows):
    values = rows[key]
    print(f"{key[0]}\t{key[1]}\t{statistics.median(values):.2f}\t{len(values)}")
PY

echo "[DONE] ${OUTDIR}"
