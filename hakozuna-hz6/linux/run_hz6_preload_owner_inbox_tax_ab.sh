#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
RUNS="${RUNS:-3}"
RUN_TIMEOUT="${HZ6_PRELOAD_REMOTE_TIMEOUT:-90}"
VARIANTS_CSV="${VARIANTS:-p0_off,p1_metadata,p1_inline_no_maintenance,p1_inline,p1_external_no_maintenance,p1_external,p1_external_split_maintenance}"
ROWS_CSV="${ROWS:-local0,remote50}"
OUTDIR="${OUTDIR:-${HZ6_DIR}/private/raw-results/linux/hz6_owner_inbox_tax_ab_$(date +%Y%m%d_%H%M%S)}"
DIAGNOSTIC="${HZ6_OWNER_INBOX_TAX_DIAGNOSTIC:-1}"

source "${HZ6_DIR}/linux/hz6_preload_flags.sh"

usage() {
  cat <<'EOF'
Usage:
  ./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh [options]

Options:
  --runs N        repeat count per row and variant (default: 3)
  --variants CSV p0_off,p0_transfer_class_presence,p0_no_origin_transfer,p1_metadata,p1_inline_no_maintenance,p1_inline,p1_external_no_maintenance,p1_external,p1_external_direct_reuse,p1_external_direct_reuse_observe,p1_external_source_gate,p1_external_source_gate_observe,p1_external_inline_skip_mid5,p1_external_inline_skip_mid4,p1_external_route_pin,p1_external_split_maintenance,p1_external_small_class
  --rows CSV     local0,remote50,remote90,remote90_short,cross128_r90 (default: local0,remote50)
  --outdir DIR    output directory
  --diagnostic    build with HZ6_DIAGNOSTIC_PROBES=1 for counter attribution (default)
  --production    build production-shaped DSOs; some counters will be NA
  --help          show this message

Variants:
  p0_off                     owner-inbox family forced off
  p0_transfer_class_presence p0_off plus TransferClassPresenceGate-L1
  p0_no_origin_transfer      p0_off plus origin-transfer backpressure off
  p1_metadata                inbox structs compiled in, producer/consumer off
  p1_inline_no_maintenance   inline owner-inbox producer on, consumer off
  p1_inline                  inline producer and owner-local maintenance on
  p1_external_no_maintenance inline+external producer on, consumer off
  p1_external                branch-selected owner-inbox external candidate
  p1_external_direct_reuse   p1_external plus frontcache-miss DirectReuse
  p1_external_direct_reuse_observe
                              p1_external_direct_reuse with DirectReuse counters on
  p1_external_source_gate    p1_external plus source-boundary DirectReuse
  p1_external_source_gate_observe
                              p1_external_source_gate with DirectReuse counters on
  p1_external_inline_skip_mid5
                              skip inline MidPage class >=5, preserve external sink
  p1_external_inline_skip_mid4
                              skip inline MidPage class >=4, preserve external sink
  p1_external_route_pin      p1_external plus production inline route-pin trust
  p1_external_split_maintenance
                              external-only front maintenance, full source-gate maintenance
  p1_external_small_class    p1_external plus HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID=3

Rows:
  local0   16 threads, remote_pct=0,  16..32768
  remote50 16 threads, remote_pct=50, 16..32768
  remote90 16 threads, remote_pct=90, 16..131072
  remote90_short
           diagnostic-only short remote90 observe row, not a promotion gate

This is an attribution runner, not a promotion gate.  Some intermediate
variants intentionally reintroduce returned backpressure so the cost of each
box can be separated.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --runs) RUNS="$2"; shift 2 ;;
    --variants) VARIANTS_CSV="$2"; shift 2 ;;
    --rows) ROWS_CSV="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --diagnostic) DIAGNOSTIC=1; shift ;;
    --production) DIAGNOSTIC=0; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][owner-inbox-tax] missing bench: $BENCH" >&2
  exit 2
fi
if [[ ! -x /usr/bin/time ]]; then
  echo "[hz6][owner-inbox-tax] missing /usr/bin/time" >&2
  exit 2
fi

variants=()
IFS=',' read -r -a raw_variants <<< "$VARIANTS_CSV"
for variant in "${raw_variants[@]}"; do
  case "$variant" in
    p0_off|p0_transfer_class_presence|p0_transfer_class_presence_min192|p0_no_origin_transfer|p1_metadata|p1_inline_no_maintenance|p1_inline|p1_external_no_maintenance|p1_external|p1_external_direct_reuse|p1_external_direct_reuse_observe|p1_external_source_gate|p1_external_source_gate_observe|p1_external_inline_skip_mid5|p1_external_inline_skip_mid4|p1_external_route_pin|p1_external_split_maintenance|p1_external_small_class)
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
    echo "rows=${ROWS_CSV}"
    echo "diagnostic=${DIAGNOSTIC}"
    echo "bench=${BENCH}"
    echo "git_sha=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo nogit)"
    echo "uname=$(uname -a)"
    echo "note=owner-inbox tax attribution; intermediate variants are not correctness gates"
  } > "${OUTDIR}/README.log"
}

build_variant() {
  local variant="$1"
  local flags=()
  hz6_preload_effective_owner_inbox_off_cflags flags 1
  case "$variant" in
    p0_off)
      ;;
    p0_transfer_class_presence)
      hz6_preload_replace_define flags HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1 1
      ;;
    p0_transfer_class_presence_min192)
      hz6_preload_replace_define flags HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1 1
      hz6_preload_replace_define flags HZ6_TRANSFER_CLASS_PRESENCE_MIN_TOTAL '((size_t)192)'
      ;;
    p0_no_origin_transfer)
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1 0
      ;;
    p1_metadata)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 0
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 0
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1 0
      ;;
    p1_inline_no_maintenance)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 0
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1 0
      ;;
    p1_inline)
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INBOX_CORE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_FREE_BACKPRESSURE_OWNER_INBOX_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1 0
      ;;
    p1_external_no_maintenance)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 0
      ;;
    p1_external)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      ;;
    p1_external_direct_reuse)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 0
      ;;
    p1_external_direct_reuse_observe)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 1
      ;;
    p1_external_source_gate)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_SOURCE_DEMAND_GATE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 0
      ;;
    p1_external_source_gate_observe)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_CLAIM_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_SOURCE_DEMAND_GATE_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_DIRECT_OBSERVE_L1 1
      ;;
    p1_external_inline_skip_mid5)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INLINE_MIDPAGE_MIN_CLASS 5
      ;;
    p1_external_inline_skip_mid4)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_INLINE_MIDPAGE_MIN_CLASS 4
      ;;
    p1_external_route_pin)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_ROUTE_PIN_TRUST_L1 1
      ;;
    p1_external_split_maintenance)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_FRONT_MAINTENANCE_EXTERNAL_ONLY_L1 1
      hz6_preload_replace_define flags HZ6_REMOTE_PENDING_SOURCE_GATE_MAINTENANCE_L1 1
      ;;
    p1_external_small_class)
      hz6_preload_effective_owner_inbox_external_cflags flags 1
      hz6_preload_replace_define flags HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID 3
      ;;
  esac
  OUT_DIR="${OUTDIR}/build/${variant}" \
    HZ6_PRELOAD_PRESERVE_PHASE_COUNTERS="$DIAGNOSTIC" \
    HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-}$([[ "$DIAGNOSTIC" -ne 0 ]] && printf ' -DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1 -DHZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1=1')" \
    HZ6_PRELOAD_DEFAULT_CFLAGS="$(hz6_preload_join_flags "${flags[@]}")" \
    "${HZ6_DIR}/linux/build_hz6_preload.sh" \
    > "${OUTDIR}/${variant}_build.log" 2>&1
}

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

counter_keys=(
  remote_free_returned_backpressure
  remote_free_returned_uncommitted
  remote_free_origin_pending_commit
  transfer_pop
  source_alloc
  frontcache_reuse_hit
  midpage_source_alloc
  toy_source_alloc
  large_source_alloc
  route_unregister_while_pending
  route_replace_while_pending
  route_rehome_while_pending
  remote_pending_enqueue_success
  remote_pending_external_ticket_success
  remote_pending_maintenance_check
  remote_pending_maintenance_entry_gate_miss
  remote_pending_maintenance_inline_gate_hit
  remote_pending_maintenance_external_gate_hit
  remote_pending_maintenance_external_attempt
  remote_pending_maintenance_external_success
  remote_pending_maintenance_external_only_call
  remote_pending_maintenance_inline_deferred
  remote_pending_maintenance_inline_policy_skip
  remote_pending_maintenance_inline_pop_attempt
  remote_pending_maintenance_inline_pop_success
  remote_pending_maintenance_route_validate_inline
  remote_pending_maintenance_route_validate_external
  remote_pending_maintenance_frontcache_push_attempt_inline
  remote_pending_maintenance_frontcache_push_attempt_external
  remote_pending_maintenance_frontcache_push_success_inline
  remote_pending_maintenance_frontcache_push_success_external
  remote_pending_maintenance_drained_inline
  remote_pending_maintenance_drained_external
  remote_pending_batch_items
  remote_pending_current
  remote_pending_external_key_probe
  remote_pending_external_key_hit
  remote_pending_external_ticket_current
  remote_pending_external_ticket_duplicate_probe_total
  remote_pending_external_ticket_duplicate_probe_max
  remote_pending_external_ticket_duplicate_index_stale
  transfer_reserve_full
  transfer_class_presence_gate_check
  transfer_class_presence_gate_hit
  transfer_class_presence_gate_miss
  transfer_class_presence_below_min_check
  transfer_class_presence_below_min_hit
  transfer_class_presence_below_min_miss
  transfer_class_presence_invalid_class
  transfer_class_presence_underflow
  transfer_class_presence_over_capacity
  transfer_class_presence_placeholder_counted
  transfer_class_presence_double_increment
  transfer_class_presence_double_decrement
  transfer_class_presence_sum_mismatch
  transfer_class_presence_scan_mismatch
  transfer_class_presence_false_zero_shadow
  remote_free_backpressure_origin_transfer_success
  remote_free_backpressure_origin_transfer_fail
  remote_free_backpressure_origin_transfer_stride_skip
  remote_free_backpressure_origin_transfer_validation_fail
  remote_free_backpressure_origin_transfer_full
  remote_free_backpressure_origin_full_transfer_count_total
  remote_free_backpressure_origin_full_class_count_total
  remote_free_backpressure_origin_full_class_count_max
  remote_free_backpressure_origin_drain_attempt
  remote_free_backpressure_origin_drain_success
  remote_free_backpressure_origin_drain_retry_success
  origin_transfer_source_commit_with_same_class_transfer
  origin_transfer_source_commit_with_any_transfer
  origin_transfer_source_commit_same_class_count_total
  origin_transfer_source_commit_same_class_count_max
  origin_transfer_source_commit_transfer_count_total
  origin_transfer_source_commit_transfer_count_max
  origin_transfer_prefill_commit_with_same_class_transfer
  origin_transfer_prefill_commit_with_any_transfer
  origin_transfer_prefill_commit_same_class_count_total
  origin_transfer_prefill_commit_same_class_count_max
  origin_transfer_prefill_commit_transfer_count_total
  origin_transfer_prefill_commit_transfer_count_max
  origin_transfer_pop_loop_attempt
  origin_transfer_pop_loop_empty
  origin_transfer_pop_empty_with_any_transfer
  origin_transfer_pop_empty_transfer_count_total
  origin_transfer_pop_empty_transfer_count_max
  origin_transfer_pop_loop_invalid
  origin_transfer_pop_loop_hit
  origin_transfer_full_transfer_count_max
  origin_transfer_full_same_class_zero
  origin_transfer_full_same_class_lt_quarter
  origin_transfer_full_same_class_lt_half
  origin_transfer_full_same_class_lt_3quarter
  origin_transfer_full_same_class_ge_3quarter
  origin_phase_audit_destination_publish
  origin_phase_audit_origin_fallback_publish
  origin_phase_audit_commit_without_stamp
  origin_phase_audit_pop_without_stamp
  origin_phase_audit_unknown_publish_kind
  origin_phase_audit_invalid_class
  origin_phase_audit_class_count_underflow
  origin_phase_audit_class_count_over_capacity
  origin_phase_audit_occupancy_sum_mismatch
  origin_phase_audit_producer_count_underflow
  origin_phase_audit_generation_mismatch
  origin_phase_audit_epoch_order_invalid
  origin_phase_audit_pop_global_age_0
  origin_phase_audit_pop_global_age_1
  origin_phase_audit_pop_global_age_2_3
  origin_phase_audit_pop_global_age_4_7
  origin_phase_audit_pop_global_age_8_15
  origin_phase_audit_pop_global_age_16_63
  origin_phase_audit_pop_global_age_64_plus
  origin_phase_audit_pop_class_age_0
  origin_phase_audit_pop_class_age_1
  origin_phase_audit_pop_class_age_2_3
  origin_phase_audit_pop_class_age_4_7
  origin_phase_audit_pop_class_age_8_15
  origin_phase_audit_pop_class_age_16_63
  origin_phase_audit_pop_class_age_64_plus
  origin_phase_audit_full_requested_age_0
  origin_phase_audit_full_requested_age_1
  origin_phase_audit_full_requested_age_2_3
  origin_phase_audit_full_requested_age_4_7
  origin_phase_audit_full_requested_age_8_15
  origin_phase_audit_full_requested_age_16_63
  origin_phase_audit_full_requested_age_64_plus
  origin_phase_audit_empty_requested_age_0
  origin_phase_audit_empty_requested_age_1
  origin_phase_audit_empty_requested_age_2_3
  origin_phase_audit_empty_requested_age_4_7
  origin_phase_audit_empty_requested_age_8_15
  origin_phase_audit_empty_requested_age_16_63
  origin_phase_audit_empty_requested_age_64_plus
  origin_phase_audit_full_destination_occupancy_total
  origin_phase_audit_full_origin_fallback_occupancy_total
  origin_phase_audit_full_destination_occupancy_max
  origin_phase_audit_full_origin_fallback_occupancy_max
  origin_phase_audit_full_same_class_destination_total
  origin_phase_audit_full_same_class_origin_fallback_total
  origin_phase_audit_full_cross_class_destination_total
  origin_phase_audit_full_cross_class_origin_fallback_total
  origin_phase_audit_full_cross_resident_fresh_total
  origin_phase_audit_full_cross_resident_recent_total
  origin_phase_audit_full_cross_resident_stale_total
  origin_phase_audit_empty_destination_occupancy_total
  origin_phase_audit_empty_origin_fallback_occupancy_total
  origin_phase_audit_empty_destination_occupancy_max
  origin_phase_audit_empty_origin_fallback_occupancy_max
  origin_phase_audit_empty_same_class_destination_total
  origin_phase_audit_empty_same_class_origin_fallback_total
  origin_phase_audit_empty_cross_class_destination_total
  origin_phase_audit_empty_cross_class_origin_fallback_total
  origin_phase_audit_empty_cross_resident_fresh_total
  origin_phase_audit_empty_cross_resident_recent_total
  origin_phase_audit_empty_cross_resident_stale_total
  origin_phase_audit_full_largest_producer_bucket_max
  origin_phase_audit_full_distinct_producer_bucket_max
  remote_pending_direct_gate_load
  remote_pending_direct_gate_hit
  remote_pending_direct_claim_attempt
  remote_pending_direct_claim_success
  remote_pending_direct_claim_busy
  remote_pending_direct_claim_empty_after_hint
  remote_pending_direct_activate_success
  remote_pending_direct_integrity_failure
  remote_pending_direct_claim_success_toy
  remote_pending_direct_claim_success_midpage
  remote_pending_direct_claim_while_transfer_nonempty
  remote_pending_direct_claim_while_frontcache_nonempty
  remote_pending_direct_claim_before_existing_reuse
  remote_pending_direct_claim_success_transfer_nonempty
  remote_pending_direct_claim_success_transfer_toy
  remote_pending_direct_claim_success_transfer_midpage
  remote_pending_direct_source_boundary_attempt
  remote_pending_direct_source_boundary_gate_hit
  remote_pending_direct_source_boundary_claim_success
  remote_pending_direct_source_alloc_avoided
)

write_meta
printf 'variant\trow\trun\tstatus\tops_s\tpeak_kb\tlog\n' > "${OUTDIR}/runs.tsv"
{
  printf 'variant\trow\trun'
  for key in "${counter_keys[@]}"; do
    printf '\t%s' "$key"
  done
  printf '\n'
} > "${OUTDIR}/counters.tsv"

extract_counters() {
  local variant="$1"
  local row="$2"
  local run="$3"
  local log="$4"
  python3 - "$variant" "$row" "$run" "$log" "${counter_keys[@]}" <<'PY'
import re
import sys

variant, row, run, path, *keys = sys.argv[1:]
text = open(path, errors="replace").read()
values = {}
for key in keys:
    m = re.search(r"(?<![A-Za-z0-9_])" + re.escape(key) + r"=([0-9]+)", text)
    values[key] = m.group(1) if m else "NA"
print("\t".join([variant, row, run] + [values[key] for key in keys]))
PY
}

for variant in "${variants[@]}"; do
  echo "[hz6][owner-inbox-tax] building ${variant}" >&2
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
          HZ6_PRELOAD_STATS=1 \
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
        "$variant" "$name" "$run" "$status" "${ops:-NA}" \
        "${peak:-NA}" "$log" >> "${OUTDIR}/runs.tsv"
      extract_counters "$variant" "$name" "$run" "$log" >> "${OUTDIR}/counters.tsv"
      echo "[hz6][owner-inbox-tax] ${variant} ${name} run=${run} status=${status} ops/s=${ops:-NA} peak_kb=${peak:-NA}" >&2
      [[ "$status" -eq 0 ]] || exit "$status"
    done
  done
done

python3 - "$OUTDIR/runs.tsv" "$OUTDIR/counters.tsv" <<'PY' | tee "${OUTDIR}/summary.tsv"
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

runs_path, counters_path = sys.argv[1:3]
runs = {}
with open(runs_path, newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        if row["status"] != "0" or row["ops_s"] == "NA" or row["peak_kb"] == "NA":
            continue
        key = (row["variant"], row["row"])
        runs.setdefault(key, {"ops": [], "peak": []})
        runs[key]["ops"].append(float(row["ops_s"]))
        runs[key]["peak"].append(float(row["peak_kb"]))

counters = {}
counter_names = []
with open(counters_path, newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    counter_names = [name for name in reader.fieldnames or [] if name not in ("variant", "row", "run")]
    for row in reader:
        key = (row["variant"], row["row"])
        bucket = counters.setdefault(key, {name: [] for name in counter_names})
        for name in counter_names:
            if row[name] != "NA":
                bucket[name].append(float(row[name]))

print(
    "variant\trow\truns\t"
    "ops_min\tops_p25\tops_median\tops_p75\tops_max\t"
    "peak_mib_min\tpeak_mib_p25\tpeak_mib_median\tpeak_mib_p75\tpeak_mib_max\t"
    + "\t".join(f"median_{name}" for name in counter_names)
)
for key in sorted(runs):
    ops = runs[key]["ops"]
    peak_mib = [value / 1024.0 for value in runs[key]["peak"]]
    row = [
        key[0],
        key[1],
        str(len(ops)),
        f"{min(ops):.2f}",
        f"{pct(ops, 1, 4):.2f}",
        f"{statistics.median(ops):.2f}",
        f"{pct(ops, 3, 4):.2f}",
        f"{max(ops):.2f}",
        f"{min(peak_mib):.2f}",
        f"{pct(peak_mib, 1, 4):.2f}",
        f"{statistics.median(peak_mib):.2f}",
        f"{pct(peak_mib, 3, 4):.2f}",
        f"{max(peak_mib):.2f}",
    ]
    for name in counter_names:
        values = counters.get(key, {}).get(name, [])
        row.append(f"{statistics.median(values):.0f}" if values else "NA")
    print("\t".join(row))
PY

echo "[DONE] ${OUTDIR}"
