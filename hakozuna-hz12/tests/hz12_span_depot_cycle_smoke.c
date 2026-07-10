#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hz12.h"
#include "hz12_inbox.h"
#include "hz12_shadow.h"
#include "hz12_size_class.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_decommit.h"
#include "hz12_span_depot.h"
#include "hz12_span_detach.h"

int main(void) {
  const uint32_t cycles = 8u;
  const size_t requested_bytes = 64u;
  const uint8_t class_id = hz12_size_class(requested_bytes);
  const size_t slot_bytes = hz12_class_slot_size(class_id);
  const uint32_t slot_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  H12InboxDeferred deferred;
  H12SpanDepotStats stats;
  void** objects;
  void* expected_base = NULL;
  uint32_t cycle;

  if (!h12_shadow_init(1u) || !h12_inbox_init(1u)) return 1;
  h12_span_accounting_reset();
  h12_span_depot_reset();
  objects = (void**)calloc(slot_count, sizeof(void*));
  if (!objects) return 2;

  for (cycle = 0u; cycle < cycles; ++cycle) {
    H12SpanAccountingRecord accounting;
    H12SpanDetachResult detach;
    H12SpanDecommitResult decommit;
    H12SpanDepotTakeResult take;
    void* span_base;
    uint32_t i;

    for (i = 0u; i < slot_count; ++i) {
      objects[i] = hz12_malloc(requested_bytes);
      if (!objects[i]) return 3;
      if (i == 0u) {
        uintptr_t value = (uintptr_t)objects[i];
        span_base = (void*)(value & ~(uintptr_t)(HZ12_SPAN_BYTES - 1u));
        if (expected_base && span_base != expected_base) return 4;
      } else if ((uintptr_t)objects[i] < (uintptr_t)span_base ||
                 (uintptr_t)objects[i] >=
                     (uintptr_t)span_base + HZ12_SPAN_BYTES) {
        return 5;
      }
      h12_shadow_on_alloc(objects[i], 0u);
      h12_span_accounting_on_alloc(objects[i]);
    }

    h12_inbox_deferred_init(&deferred);
    for (i = 0u; i < slot_count; ++i) {
      h12_inbox_defer_free(&deferred, objects[i]);
    }
    h12_inbox_flush(&deferred);
    if (h12_inbox_drain_owner(0u) != slot_count) return 6;
    if (!h12_span_accounting_query(span_base, &accounting) ||
        !accounting.accounting_candidate || accounting.live != 0u) {
      return 7;
    }
    if (!h12_span_detach_quiescent(span_base, &detach) ||
        !detach.completed) {
      return 8;
    }
    if (!h12_span_decommit_detached(span_base, &decommit) ||
        !decommit.succeeded) {
      return 9;
    }
    if (!h12_span_depot_put(span_base) ||
        !h12_span_depot_take(class_id, &take)) {
      return 10;
    }
    if (take.span_base != span_base || !take.recommitted ||
        !take.accounting_reset || !take.route_attached ||
        !take.current_installed || h12_span_depot_count() != 0u) {
      return 11;
    }
    expected_base = span_base;
  }

  h12_span_depot_stats(&stats);
  if (stats.put_attempt != cycles || stats.put_success != cycles ||
      stats.take_attempt != cycles || stats.take_hit != cycles ||
      stats.recommit_success != cycles || stats.depot_current != 0u ||
      stats.depot_max != 1u || stats.put_duplicate != 0u ||
      stats.put_full != 0u || stats.put_reject_state != 0u ||
      stats.take_miss != 0u || stats.recommit_fail != 0u ||
      stats.route_fail != 0u || stats.current_install_fail != 0u ||
      stats.rollback != 0u) {
    return 12;
  }
  printf("[HZ12_SPAN_DEPOT_CYCLE] cycles=%u slots=%u put=%llu take=%llu "
         "recommit=%llu current=%u max=%u failures=0\n",
         cycles, slot_count, (unsigned long long)stats.put_success,
         (unsigned long long)stats.take_hit,
         (unsigned long long)stats.recommit_success, stats.depot_current,
         stats.depot_max);
  free(objects);
  return 0;
}
