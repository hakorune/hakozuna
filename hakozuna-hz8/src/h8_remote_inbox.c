#include "h8_internal.h"

static void h8_pending_queue_push(H8OwnerRecord* owner, H8Span* span) {
  H8Span* head = atomic_load_explicit(&owner->pending_head, memory_order_relaxed);
  do {
    span->next_pending = head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner->pending_head, &head, span, memory_order_release,
      memory_order_relaxed));
  atomic_fetch_add_explicit(&owner->pending_span_count, 1, memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.pending_enqueue_count, 1, memory_order_relaxed);
}

static void h8_pending_queue_push_list(H8OwnerRecord* owner, H8Span* list) {
  if (!list) {
    return;
  }
  size_t count = 0;
  H8Span* tail = list;
  while (tail) {
    ++count;
    if (!tail->next_pending) {
      break;
    }
    tail = tail->next_pending;
  }
  H8Span* head = atomic_load_explicit(&owner->pending_head, memory_order_relaxed);
  do {
    tail->next_pending = head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner->pending_head, &head, list, memory_order_release,
      memory_order_relaxed));
  atomic_fetch_add_explicit(&owner->pending_span_count, count, memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.pending_enqueue_count, count, memory_order_relaxed);
}

void h8_span_notify(H8OwnerRecord* owner, H8Span* span) {
  uint8_t expected = H8_Q_IDLE;
  if (atomic_compare_exchange_strong_explicit(&span->qstate, &expected, H8_Q_QUEUED,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    h8_pending_queue_push(owner, span);
  }
}

static H8PublishResult h8_remote_free_publish_locked(H8Span* span, H8OwnerRecord* owner,
                                                     size_t slot, void* ptr) {
  (void)ptr;
  if (span->owner_slot != owner->slot ||
      span->owner_generation != owner->generation) {
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8SpanState state = h8_span_state_load(span);
  if (state == H8_SPAN_ORPHAN_QUIESCING ||
      state == H8_SPAN_ORPHAN_READY ||
      state == H8_SPAN_ADOPTING) {
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (state != H8_SPAN_OWNED_ACTIVE) {
    return H8_PUBLISH_INVALID;
  }
  if (h8_bitmap_test_and_set((_Atomic uint64_t*)span->pending_bits, slot)) {
    return H8_PUBLISH_DOUBLE_FREE;
  }
  if (!h8_bitmap_test((_Atomic uint64_t*)span->live_bits, slot)) {
    h8_bitmap_clear((_Atomic uint64_t*)span->pending_bits, slot);
    return H8_PUBLISH_INVALID;
  }
  atomic_fetch_add_explicit(&h8g.remote_publish_count, 1, memory_order_relaxed);
  h8_span_notify(owner, span);
  return H8_PUBLISH_OK;
}

H8PublishResult h8_remote_free_publish(void* ptr) {
  size_t slot = 0;
  H8Span* span = h8_span_from_ptr_checked(ptr, &slot);
  if (!span) {
    return H8_PUBLISH_MISS;
  }
  H8OwnerRecord* owner = h8_owner_by_slot(span->owner_slot);
  if (!owner || owner->generation != span->owner_generation) {
    atomic_fetch_add_explicit(&h8g.owner_transition_count, 1, memory_order_relaxed);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (!h8_span_publish_enter(span)) {
    atomic_fetch_add_explicit(&h8g.owner_transition_count, 1, memory_order_relaxed);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (!h8_owner_publish_enter(owner, span->owner_generation)) {
    h8_span_publish_exit(span);
    atomic_fetch_add_explicit(&h8g.owner_transition_count, 1, memory_order_relaxed);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot, ptr);
  if (res == H8_PUBLISH_OK && owner->placement == H8_OWNER_PLACEMENT_ORPHAN) {
    h8_collect_owner_pending(owner);
  }
  h8_span_publish_exit(span);
  h8_owner_publish_exit(owner);
  return res;
}

void h8_span_collect_remote(H8OwnerRecord* owner, H8Span* span) {
  uint8_t expected = H8_Q_QUEUED;
  if (!atomic_compare_exchange_strong_explicit(&span->qstate, &expected,
                                               H8_Q_DRAINING, memory_order_acq_rel,
                                               memory_order_acquire)) {
    return;
  }

  for (uint32_t slot = 0; slot < span->slot_count; ++slot) {
    if (!h8_bitmap_test((_Atomic uint64_t*)span->pending_bits, slot)) {
      continue;
    }
    if (!h8_bitmap_test((_Atomic uint64_t*)span->live_bits, slot)) {
      atomic_fetch_add_explicit(&h8g.invalid_count, 1, memory_order_relaxed);
      h8_bitmap_clear((_Atomic uint64_t*)span->pending_bits, slot);
      continue;
    }
    h8_bitmap_clear((_Atomic uint64_t*)span->live_bits, slot);
    h8_bitmap_clear((_Atomic uint64_t*)span->pending_bits, slot);
    span->next_free[slot] = atomic_load_explicit(&span->local_free_head,
                                                 memory_order_relaxed);
    atomic_store_explicit(&span->local_free_head, (uint32_t)slot,
                          memory_order_release);
    atomic_fetch_sub_explicit(&span->used_count, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.remote_collect_count, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.pending_dequeue_count, 1, memory_order_relaxed);
  }

  atomic_store_explicit(&span->qstate, H8_Q_IDLE, memory_order_release);
  if (h8_span_state_load(span) == H8_SPAN_ORPHAN_QUIESCING &&
      atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
      h8_bitmap_popcount((_Atomic uint64_t*)span->pending_bits,
                         h8_word_count_for_slots(span->slot_count)) == 0) {
    h8_span_mark_orphan_ready(span);
  }
  if (h8_bitmap_popcount((_Atomic uint64_t*)span->pending_bits,
                         h8_word_count_for_slots(span->slot_count)) != 0) {
    h8_span_notify(owner, span);
  }
}

void h8_collect_owner_pending_budget(H8OwnerRecord* owner, size_t budget) {
  H8Span* list = atomic_exchange_explicit(&owner->pending_head, NULL,
                                          memory_order_acq_rel);
  size_t collected = 0;
  H8Span* remainder = NULL;
  H8Span* remainder_tail = NULL;
  while (list) {
    H8Span* span = list;
    list = span->next_pending;
    span->next_pending = NULL;
    if (collected < budget) {
      h8_span_collect_remote(owner, span);
      atomic_fetch_sub_explicit(&owner->pending_span_count, 1, memory_order_relaxed);
      ++collected;
      continue;
    }
    if (!remainder) {
      remainder = span;
      remainder_tail = span;
    } else {
      remainder_tail->next_pending = span;
      remainder_tail = span;
    }
  }
  if (remainder) {
    h8_pending_queue_push_list(owner, remainder);
  }
}

void h8_collect_owner_pending(H8OwnerRecord* owner) {
  h8_collect_owner_pending_budget(owner, SIZE_MAX);
}
