#include "h8_internal.h"

static void h8_pending_queue_push(H8OwnerRecord* owner, H8Span* span) {
  H8Span* head = atomic_load_explicit(&owner->pending_head, memory_order_relaxed);
  do {
    span->next_pending = head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner->pending_head, &head, span, memory_order_release,
      memory_order_relaxed));
  atomic_fetch_add_explicit(&owner->pending_span_count, 1, memory_order_relaxed);
  H8_DEBUG_INC(pending_enqueue_count);
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
  H8_DEBUG_ADD(pending_enqueue_count, count);
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
                                                     size_t slot) {
  if (h8_bitmap_test_and_set((_Atomic uint64_t*)span->pending_bits, slot)) {
    return H8_PUBLISH_DOUBLE_FREE;
  }
  if (!h8_bitmap_test((_Atomic uint64_t*)span->live_bits, slot)) {
    h8_bitmap_clear((_Atomic uint64_t*)span->pending_bits, slot);
    return H8_PUBLISH_INVALID;
  }
  H8_DEBUG_INC(remote_publish_count);
  atomic_fetch_add_explicit(&span->pending_count, 1, memory_order_relaxed);
  h8_span_notify(owner, span);
  return H8_PUBLISH_OK;
}

H8PublishResult h8_remote_free_publish(void* ptr) {
  size_t slot = 0;
  H8Span* span = h8_span_from_ptr_checked(ptr, &slot);
  if (!span) {
    return H8_PUBLISH_MISS;
  }
  H8OwnerWord ow = h8_span_owner_word_load(span);
  H8OwnerRecord* owner = h8_owner_by_slot(ow.slot);
  if (!owner || owner->generation != ow.generation) {
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (!h8_span_publish_enter(span)) {
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (!h8_owner_publish_enter(owner, ow.generation)) {
    h8_span_publish_exit(span);
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot);
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
      H8_DEBUG_INC(invalid_count);
      h8_bitmap_clear((_Atomic uint64_t*)span->pending_bits, slot);
      atomic_fetch_sub_explicit(&span->pending_count, 1, memory_order_relaxed);
      continue;
    }
    h8_bitmap_clear((_Atomic uint64_t*)span->live_bits, slot);
    h8_bitmap_clear((_Atomic uint64_t*)span->pending_bits, slot);
    atomic_fetch_sub_explicit(&span->pending_count, 1, memory_order_relaxed);
    span->next_free[slot] = atomic_load_explicit(&span->local_free_head,
                                                 memory_order_relaxed);
    atomic_store_explicit(&span->local_free_head, (uint32_t)slot,
                          memory_order_release);
    atomic_fetch_sub_explicit(&span->used_count, 1, memory_order_relaxed);
    H8_DEBUG_INC(remote_collect_count);
    H8_DEBUG_INC(pending_dequeue_count);
  }

  atomic_store_explicit(&span->qstate, H8_Q_IDLE, memory_order_release);
  if ((H8SpanState)h8_span_owner_word_load(span).state == H8_SPAN_ORPHAN_QUIESCING &&
      atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
      atomic_load_explicit(&span->pending_count, memory_order_acquire) == 0) {
    h8_span_mark_orphan_ready(span);
  }
  if (atomic_load_explicit(&span->pending_count, memory_order_acquire) != 0) {
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
