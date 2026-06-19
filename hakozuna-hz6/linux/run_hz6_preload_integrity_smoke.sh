#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

BENCH="${HZ6_BENCH_REMOTE_MT:-/mnt/workdisk/public_share/hakmem/hakozuna/out/bench_random_mixed_mt_remote_malloc}"
DSO="$REPO_ROOT/hakozuna-hz6/out/linux/hz6_preload_diag/libhakozuna_hz6_preload.so"
RUN_TIMEOUT="${HZ6_PRELOAD_INTEGRITY_TIMEOUT:-30}"

if [[ ! -x "$BENCH" ]]; then
  echo "[hz6][integrity] missing bench: $BENCH" >&2
  exit 2
fi

"$SCRIPT_DIR/build_hz6_preload_diag.sh" >/dev/null

output="$(
  timeout "$RUN_TIMEOUT" env -i \
    PATH="${PATH:-/usr/bin:/bin}" \
    HOME="${HOME:-/tmp}" \
    HZ6_PRELOAD_STATS=1 \
    LD_PRELOAD="$DSO" \
    "$BENCH" 16 10000 100 16 32768 50 65536 2>&1
)"

extract_counter() {
  local name="$1"
  local match
  match="$(printf '%s\n' "$output" | grep -o "${name}=[0-9][0-9]*" | tail -1 || true)"
  if [[ -z "$match" ]]; then
    echo "[hz6][integrity] missing counter: $name" >&2
    printf '%s\n' "$output" >&2
    exit 3
  fi
  printf '%s' "${match#*=}"
}

require_zero() {
  local name="$1"
  local value
  value="$(extract_counter "$name")"
  if [[ "$value" != "0" ]]; then
    echo "[hz6][integrity] expected ${name}=0, got $value" >&2
    printf '%s\n' "$output" >&2
    exit 4
  fi
}

require_zero "free_route_retry_abort"
require_zero "free_route_integrity_abort"
require_zero "free_route_real_free_unproven"
require_zero "free_resolve_result_retry"
require_zero "free_resolve_result_unresolved_integrity"
require_zero "route_rehome_fail"
require_zero "route_rehome_expected_owner_mismatch"
require_zero "route_rehome_destination_route_fail"
require_zero "route_rehome_directory_transfer_fail"
require_zero "route_rehome_rollback_fail"
require_zero "transfer_reserve_full_after_state_mutation"
require_zero "remote_free_backpressure_requeue_fail"
require_zero "remote_free_status_integrity_failure"
require_zero "remote_pending_enqueue_full"
require_zero "remote_pending_duplicate_claim"
require_zero "remote_pending_publish_fail"
require_zero "remote_pending_generation_mismatch"
require_zero "remote_pending_owner_mismatch"
require_zero "remote_pending_route_mismatch"
require_zero "remote_pending_state_mismatch"
require_zero "remote_free_pending_publish_fail"
require_zero "remote_pending_owner_inbox_storage_ineligible"
require_zero "remote_pending_owner_inbox_descriptor_mismatch"
require_zero "remote_pending_owner_inbox_owner_dead"
require_zero "remote_pending_owner_inbox_owner_mismatch"
require_zero "remote_pending_owner_inbox_enqueue_fail"
require_zero "remote_pending_external_ticket_full"
require_zero "remote_pending_external_ticket_duplicate"
require_zero "remote_pending_external_ticket_locked_revalidate_fail"
require_zero "remote_pending_external_ticket_route_mismatch"
require_zero "remote_pending_external_ticket_owner_mismatch"
require_zero "remote_pending_external_ticket_state_mismatch"
require_zero "remote_pending_external_ticket_storage_mismatch"
require_zero "remote_pending_external_ticket_integrity_abort"
require_zero "remote_pending_inline_accounting_mismatch"
require_zero "remote_pending_external_accounting_mismatch"
require_zero "remote_pending_total_state_count_mismatch"
require_zero "remote_pending_external_free_list_corruption"
require_zero "remote_pending_external_list_cycle"
require_zero "remote_pending_external_ticket_multiple_list_membership"
require_zero "remote_pending_claimed_current_at_quiescence"
require_zero "remote_pending_external_claimed_at_quiescence"
require_zero "remote_free_returned_uncommitted"
require_zero "remote_free_returned_stale"
require_zero "remote_free_returned_integrity_failure"
require_zero "route_compact_remote_path_attempt"
require_zero "ring_full_fallback"
require_zero "overflow_sent"
require_zero "overflow_received"

printf '%s\n' "$output" | grep -E \
  'BENCH_MT_REMOTE|ops/s|HZ6_PRELOAD_HOOK_DETAIL|HZ6_PRELOAD_RESOLVE_DETAIL|HZ6_PRELOAD_ROUTE_DETAIL|HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL|EFFECTIVE_REMOTE|OVERFLOW_PENDING'
echo "[hz6][integrity] ok"
