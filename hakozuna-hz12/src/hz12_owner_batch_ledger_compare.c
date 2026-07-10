#include "hz12_owner_batch_ledger.h"

#include "hz12_span.h"
#include "hz12_span_accounting.h"

#include <string.h>

static H12OwnerBatchLedgerAgreement h12_ledger_compare_atomic(
    const H12OwnerToken* owner) {
  H12OwnerBatchLedgerAgreement result;
  memset(&result, 0, sizeof(result));
  for (uint32_t span_id = 0u; span_id < HZ12_SPAN_COUNT; ++span_id) {
    H12OwnerBatchLedgerRecord ledger;
    H12SpanAccountingRecord atomic_record;
    void* base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!h12_owner_batch_ledger_query(base, &ledger)) continue;
    if (owner && (ledger.owner.slot != owner->slot ||
                  ledger.owner.generation != owner->generation)) {
      continue;
    }
    if (!h12_span_accounting_query(base, &atomic_record)) {
      result.mismatched_spans += 1u;
      continue;
    }
    result.compared_spans += 1u;
    result.ledger_candidates += ledger.ledger_candidate != 0u;
    result.atomic_candidates += atomic_record.accounting_candidate != 0u;
    if (ledger.ledger_candidate == atomic_record.accounting_candidate &&
        ledger.full_span == (atomic_record.carved_slots ==
                             atomic_record.slot_capacity) &&
        ledger.rejected_transitions == 0u) {
      result.matching_spans += 1u;
    } else {
      result.mismatched_spans += 1u;
    }
  }
  return result;
}

H12OwnerBatchLedgerAgreement h12_owner_batch_ledger_compare_atomic(void) {
  return h12_ledger_compare_atomic(NULL);
}

H12OwnerBatchLedgerAgreement h12_owner_batch_ledger_compare_owner_atomic(
    H12OwnerToken owner) {
  return h12_ledger_compare_atomic(&owner);
}
