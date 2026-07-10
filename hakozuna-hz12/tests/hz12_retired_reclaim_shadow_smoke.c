#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hz12.h"
#include "hz12_inbox.h"
#include "hz12_owner_epoch.h"
#include "hz12_owner_registry.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_retired_reclaim_shadow.h"
#include "hz12_shadow.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_detach.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_token_inbox.h"

int main(void) {
  const size_t requested_bytes = 64u;
  H12InboxDeferred deferred;
  H12OwnerEpochParticipant participant;
  H12OwnerToken owner;
  H12OwnerToken replacement;
  H12RetiredReclaimShadow unknown_scope;
  H12RetiredReclaimShadow blocked;
  H12RetiredReclaimShadow reclaimable;
  H12RetiredReclaimShadow stale;
  H12SpanDetachResult detach;
  void** objects;
  void* first;
  uint8_t class_id;
  uint32_t object_count;
  uint32_t i;
  size_t slot_bytes;

  if (!h12_shadow_init(1u) || !h12_inbox_init(1u)) return 1;
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  h12_span_owner_shadow_reset();
  h12_span_accounting_reset();
  if (!h12_owner_register(&owner) ||
      !h12_owner_epoch_register(&participant)) {
    return 2;
  }
  first = hz12_malloc(requested_bytes);
  if (!first || !hz12_span_classify(first, &class_id) ||
      !h12_span_owner_shadow_assign(first, owner)) {
    return 3;
  }
  slot_bytes = hz12_class_slot_size(class_id);
  object_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  objects = (void**)calloc(object_count, sizeof(void*));
  if (!objects) return 4;
  objects[0] = first;
  h12_shadow_on_alloc(first, 0u);
  h12_span_accounting_on_alloc(first);
  for (i = 1u; i < object_count; ++i) {
    objects[i] = hz12_malloc(requested_bytes);
    if (!objects[i]) return 5;
    h12_shadow_on_alloc(objects[i], 0u);
    h12_span_accounting_on_alloc(objects[i]);
  }
  h12_inbox_deferred_init(&deferred);
  for (i = 0u; i < object_count; ++i) {
    h12_inbox_defer_free(&deferred, objects[i]);
  }
  h12_inbox_flush(&deferred);
  if (h12_inbox_drain_owner(0u) != object_count) return 6;
  if (!h12_owner_begin_retire(owner) ||
      !h12_owner_epoch_begin_retire(owner) ||
      !h12_owner_epoch_checkpoint(participant) ||
      !h12_owner_epoch_unregister(participant)) {
    return 7;
  }
  unknown_scope = h12_retired_reclaim_shadow_scan(owner, 0);
  if (!unknown_scope.owner_gate_open ||
      unknown_scope.blocked_foreign_cache_scope != 1u ||
      unknown_scope.detach_ready_bytes != 0u ||
      unknown_scope.reclaimable_bytes != 0u) {
    return 8;
  }
  blocked = h12_retired_reclaim_shadow_scan(owner, 1);
  if (blocked.owner_spans != 1u || blocked.accounting_candidates != 1u ||
      blocked.blocked_current != 1u || blocked.blocked_front_cache != 1u ||
      blocked.blocked_returned_sink != 1u || blocked.blocked_route != 1u ||
      blocked.detach_ready_bytes != 0u || blocked.reclaimable_bytes != 0u) {
    return 9;
  }
  if (!h12_span_detach_quiescent(first, &detach) || !detach.completed) return 10;
  reclaimable = h12_retired_reclaim_shadow_scan(owner, 1);
  if (reclaimable.owner_spans != 1u || reclaimable.reclaimable_spans != 1u ||
      reclaimable.reclaimable_bytes != HZ12_SPAN_BYTES ||
      reclaimable.detach_ready_bytes != 0u) {
    return 11;
  }
  if (!h12_owner_mark_dead(owner) || !h12_owner_register(&replacement) ||
      replacement.slot != owner.slot ||
      replacement.generation == owner.generation) {
    return 12;
  }
  stale = h12_retired_reclaim_shadow_scan(replacement, 1);
  if (stale.owner_spans != 0u || stale.stale_generation_spans != 1u ||
      stale.reclaimable_bytes != 0u) {
    return 13;
  }
  printf("[HZ12_RETIRED_RECLAIM_SHADOW_SMOKE] owner_spans=1 "
         "foreign_scope_blocked=1 blocked_before_detach=1 "
         "reclaimable_bytes=%llu stale_generation=1\n",
         (unsigned long long)reclaimable.reclaimable_bytes);
  h12_retired_reclaim_shadow_dump(stdout, &unknown_scope);
  h12_retired_reclaim_shadow_dump(stdout, &blocked);
  h12_retired_reclaim_shadow_dump(stdout, &reclaimable);
  h12_retired_reclaim_shadow_dump(stdout, &stale);
  free(objects);
  return 0;
}
