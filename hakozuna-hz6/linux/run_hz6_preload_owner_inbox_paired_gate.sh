#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-90}"
VARIANTS_CSV="${VARIANTS:-p0_selected_off,p1_owner_inbox}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_owner_inbox_paired_gate_$(date +%Y%m%d_%H%M%S)}"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_paired_gate.sh [options]

Options:
  --runs N        repeat count per row and variant (default: 3)
  --variants CSV p0_selected_off,p1_owner_inbox,p0_transfer_class_presence
  --outdir DIR    output directory
  --help          show this message

Rows:
  local0        16 threads, remote_pct=0,  16..32768
  remote50      16 threads, remote_pct=50, 16..32768
  remote90      16 threads, remote_pct=90, 16..131072
  cross128_r90  16 threads, remote_pct=90, fixed 128-byte objects
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
  echo "[hz6][owner-inbox-paired] missing bench: $BENCH" >&2
  exit 2
fi
if [[ ! -x /usr/bin/time ]]; then
  echo "[hz6][owner-inbox-paired] missing /usr/bin/time" >&2
  exit 2
fi

variants=()
IFS=',' read -r -a raw_variants <<< "$VARIANTS_CSV"
for variant in "${raw_variants[@]}"; do
  case "$variant" in
    p0_selected_off|p1_owner_inbox|p0_transfer_class_presence)
      variants+=("$variant")
      ;;
    "") ;;
    *) echo "unknown variant: $variant" >&2; exit 2 ;;
  esac
done

mkdir -p "$OUTDIR/build"

write_meta() {
  {
    echo "runs=${RUNS}"
    echo "variants=${VARIANTS_CSV}"
    echo "bench=${BENCH}"
    echo "git_sha=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo nogit)"
    echo "uname=$(uname -a)"
  } > "${OUTDIR}/README.log"
}

build_variant() {
  local variant="$1"
  local flags=()
  case "$variant" in
    p0_selected_off)
      hz6_preload_effective_owner_inbox_off_cflags flags 1
      ;;
    p1_owner_inbox)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      ;;
    p0_transfer_class_presence)
      hz6_preload_effective_owner_inbox_off_cflags flags 1
      hz6_preload_replace_define flags HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1 1
      ;;
  esac
  OUT_DIR="${OUTDIR}/build/${variant}" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${flags[@]}")" \
    "${HZ6_DIR}/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/${variant}_build.log" 2>&1
}

rows=(
  "local0 16 10000 100 16 32768 0 65536"
  "remote50 16 10000 100 16 32768 50 65536"
  "remote90 16 120000 100 16 131072 90 65536"
  "cross128_r90 16 120000 100 128 128 90 65536"
)

write_meta
printf 'variant\trow\trun\tstatus\tops_s\tpeak_kb\tlog\n' > "${OUTDIR}/runs.tsv"

for variant in "${variants[@]}"; do
  echo "[hz6][owner-inbox-paired] building ${variant}" >&2
  build_variant "$variant"
  so="${OUTDIR}/build/${variant}/libhakozuna_hz6_preload.so"
  [[ -f "$so" ]] || { echo "missing preload: $so" >&2; exit 3; }
  for row in "${rows[@]}"; do
    read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
    for run in $(seq 1 "$RUNS"); do
      log="${OUTDIR}/${variant}_${name}_r${run}.log"
      time_log="${OUTDIR}/${variant}_${name}_r${run}.time"
      status=0
      timeout "$RUN_TIMEOUT" /usr/bin/time -f 'peak_kb=%M' -o "$time_log" \
        env -i \
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
      ops="$(awk -F'ops/s=' '/bench_random_mixed_mt_remote/ { print $2; exit }' "$log")"
      peak="$(awk -F= '/^peak_kb=/ { print $2; exit }' "$time_log")"
      printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$variant" "$name" "$run" "$status" "${ops:-NA}" \
        "${peak:-NA}" "$log" >> "${OUTDIR}/runs.tsv"
      echo "[hz6][owner-inbox-paired] ${variant} ${name} run=${run} status=${status} ops/s=${ops:-NA} peak_kb=${peak:-NA}" >&2
      [[ "$status" -eq 0 ]] || exit "$status"
    done
  done
done

python3 - "$OUTDIR/runs.tsv" <<'PY' | tee "${OUTDIR}/summary.tsv"
import csv
import statistics
import sys

def pct(values, numerator, denominator):
    if not values:
        return float("nan")
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    index = round((len(ordered) - 1) * numerator / denominator)
    return ordered[index]

rows = {}
with open(sys.argv[1], newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        if row["status"] != "0" or row["ops_s"] == "NA" or row["peak_kb"] == "NA":
            continue
        key = (row["variant"], row["row"])
        rows.setdefault(key, {"ops": [], "peak": []})
        rows[key]["ops"].append(float(row["ops_s"]))
        rows[key]["peak"].append(float(row["peak_kb"]))

print(
    "variant\trow\truns\t"
    "ops_min\tops_p25\tops_median\tops_p75\tops_max\t"
    "peak_mib_min\tpeak_mib_p25\tpeak_mib_median\tpeak_mib_p75\tpeak_mib_max"
)
for key in sorted(rows):
    ops = rows[key]["ops"]
    peak_mib = [value / 1024.0 for value in rows[key]["peak"]]
    print(
        f"{key[0]}\t{key[1]}\t{len(ops)}\t"
        f"{min(ops):.2f}\t{pct(ops, 1, 4):.2f}\t{statistics.median(ops):.2f}\t"
        f"{pct(ops, 3, 4):.2f}\t{max(ops):.2f}\t"
        f"{min(peak_mib):.2f}\t{pct(peak_mib, 1, 4):.2f}\t"
        f"{statistics.median(peak_mib):.2f}\t{pct(peak_mib, 3, 4):.2f}\t"
        f"{max(peak_mib):.2f}"
    )
PY

echo "[DONE] ${OUTDIR}"
