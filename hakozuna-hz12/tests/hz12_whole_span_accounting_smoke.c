#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hz12.h"
#include "hz12_inbox.h"
#include "hz12_reclaim_gate.h"
#include "hz12_shadow.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_detach.h"
#include "hz12_span_decommit.h"
#include "hz12_span_depot.h"

int main(void) {
  const size_t requested_bytes = 64u;
  H12InboxDeferred deferred;
  H12WholeSpanShadow result;
  H12ReclaimGate gate;
  H12SpanDetachResult detach;
  H12SpanDecommitResult decommit;
  H12SpanDepotTakeResult depot_take;
  H12SpanAccountingRecord reused_accounting;
  void* first;
  void** objects;
  uint8_t class_id;
  size_t slot_bytes;
  uint32_t object_count;
  uint32_t i;
  void* reused;
  uint8_t reused_class;

  if (!h12_shadow_init(1u) || !h12_inbox_init(1u)) return 1;
  h12_span_accounting_reset();
  first = hz12_malloc(requested_bytes);
  if (!first || !hz12_span_classify(first, &class_id)) return 2;
  slot_bytes = hz12_class_slot_size(class_id);
  if (slot_bytes == 0u) return 3;
  object_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  objects = (void**)calloc(object_count, sizeof(void*));
  if (!objects) return 4;
  objects[0] = first;
  h12_shadow_on_alloc(objects[0], 0u);
  h12_span_accounting_on_alloc(objects[0]);
  for (i = 1u; i < object_count; ++i) {
    objects[i] = hz12_malloc(requested_bytes);
    if (!objects[i]) return 6;
    h12_shadow_on_alloc(objects[i], 0u);
    h12_span_accounting_on_alloc(objects[i]);
  }
  h12_inbox_deferred_init(&deferred);
  for (i = 0u; i < object_count; ++i) {
    h12_inbox_defer_free(&deferred, objects[i]);
  }
  h12_inbox_flush(&deferred);
  if (h12_inbox_drain_owner(0u) != object_count) return 7;
  result = h12_span_accounting_scan();
  if (result.tracked_spans != 1u || result.full_capacity_spans != 1u ||
      result.accounting_candidates != 1u ||
      result.tracked_allocations != object_count ||
      result.tracked_releases != object_count ||
      result.tracked_live_objects != 0u || result.release_untracked != 0u ||
      result.release_underflow != 0u) {
    return 8;
  }
  if (!h12_reclaim_gate_query(first, 1, &gate)) return 9;
  if (!gate.accounting_candidate || !gate.accounting_clean ||
      !gate.thread_scope_complete || gate.current_span_refs != 1u ||
      gate.front_cache_objects + gate.returned_sink_objects != object_count ||
      !gate.route_attached || gate.gate_open) {
    return 10;
  }
  printf("[HZ12_WHOLE_SPAN_ACCOUNTING_SMOKE] slots=%u alloc=%llu free=%llu "
         "live=%llu candidates=%u\n",
         object_count, (unsigned long long)result.tracked_allocations,
         (unsigned long long)result.tracked_releases,
         (unsigned long long)result.tracked_live_objects,
         result.accounting_candidates);
  h12_reclaim_gate_dump(stdout, &gate);
  if (!h12_span_detach_quiescent(first, &detach)) return 11;
  if (!detach.completed || !detach.route_detached ||
      detach.front_cache_detached + detach.returned_sink_detached !=
          object_count || !detach.after.gate_open) {
    return 12;
  }
  h12_span_detach_dump(stdout, &detach);
  if (!h12_span_decommit_detached(first, &decommit)) return 13;
  if (!decommit.gate_open || !decommit.attempted || !decommit.succeeded ||
      decommit.bytes != HZ12_SPAN_BYTES) {
    return 14;
  }
  h12_span_decommit_dump(stdout, &decommit);
  if (hz12_malloc_usable_size(first) != 0u) return 15;
  hz12_free(first); /* Detached arena pointers must fail closed. */
  if (hz12_malloc_usable_size(first) != 0u) return 16;
  h12_span_depot_reset();
  if (!h12_span_depot_put(first) || h12_span_depot_count() != 1u) return 17;
  if (h12_span_depot_put(first) || h12_span_depot_count() != 1u) return 18;
  if (!h12_span_depot_take(class_id, &depot_take)) return 19;
  if (h12_span_depot_count() != 0u || !depot_take.recommitted ||
      !depot_take.accounting_reset || !depot_take.route_attached ||
      !depot_take.current_installed || depot_take.span_base != first) {
    return 20;
  }
  h12_span_depot_dump(stdout, &depot_take);
  if (!hz12_span_classify(first, &reused_class) || reused_class != class_id) {
    return 21;
  }
  reused = hz12_malloc(requested_bytes);
  if (reused != first) return 22;
  h12_span_accounting_on_alloc(reused);
  if (!h12_span_accounting_query(reused, &reused_accounting) ||
      reused_accounting.allocated != 1u || reused_accounting.released != 0u ||
      reused_accounting.live != 1u) {
    return 23;
  }
  h12_span_accounting_on_release(reused);
  hz12_free(reused);
  if (!h12_span_accounting_query(reused, &reused_accounting) ||
      reused_accounting.allocated != 1u || reused_accounting.released != 1u ||
      reused_accounting.live != 0u) {
    return 24;
  }
  free(objects);
  return 0;
}
