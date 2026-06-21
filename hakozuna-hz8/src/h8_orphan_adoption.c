#include "h8_internal.h"

static void h8_owner_lock_pair(H8OwnerRecord* a, H8OwnerRecord* b) {
  if (a == b) {
    pthread_mutex_lock(&a->owned_lock);
    return;
  }
  if (a->slot < b->slot) {
    pthread_mutex_lock(&a->owned_lock);
    pthread_mutex_lock(&b->owned_lock);
  } else {
    pthread_mutex_lock(&b->owned_lock);
    pthread_mutex_lock(&a->owned_lock);
  }
}

static void h8_owner_unlock_pair(H8OwnerRecord* a, H8OwnerRecord* b) {
  if (a == b) {
    pthread_mutex_unlock(&a->owned_lock);
    return;
  }
  pthread_mutex_unlock(&a->owned_lock);
  pthread_mutex_unlock(&b->owned_lock);
}

static void h8_owner_add_owned_span_locked(H8OwnerRecord* owner, H8Span* span) {
  span->next_owned = owner->owned_head;
  owner->owned_head = span;
}

static void h8_owner_remove_orphan_span_locked(H8OwnerRecord* owner, H8Span* span) {
  H8Span** cur = &owner->orphan_head;
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_orphan;
      span->next_orphan = NULL;
      return;
    }
    cur = &(*cur)->next_orphan;
  }
}

static bool h8_span_quiescent_for_adoption(const H8Span* span) {
  return atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
         atomic_load_explicit(&span->qstate, memory_order_acquire) == H8_Q_IDLE &&
         h8_bitmap_popcount((const _Atomic uint64_t*)span->pending_bits,
                            h8_word_count_for_slots(span->slot_count)) == 0 &&
         atomic_load_explicit(&span->publish_closed, memory_order_acquire) != 0;
}

H8Span* h8_orphan_adopt_span(H8OwnerRecord* adopter, uint32_t class_id) {
  H8OwnerRecord* orphan = h8_orphan_owner();
  if (!adopter || adopter == orphan) {
    return NULL;
  }
  if (!h8_owner_lifecycle_enter(adopter, (uint16_t)adopter->generation)) {
    return NULL;
  }

  for (;;) {
    H8Span* candidate = NULL;
    H8SpanState candidate_state = H8_SPAN_RETIRED;

    pthread_mutex_lock(&orphan->owned_lock);
    for (H8Span* span = orphan->orphan_head; span; span = span->next_orphan) {
      if (span->class_id != class_id) {
        continue;
      }
      if (atomic_load_explicit(&span->used_count, memory_order_acquire) == 0) {
        continue;
      }
      candidate_state = h8_span_state_load(span);
      if (candidate_state == H8_SPAN_OWNED_ACTIVE) {
        h8_span_mark_orphan_quiescing(span);
        candidate = span;
        break;
      }
      if (candidate_state == H8_SPAN_ORPHAN_READY) {
        candidate = span;
        break;
      }
      if (candidate_state == H8_SPAN_ORPHAN_QUIESCING) {
        continue;
      }
      if (candidate_state == H8_SPAN_ADOPTING) {
        continue;
      }
      candidate = span;
      break;
    }
    pthread_mutex_unlock(&orphan->owned_lock);

    if (!candidate) {
      h8_owner_lifecycle_exit(adopter);
      return NULL;
    }

    h8_span_wait_publishers_zero(candidate);
    h8_collect_owner_pending(orphan);

    if (atomic_load_explicit(&candidate->used_count, memory_order_acquire) == 0) {
      h8_span_mark_orphan_ready(candidate);
      atomic_store_explicit(&candidate->publish_closed, 0, memory_order_release);
      h8_owner_lifecycle_exit(adopter);
      return NULL;
    }

    if (candidate_state == H8_SPAN_OWNED_ACTIVE) {
      if (!h8_span_quiescent_for_adoption(candidate)) {
        continue;
      }
      h8_span_mark_orphan_ready(candidate);
    } else if (!h8_span_quiescent_for_adoption(candidate)) {
      continue;
    }

    h8_owner_lock_pair(orphan, adopter);
    if (candidate->owner_slot != orphan->slot ||
        candidate->owner_generation != orphan->generation ||
        h8_span_state_load(candidate) != H8_SPAN_ORPHAN_READY ||
        !h8_span_quiescent_for_adoption(candidate)) {
      h8_owner_unlock_pair(orphan, adopter);
      continue;
    }

    h8_owner_remove_orphan_span_locked(orphan, candidate);
    candidate->owner_slot = adopter->slot;
    candidate->owner_generation = adopter->generation;
    h8_span_state_store(candidate, H8_SPAN_ADOPTING, memory_order_release);
    atomic_fetch_add_explicit(&candidate->span_epoch, 1, memory_order_acq_rel);
    h8_owner_add_owned_span_locked(adopter, candidate);
    atomic_store_explicit(&candidate->publish_closed, 0, memory_order_release);
    atomic_store_explicit(&candidate->publish_refs, 0, memory_order_release);
    h8_span_state_store(candidate, H8_SPAN_OWNED_ACTIVE, memory_order_release);
    if (atomic_load_explicit(&h8g.orphan_span_count, memory_order_relaxed) > 0) {
      atomic_fetch_sub_explicit(&h8g.orphan_span_count, 1, memory_order_relaxed);
    }
    atomic_fetch_add_explicit(&h8g.orphan_handoff_count, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.handoff_success_count, 1, memory_order_relaxed);
    h8_owner_unlock_pair(orphan, adopter);
    h8_owner_lifecycle_exit(adopter);
    return candidate;
  }
}
