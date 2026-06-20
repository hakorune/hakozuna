#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-90}"
PROFILES_CSV="${PROFILES:-selected,transfer_presence,owner_inbox,direct_reuse,small_class_shard,toy2_split}"
ROWS_CSV="${ROWS:-remote50,remote90,cross128_r90}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_remote_profile_frontier_$(date +%Y%m%d_%H%M%S)}"
SKIP_BUILDS=0

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_remote_profile_frontier.sh [options]

Options:
  --runs N         repeat count per row/profile (default: 3)
  --profiles CSV   selected,transfer_presence,owner_inbox,direct_reuse,small_class_shard,toy2_split
  --rows CSV       local0,remote50,remote90,remote90_short,cross128_r90
  --outdir DIR     output directory
  --skip-builds    reuse existing DSOs
  --help           show this message

This is a production-shaped MT remote profile frontier.  It measures real
profile aliases, not internal tax-runner variants, and intentionally does not
enable HZ6_PRELOAD_STATS.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2 ;;
    --profiles) PROFILES_CSV="$2"; shift 2 ;;
    --rows) ROWS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-builds) SKIP_BUILDS=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][remote-profile-frontier] missing bench: $BENCH" >&2
  exit 2
fi

mkdir -p "$OUTDIR"

profiles=()
IFS=',' read -r -a raw_profiles <<< "$PROFILES_CSV"
for profile in "${raw_profiles[@]}"; do
  case "$profile" in
    selected|transfer_presence|owner_inbox|direct_reuse|small_class_shard|toy2_split)
      profiles+=("$profile")
      ;;
    "") ;;
    *) echo "unknown profile: $profile" >&2; exit 2 ;;
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

build_profile() {
  case "$1" in
    selected) "${HZ6_DIR}/linux/build_hz6_preload.sh" ;;
    transfer_presence) "${HZ6_DIR}/linux/build_hz6_preload_high_remote_transfer_presence_target.sh" ;;
    owner_inbox) "${HZ6_DIR}/linux/build_hz6_preload_high_remote_owner_inbox_target.sh" ;;
    direct_reuse) "${HZ6_DIR}/linux/build_hz6_preload_high_remote_owner_inbox_direct_reuse_target.sh" ;;
    small_class_shard) "${HZ6_DIR}/linux/build_hz6_preload_transfer_small_class_shard_target.sh" ;;
    toy2_split) "${HZ6_DIR}/linux/build_hz6_preload_cross128_toy2_split_target.sh" ;;
  esac
}

profile_so() {
  case "$1" in
    selected) echo "${HZ6_DIR}/out/linux/hz6_preload/libhakozuna_hz6_preload.so" ;;
    transfer_presence) echo "${HZ6_DIR}/out/linux/hz6_preload_high_remote_transfer_presence/libhakozuna_hz6_preload.so" ;;
    owner_inbox) echo "${HZ6_DIR}/out/linux/hz6_preload_high_remote_owner_inbox/libhakozuna_hz6_preload.so" ;;
    direct_reuse) echo "${HZ6_DIR}/out/linux/hz6_preload_high_remote_owner_inbox_direct_reuse/libhakozuna_hz6_preload.so" ;;
    small_class_shard) echo "${HZ6_DIR}/out/linux/hz6_preload_transfer_small_class_shard/libhakozuna_hz6_preload.so" ;;
    toy2_split) echo "${HZ6_DIR}/out/linux/hz6_preload_cross128_toy2_split/libhakozuna_hz6_preload.so" ;;
  esac
}

{
  echo "runs=${RUNS}"
  echo "profiles=${PROFILES_CSV}"
  echo "rows=${ROWS_CSV}"
  echo "bench=${BENCH}"
  echo "git_sha=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo nogit)"
  echo "uname=$(uname -a)"
} > "${OUTDIR}/README.log"

for profile in "${profiles[@]}"; do
  if [[ "$SKIP_BUILDS" -ne 1 ]]; then
    echo "[hz6][remote-profile-frontier] building ${profile}" >&2
    build_profile "$profile" > "${OUTDIR}/${profile}_build.log" 2>&1
  fi
  so="$(profile_so "$profile")"
  [[ -f "$so" ]] || { echo "missing preload for ${profile}: ${so}" >&2; exit 3; }
done

printf 'profile\trow\trun\tstatus\tops_s\tpeak_kb\tlog\n' > "${OUTDIR}/runs.tsv"

for profile in "${profiles[@]}"; do
  so="$(profile_so "$profile")"
  for row in "${rows[@]}"; do
    read -r name threads iters ws min_size max_size remote_pct ring_slots <<<"$row"
    for run in $(seq 1 "$RUNS"); do
      log="${OUTDIR}/${profile}_${name}_r${run}.log"
      time_log="${OUTDIR}/${profile}_${name}_r${run}.time"
      status=0
      timeout "$RUN_TIMEOUT" /usr/bin/time -f 'peak_kb=%M' -o "$time_log" \
        env -i PATH="${PATH:-/usr/bin:/bin}" HOME="${HOME:-/tmp}" \
          LD_PRELOAD="$so" \
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
      printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$profile" "$name" "$run" "$status" "${ops:-NA}" "${peak:-NA}" "$log" \
        >> "${OUTDIR}/runs.tsv"
      echo "[hz6][remote-profile-frontier] ${profile} ${name} run=${run} status=${status} ops/s=${ops:-NA} peak_kb=${peak:-NA}" >&2
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
        key = (row["profile"], row["row"])
        rows.setdefault(key, {"ops": [], "peak": []})
        rows[key]["ops"].append(float(row["ops_s"]))
        rows[key]["peak"].append(float(row["peak_kb"]) / 1024.0)

print("profile\trow\truns\tops_min\tops_median\tops_max\tpeak_mib_median")
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
