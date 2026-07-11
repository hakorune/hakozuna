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
#include "hz12_span_depot_core.h"
#include "hz12_span_detach.h"

int main(void) {
  const uint32_t span_count = HZ12_SPAN_DEPOT_CAP + 1u;
  const size_t requested_bytes = 64u;
  const uint8_t class_id = hz12_size_class(requested_bytes);
  const uint32_t slots_per_span = (uint32_t)(
      HZ12_SPAN_BYTES / hz12_class_slot_size(class_id));
  const uint32_t object_count = span_count * slots_per_span;
  H12InboxDeferred deferred;
  H12SpanDepotStats stats;
  H12WholeSpanShadow accounting;
  void** objects;
  void** span_bases;
  uint32_t i;

  if (!h12_shadow_init(1u) || !h12_inbox_init(1u)) return 1;
  h12_span_accounting_reset();
  h12_span_depot_reset();
  if (h12_span_depot_core_reserve(HZ12_SPAN_DEPOT_CAP) !=
          HZ12_SPAN_DEPOT_CAP ||
      h12_span_depot_core_available() != 0u ||
      h12_span_depot_core_reserve(1u) != 0u) {
    return 9;
  }
  h12_span_depot_core_release_reservation(HZ12_SPAN_DEPOT_CAP);
  if (h12_span_depot_core_available() != HZ12_SPAN_DEPOT_CAP) return 10;
  objects = (void**)calloc(object_count, sizeof(void*));
  span_bases = (void**)calloc(span_count, sizeof(void*));
  if (!objects || !span_bases) return 2;

  for (i = 0u; i < object_count; ++i) {
    objects[i] = hz12_malloc(requested_bytes);
    if (!objects[i]) return 3;
    h12_shadow_on_alloc(objects[i], 0u);
    h12_span_accounting_on_alloc(objects[i]);
    if ((i % slots_per_span) == 0u) {
      span_bases[i / slots_per_span] = objects[i];
    }
  }
  h12_inbox_deferred_init(&deferred);
  for (i = 0u; i < object_count; ++i) {
    h12_inbox_defer_free(&deferred, objects[i]);
  }
  h12_inbox_flush(&deferred);
  (void)h12_inbox_drain_owner(0u);
  accounting = h12_span_accounting_scan();
  if (accounting.tracked_allocations != object_count ||
      accounting.tracked_releases != object_count ||
      accounting.tracked_live_objects != 0u ||
      accounting.release_untracked != 0u ||
      accounting.release_underflow != 0u) {
    return 4;
  }

  for (i = 0u; i < span_count; ++i) {
    H12SpanDetachResult detach;
    H12SpanDecommitResult decommit;
    if (!h12_span_detach_quiescent(span_bases[i], &detach) ||
        !h12_span_decommit_detached(span_bases[i], &decommit)) {
      return 5;
    }
    if (i < HZ12_SPAN_DEPOT_CAP) {
      if (!h12_span_depot_put(span_bases[i])) return 6;
    } else if (h12_span_depot_put(span_bases[i])) {
      return 7;
    }
  }

  h12_span_depot_stats(&stats);
  if (stats.put_attempt != span_count ||
      stats.put_success != HZ12_SPAN_DEPOT_CAP || stats.put_full != 1u ||
      stats.put_duplicate != 0u || stats.put_reject_state != 0u ||
      stats.depot_current != HZ12_SPAN_DEPOT_CAP ||
      stats.depot_max != HZ12_SPAN_DEPOT_CAP) {
    return 8;
  }
  if (h12_span_depot_core_available() != 0u ||
      h12_span_depot_core_reserve(1u) != 0u) {
    return 11;
  }
  printf("[HZ12_SPAN_DEPOT_CAP] capacity=%u attempts=%llu success=%llu "
         "full=%llu current=%u max=%u\n",
         HZ12_SPAN_DEPOT_CAP, (unsigned long long)stats.put_attempt,
         (unsigned long long)stats.put_success,
         (unsigned long long)stats.put_full, stats.depot_current,
         stats.depot_max);
  free(span_bases);
  free(objects);
  return 0;
}
