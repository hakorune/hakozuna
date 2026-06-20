#include "h8_internal.h"

#include <string.h>

void h8_pressure_owner_collect(H8OwnerRecord* owner) {
  h8_collect_owner_pending(owner);
}

static H8Span* h8_find_orphan_span(H8OwnerRecord* owner, uint32_t class_id) {
  pthread_mutex_lock(&owner->owned_lock);
  for (H8Span* span = owner->orphan_head; span; span = span->next_orphan) {
    if (span->class_id == class_id &&
        atomic_load_explicit(&span->used_count, memory_order_acquire) < span->slot_count) {
      pthread_mutex_unlock(&owner->owned_lock);
      return span;
    }
  }
  pthread_mutex_unlock(&owner->owned_lock);
  return NULL;
}

static H8Span* h8_reclaim_orphan_span(H8OwnerRecord* owner, uint32_t class_id) {
  H8Span* span = h8_find_orphan_span(h8_orphan_owner(), class_id);
  if (!span) {
    return NULL;
  }
  if (!h8_owner_acquire_span_as_orphan(owner, span)) {
    return NULL;
  }
  h8_owner_remove_orphan_span(h8_orphan_owner(), span);
  h8_owner_add_owned_span(owner, span);
  return span;
}

H8Span* h8_span_commit_for_class(H8OwnerRecord* owner, uint32_t class_id);

H8Span* h8_pressure_refill(H8OwnerRecord* owner, uint32_t class_id) {
  H8Span* span = h8_reclaim_orphan_span(owner, class_id);
  if (span) {
    return span;
  }
  return h8_span_commit_for_class(owner, class_id);
}
