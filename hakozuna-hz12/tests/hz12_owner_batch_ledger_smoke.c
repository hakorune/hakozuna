#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hz12.h"
#include "hz12_owner_batch_ledger.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_thread_cache.h"

int main(void) {
  const size_t requested_bytes = 64u;
  const H12OwnerToken owner = {3u, 7u};
  H12OwnerBatchLedgerAgreement agreement;
  H12OwnerBatchLedgerRecord ledger;
  void** objects;
  void* first;
  uint8_t class_id;
  size_t slot_bytes;
  uint32_t object_count;
  uint32_t i;

  h12_span_accounting_reset();
  h12_span_owner_shadow_reset();
  h12_owner_batch_ledger_reset();
  first = hz12_malloc(requested_bytes);
  if (!first || !hz12_span_classify(first, &class_id)) return 1;
  slot_bytes = hz12_class_slot_size(class_id);
  if (slot_bytes == 0u) return 2;
  object_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  objects = (void**)calloc(object_count, sizeof(void*));
  if (!objects) return 3;
  objects[0] = first;
  for (i = 1u; i < object_count; ++i) {
    objects[i] = hz12_malloc(requested_bytes);
    if (!objects[i]) return 4;
  }
  if (!h12_span_owner_shadow_assign(first, owner)) return 5;
  for (i = 0u; i < object_count; i += 32u) {
    uint32_t batch = object_count - i;
    if (batch > 32u) batch = 32u;
    for (uint32_t j = 0u; j < batch; ++j) {
      h12_span_accounting_on_alloc(objects[i + j]);
    }
    if (h12_owner_batch_ledger_acquire_range(owner, objects + i, batch) !=
        batch) {
      return 6;
    }
  }
  if (h12_owner_batch_ledger_owner_drain_range(owner, objects,
                                                object_count / 2u) !=
      object_count / 2u) {
    return 7;
  }
  if (!h12_owner_batch_ledger_query(first, &ledger) ||
      ledger.outstanding_slots != object_count || ledger.ledger_candidate) {
    return 8;
  }
  for (i = 0u; i < object_count; ++i) {
    h12_span_accounting_on_release(objects[i]);
    hz12_free(objects[i]);
  }
  hz12_thread_cache_reclaim_checkpoint();
  if (h12_owner_batch_ledger_return_range(owner, objects, object_count) !=
      object_count) {
    return 9;
  }
  if (!h12_owner_batch_ledger_query(first, &ledger) ||
      !ledger.full_span || !ledger.ledger_candidate ||
      ledger.outstanding_slots != 0u || ledger.rejected_transitions != 0u) {
    return 10;
  }
  agreement = h12_owner_batch_ledger_compare_atomic();
  if (agreement.compared_spans != 1u || agreement.matching_spans != 1u ||
      agreement.mismatched_spans != 0u ||
      agreement.ledger_candidates != 1u ||
      agreement.atomic_candidates != 1u) {
    return 11;
  }
  printf("[HZ12_OWNER_BATCH_LEDGER_SMOKE] slots=%u drained=%u "
         "outstanding=%u match=%u mismatch=%u\n",
         object_count, ledger.owner_drain_objects, ledger.outstanding_slots,
         agreement.matching_spans, agreement.mismatched_spans);
  free(objects);
  return 0;
}
