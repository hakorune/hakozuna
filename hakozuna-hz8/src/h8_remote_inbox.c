#include "h8_internal.h"
#include "h8_used_count.h"

#include <stdlib.h>

static void h8_pending_queue_push(H8OwnerRecord* owner, H8Span* span) {
  H8_DEBUG_INC(pending_head_push_attempt_count);
  H8Span* head = atomic_load_explicit(&owner->pending_head, memory_order_relaxed);
  for (;;) {
    span->next_pending = head;
    if (atomic_compare_exchange_weak_explicit(&owner->pending_head, &head, span,
                                              memory_order_release,
                                              memory_order_relaxed)) {
      break;
    }
    H8_DEBUG_INC(pending_head_push_retry_count);
  }
  H8_DEBUG_INC(pending_head_push_success_count);
  atomic_fetch_add_explicit(&owner->pending_span_count, 1, memory_order_relaxed);
  H8_DEBUG_INC(pending_enqueue_count);
}

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_pending_count_dec(H8Span* span) {
  size_t prev = atomic_fetch_sub_explicit(&span->pending_count, 1, memory_order_relaxed);
  if (prev == 0) {
    abort();
  }
}

static void h8_pending_count_sub(H8Span* span, size_t count) {
  size_t prev =
      atomic_fetch_sub_explicit(&span->pending_count, count, memory_order_relaxed);
  if (prev < count) {
    abort();
  }
}
#endif

static void h8_used_count_sub(H8Span* span, size_t count) {
#if defined(H8_ENABLE_DEBUG_STATS)
  if (!h8_used_count_mirror_sub(span, count)) {
    abort();
  }
#else
  (void)span;
  (void)count;
#endif
}

static uint64_t h8_word_valid_mask(H8Span* span, size_t word_index) {
  size_t first_slot = word_index << 6u;
  if (first_slot >= span->slot_count) {
    return 0;
  }
  size_t remaining = span->slot_count - first_slot;
  if (remaining >= 64u) {
    return UINT64_MAX;
  }
  return (UINT64_C(1) << remaining) - 1u;
}

static void h8_pending_word_bucket_add(size_t popcount) {
  if (popcount == 1) {
    H8_DEBUG_INC(pending_word_popcount_1);
  } else if (popcount == 2) {
    H8_DEBUG_INC(pending_word_popcount_2);
  } else if (popcount <= 4) {
    H8_DEBUG_INC(pending_word_popcount_3_4);
  } else if (popcount <= 8) {
    H8_DEBUG_INC(pending_word_popcount_5_8);
  } else if (popcount <= 16) {
    H8_DEBUG_INC(pending_word_popcount_9_16);
  } else {
    H8_DEBUG_INC(pending_word_popcount_17_plus);
  }
}

#if defined(H8_ENABLE_DEBUG_STATS)
static bool h8_pending_bitmap_any(H8Span* span, size_t words) {
  for (size_t i = 0; i < words; ++i) {
    if (atomic_load_explicit(&((_Atomic uint64_t*)span->pending_bits)[i],
                             memory_order_acquire) &
        h8_word_valid_mask(span, i)) {
      return true;
    }
  }
  return false;
}
#endif

bool h8_span_pending_quiescent(H8Span* span) {
  size_t words = h8_word_count_for_slots(span->slot_count);
  if (atomic_load_explicit(&span->qstate, memory_order_acquire) != H8_Q_IDLE ||
      atomic_load_explicit(&span->pending_word_mask, memory_order_acquire) != 0) {
    return false;
  }
  for (size_t i = 0; i < words; ++i) {
    if (atomic_load_explicit(&span->pending_bits[i], memory_order_acquire) &
        h8_word_valid_mask(span, i)) {
      H8_DEBUG_INC(quiescent_pending_bitmap_nonzero);
      return false;
    }
  }
  return true;
}

bool h8_span_repair_pending_mask(H8OwnerRecord* owner, H8Span* span) {
  size_t words = h8_word_count_for_slots(span->slot_count);
  uint64_t mask = 0;
  for (size_t i = 0; i < words; ++i) {
    if (atomic_load_explicit(&span->pending_bits[i], memory_order_acquire) &
        h8_word_valid_mask(span, i)) {
      mask |= UINT64_C(1) << i;
    }
  }
  if (mask == 0) {
    return false;
  }
  atomic_fetch_or_explicit(&span->pending_word_mask, mask, memory_order_release);
  H8_DEBUG_INC(quiescent_pending_repair);
  h8_span_notify(owner, span);
  return true;
}

static bool h8_publish_claim_accepted_by_collector(H8Span* span) {
  uint8_t qstate = atomic_load_explicit(&span->qstate, memory_order_acquire);
  return qstate == H8_Q_DRAINING || qstate == H8_Q_DRAINING_DIRTY;
}

static void h8_collect_one_slot(H8Span* span, size_t word_index, uint64_t bit) {
  size_t bit_index = (size_t)__builtin_ctzll(bit);
  size_t slot = (word_index << 6u) + bit_index;
  uint64_t clear_mask = ~bit;
  _Atomic uint64_t* pending_word = &((_Atomic uint64_t*)span->pending_bits)[word_index];

  H8_DEBUG_INC(pending_collect_bit_count);
  uint32_t state = h8_slot_state_load_hot(span, slot);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  _Atomic uint64_t* live_word = &span->live_bits[word_index];
  uint64_t old_live =
      atomic_fetch_and_explicit(live_word, clear_mask, memory_order_acq_rel);
  if ((old_live & bit) == 0) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
#endif
  uint32_t old_head = atomic_load_explicit(&span->local_hot.local_free_head_word,
                                           memory_order_relaxed);
  h8_slot_state_store_free_hot(span, slot, old_head);
  uint64_t old_pending =
      atomic_fetch_and_explicit(pending_word, clear_mask, memory_order_acq_rel);
  if ((old_pending & bit) == 0) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_pending_count_dec(span);
#endif
  atomic_store_explicit(&span->local_hot.local_free_head_word, (uint32_t)slot,
                        memory_order_release);
  h8_used_count_sub(span, 1);
  H8_DEBUG_INC(remote_collect_count);
  H8_DEBUG_INC(pending_dequeue_count);
}

static void h8_collect_bulk_word(H8Span* span, size_t word_index, uint64_t claimed,
                                 size_t count) {
  _Atomic uint64_t* pending_word = &((_Atomic uint64_t*)span->pending_bits)[word_index];
  uint64_t clear_mask = ~claimed;

  uint64_t slots = claimed;
  while (slots) {
    uint64_t bit = slots & (~slots + 1ull);
    size_t bit_index = (size_t)__builtin_ctzll(slots);
    size_t slot = (word_index << 6u) + bit_index;
    slots ^= bit;
    uint32_t state = h8_slot_state_load_hot(span, slot);
    if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
      H8_DEBUG_INC(invalid_count);
      abort();
    }
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  _Atomic uint64_t* live_word = &span->live_bits[word_index];
  uint64_t old_live =
      atomic_fetch_and_explicit(live_word, clear_mask, memory_order_acq_rel);
  if ((old_live & claimed) != claimed) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
#endif
  uint32_t head = atomic_load_explicit(&span->local_hot.local_free_head_word,
                                       memory_order_relaxed);
  slots = claimed;
  while (slots) {
    uint64_t bit = slots & (~slots + 1ull);
    size_t bit_index = (size_t)__builtin_ctzll(slots);
    size_t slot = (word_index << 6u) + bit_index;
    slots ^= bit;
    h8_slot_state_store_free_hot(span, slot, head);
    head = (uint32_t)slot;
  }
  uint64_t old_pending =
      atomic_fetch_and_explicit(pending_word, clear_mask, memory_order_acq_rel);
  if ((old_pending & claimed) != claimed) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }

  atomic_store_explicit(&span->local_hot.local_free_head_word, head,
                        memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_pending_count_sub(span, count);
#endif
  h8_used_count_sub(span, count);
  H8_DEBUG_ADD(pending_collect_bit_count, count);
  H8_DEBUG_ADD(remote_collect_count, count);
  H8_DEBUG_ADD(pending_dequeue_count, count);
}

static void h8_collect_pending_word(H8Span* span, size_t word_index, bool from_summary) {
  uint64_t summary_bit = UINT64_C(1) << word_index;
  H8_DEBUG_INC(pending_collect_word_count);
  uint64_t valid_mask = h8_word_valid_mask(span, word_index);
  uint64_t bits = atomic_load_explicit(
                      &((_Atomic uint64_t*)span->pending_bits)[word_index],
                      memory_order_acquire) &
                  valid_mask;
  bool bits_present = bits != 0;

  if (from_summary && bits_present) {
    H8_DEBUG_INC(pending_word_summary_shadow_hit);
  } else if (from_summary && !bits_present) {
    H8_DEBUG_INC(pending_word_summary_false_positive);
  } else if (!from_summary && bits_present) {
    H8_DEBUG_INC(pending_word_summary_false_negative);
  }

  if (bits) {
    size_t word_popcount = (size_t)__builtin_popcountll(bits);
    H8_DEBUG_INC(pending_collect_word_nonzero_count);
    H8_DEBUG_INC(pending_word_drain_count);
    H8_DEBUG_ADD(pending_slots_drained, word_popcount);
    h8_pending_word_bucket_add(word_popcount);
    if (!from_summary) {
      H8_DEBUG_INC(pending_word_new_publish_during_drain);
    }
    if (word_popcount == 1) {
      h8_collect_one_slot(span, word_index, bits);
    } else {
      h8_collect_bulk_word(span, word_index, bits, word_popcount);
    }
  }

  uint64_t remaining = atomic_load_explicit(
                           &((_Atomic uint64_t*)span->pending_bits)[word_index],
                           memory_order_acquire) &
                       valid_mask;
  if (remaining != 0) {
    atomic_fetch_or_explicit(&span->pending_word_mask, summary_bit,
                             memory_order_release);
    H8_DEBUG_INC(pending_word_summary_rearm);
    H8_DEBUG_INC(pending_words_rearmed);
  }
}

void h8_span_notify(H8OwnerRecord* owner, H8Span* span) {
  H8_DEBUG_INC(pending_notify_count);
  H8_DEBUG_INC(qstate_notify_attempt_count);
  uint8_t cur = atomic_load_explicit(&span->qstate, memory_order_acquire);
  for (;;) {
    if (cur == H8_Q_IDLE) {
      uint8_t expected = H8_Q_IDLE;
      if (atomic_compare_exchange_weak_explicit(&span->qstate, &expected,
                                                H8_Q_QUEUED,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        H8_DEBUG_INC(qstate_notify_success_count);
        h8_pending_queue_push(owner, span);
        return;
      }
      cur = expected;
      continue;
    }
    if (cur == H8_Q_DRAINING) {
      uint8_t expected = H8_Q_DRAINING;
      if (atomic_compare_exchange_weak_explicit(&span->qstate, &expected,
                                                H8_Q_DRAINING_DIRTY,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        H8_DEBUG_INC(qstate_dirty_set);
        H8_DEBUG_INC(qstate_notify_success_count);
        return;
      }
      cur = expected;
      continue;
    }
    H8_DEBUG_INC(qstate_notify_skip_nonidle_count);
    return;
  }
}

static H8Span* h8_remote_span_from_ptr_checked(void* ptr, size_t* slot_out) {
  H8_DEBUG_INC(remote_lookup_enter);
  if (!ptr || !h8_arena_contains(ptr)) {
    H8_DEBUG_INC(remote_lookup_arena_miss);
    return NULL;
  }
  size_t index = h8_span_index_from_ptr(ptr);
  H8Span* span = atomic_load_explicit(&h8g.spans[index], memory_order_acquire);
  if (!span) {
    H8_DEBUG_INC(remote_lookup_span_miss);
    return NULL;
  }
  if (h8_span_state_load(span) == H8_SPAN_RETIRED) {
    H8_DEBUG_INC(remote_lookup_retired);
    return NULL;
  }
  size_t slot = 0;
  if (!h8_slot_index_from_ptr_checked(span, ptr, &slot)) {
    H8_DEBUG_INC(remote_lookup_slot_oob);
    return NULL;
  }
  *slot_out = slot;
  H8_DEBUG_INC(remote_lookup_ok);
  return span;
}

static inline __attribute__((always_inline)) H8PublishResult
h8_remote_free_publish_locked(H8Span* span, H8OwnerRecord* owner, size_t slot) {
  size_t word_index = slot >> 6u;
  uint64_t slot_bit = UINT64_C(1) << (slot & 63u);
  uint64_t word_bit = UINT64_C(1) << word_index;
  _Atomic uint64_t* pending_word = &((_Atomic uint64_t*)span->pending_bits)[word_index];
  bool pending_elision = h8_remote_pending_publish_elision_enabled();
  uint32_t state = h8_slot_state_load_hot(span, slot);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    H8_DEBUG_INC(remote_stage_validate_fail);
    return H8_PUBLISH_INVALID;
  }
  if (pending_elision) {
    /*
     * Unsafe evidence mode: this intentionally drops remote-free publication
     * after validation.  It is only a speed ceiling probe and leaks remote frees.
     */
    H8_DEBUG_INC(remote_stage_pending_publish_elided);
    H8_DEBUG_INC(remote_stage_publish_ok);
    return H8_PUBLISH_OK;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t prev = atomic_fetch_add_explicit(&span->pending_count, 1,
                                          memory_order_acq_rel);
#else
  size_t prev = 1;
#endif
  uint64_t old_word =
      atomic_fetch_or_explicit(pending_word, slot_bit, memory_order_acq_rel);
  if (old_word & slot_bit) {
#if defined(H8_ENABLE_DEBUG_STATS)
    h8_pending_count_dec(span);
#endif
    H8_DEBUG_INC(remote_publish_pending_claim_duplicate_count);
    return H8_PUBLISH_DOUBLE_FREE;
  }
  state = h8_slot_state_load_hot(span, slot);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    if (h8_publish_claim_accepted_by_collector(span)) {
      H8_DEBUG_INC(remote_stage_publish_ok);
      return H8_PUBLISH_OK;
    }
    uint64_t rollback =
        atomic_fetch_and_explicit(pending_word, ~slot_bit, memory_order_acq_rel);
    if ((rollback & slot_bit) == 0) {
      H8_DEBUG_INC(remote_stage_publish_ok);
      return H8_PUBLISH_OK;
    }
#if defined(H8_ENABLE_DEBUG_STATS)
    h8_pending_count_dec(span);
#endif
    H8_DEBUG_INC(remote_stage_validate_fail);
    return H8_PUBLISH_INVALID;
  }
  /* The pending bit is the publish commit point; collectors may process it now. */
  H8_DEBUG_INC(remote_stage_pending_claim_ok);
  H8_DEBUG_INC(remote_publish_count);
  if (old_word == 0) {
    atomic_fetch_or_explicit(&span->pending_word_mask, word_bit,
                             memory_order_release);
    H8_DEBUG_INC(pending_word_summary_set);
    if (prev != 0) {
      H8_DEBUG_INC(pending_mask_notify_without_count);
    }
    H8_DEBUG_INC(remote_stage_notify_first);
    h8_span_notify(owner, span);
  } else if (prev == 0) {
    H8_DEBUG_INC(pending_count_notify_without_mask);
  }
  H8_DEBUG_INC(remote_stage_publish_ok);
  return H8_PUBLISH_OK;
}

#if defined(H8_REMOTE_SPAN_LEASE_PUBLISH_L1)
static H8PublishResult h8_remote_free_publish_span_lease(H8Span* span,
                                                         H8OwnerRecord* owner,
                                                         H8OwnerWord expected,
                                                         size_t slot) {
  if (!h8_span_publish_enter(span)) {
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8OwnerWord current = h8_span_owner_word_load(span);
  if (current.slot != expected.slot ||
      current.generation != expected.generation ||
      h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
    h8_span_publish_exit(span);
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot);
  h8_span_publish_exit(span);
  return res;
}
#endif

H8PublishResult h8_remote_free_publish_known(H8Span* span, size_t slot) {
  H8_DEBUG_INC(remote_stage_enter);
  H8OwnerWord ow = h8_span_owner_word_load(span);
  H8_DEBUG_INC(remote_owner_word_load);
  H8OwnerRecord* owner = h8_owner_by_slot(ow.slot);
  if (!owner) {
    H8_DEBUG_INC(owner_transition_count);
    H8_DEBUG_INC(remote_stage_owner_missing);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (owner->permanent) {
    if (!h8_span_publish_enter(span)) {
      H8_DEBUG_INC(owner_transition_count);
      H8_DEBUG_INC(remote_stage_orphan_lease_fail);
      return H8_PUBLISH_OWNER_TRANSITION;
    }
    H8_DEBUG_INC(remote_stage_orphan_lease_ok);
    H8_DEBUG_INC(remote_orphan_admission_count);
    H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot);
    h8_span_publish_exit(span);
    return res;
  }
  if (h8_remote_lease_elision_enabled()) {
    /*
     * Unsafe evidence mode: skipping the owner lifecycle lease removes the
     * handoff/reuse protection and must not be used for correctness claims.
     */
    H8_DEBUG_INC(remote_stage_regular_lease_elided);
    H8_DEBUG_INC(remote_regular_admission_count);
    return h8_remote_free_publish_locked(span, owner, slot);
  }
#if defined(H8_REMOTE_SPAN_LEASE_PUBLISH_L1)
  H8_DEBUG_INC(remote_regular_admission_count);
  return h8_remote_free_publish_span_lease(span, owner, ow, slot);
#else
  if (!h8_owner_publish_enter(owner, ow.generation)) {
    H8_DEBUG_INC(owner_transition_count);
    H8_DEBUG_INC(remote_stage_regular_lease_fail);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8_DEBUG_INC(remote_stage_regular_lease_ok);
  H8_DEBUG_INC(remote_regular_admission_count);
  H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot);
  h8_owner_publish_exit(owner);
  return res;
#endif
}

H8PublishResult h8_remote_free_publish(void* ptr) {
  size_t slot = 0;
  H8Span* span = h8_remote_span_from_ptr_checked(ptr, &slot);
  if (!span) {
    H8_DEBUG_INC(remote_stage_enter);
    H8_DEBUG_INC(remote_stage_span_miss);
    return H8_PUBLISH_MISS;
  }
  return h8_remote_free_publish_known(span, slot);
}

void h8_span_collect_remote(H8OwnerRecord* owner, H8Span* span) {
  uint8_t expected = H8_Q_QUEUED;
  if (!atomic_compare_exchange_strong_explicit(&span->qstate, &expected,
                                               H8_Q_DRAINING, memory_order_acq_rel,
                                               memory_order_acquire)) {
    return;
  }

  size_t words = h8_word_count_for_slots(span->slot_count);
  uint64_t word_mask = atomic_exchange_explicit(&span->pending_word_mask, 0,
                                                memory_order_acq_rel);
  while (word_mask) {
    size_t word_index = (size_t)__builtin_ctzll(word_mask);
    word_mask &= word_mask - 1;
    if (word_index >= words) {
      continue;
    }
    h8_collect_pending_word(span, word_index, true);
  }

  if (atomic_load_explicit(&span->pending_word_mask, memory_order_acquire) != 0) {
    uint8_t expected_dirty = H8_Q_DRAINING;
    bool set_dirty = atomic_compare_exchange_strong_explicit(
        &span->qstate, &expected_dirty, H8_Q_DRAINING_DIRTY,
        memory_order_acq_rel, memory_order_acquire);
    if (set_dirty) {
      H8_DEBUG_INC(qstate_dirty_self_set);
    }
  }

  uint8_t expected_finish = H8_Q_DRAINING;
  if (!atomic_compare_exchange_strong_explicit(&span->qstate, &expected_finish,
                                               H8_Q_IDLE, memory_order_acq_rel,
                                               memory_order_acquire)) {
    if (expected_finish == H8_Q_DRAINING_DIRTY) {
      uint8_t expected_dirty = H8_Q_DRAINING_DIRTY;
      if (atomic_compare_exchange_strong_explicit(
              &span->qstate, &expected_dirty, H8_Q_QUEUED,
              memory_order_acq_rel, memory_order_acquire)) {
        H8_DEBUG_INC(qstate_dirty_requeue);
        h8_pending_queue_push(owner, span);
      }
    }
  }
  if ((H8SpanState)h8_span_owner_word_load(span).state == H8_SPAN_ORPHAN_QUIESCING &&
      atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
      h8_span_pending_quiescent(span)) {
    h8_span_mark_orphan_ready(span);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_slot_shadow_verify_span(span);
  size_t finish_count =
      atomic_load_explicit(&span->pending_count, memory_order_acquire);
  uint64_t finish_mask =
      atomic_load_explicit(&span->pending_word_mask, memory_order_acquire);
  if (finish_mask != 0 && finish_count == 0) {
    H8_DEBUG_INC(pending_mask_requeue_without_count);
  } else if (finish_mask == 0 && finish_count != 0) {
    H8_DEBUG_INC(pending_count_requeue_without_mask);
  }
  bool finish_bitmap_any = h8_pending_bitmap_any(span, words);
  if (finish_count != 0 && finish_mask == 0) {
    if (finish_bitmap_any) {
      H8_DEBUG_INC(pending_finish_count_mask_zero_bitmap_nonzero);
    } else {
      H8_DEBUG_INC(pending_finish_count_mask_zero_bitmap_zero);
    }
  } else if (finish_mask != 0) {
    if (finish_bitmap_any) {
      H8_DEBUG_INC(pending_finish_mask_nonzero_bitmap_nonzero);
    } else {
      H8_DEBUG_INC(pending_finish_mask_nonzero_bitmap_zero);
    }
  }
#endif
}

static size_t h8_collect_pending_list(H8OwnerRecord* owner, H8Span* list,
                                      size_t budget, H8Span** remainder_out) {
  size_t collected = 0;
  H8Span* remainder = NULL;
  while (list) {
    H8Span* span = list;
    list = span->next_pending;
    span->next_pending = NULL;
    if (collected == budget) {
      remainder = span;
      remainder->next_pending = list;
      break;
    }
    h8_span_collect_remote(owner, span);
    atomic_fetch_sub_explicit(&owner->pending_span_count, 1, memory_order_relaxed);
    ++collected;
  }
  *remainder_out = remainder;
  return collected;
}

size_t h8_collect_owner_pending_budget(H8OwnerRecord* owner, size_t budget) {
  if (!owner || budget == 0) {
    return 0;
  }
  H8_DEBUG_INC(pending_collect_call_count);

  h8_platform_mutex_lock(&owner->pending_lock);

  H8Span* carry = owner->pending_carry;
  owner->pending_carry = NULL;
  size_t collected = 0;

  if (carry) {
    H8_DEBUG_INC(pending_collect_carry_hit_count);
    collected += h8_collect_pending_list(owner, carry, budget, &carry);
    if (carry) {
      owner->pending_carry = carry;
      H8_DEBUG_INC(pending_collect_requeue_count);
      h8_platform_mutex_unlock(&owner->pending_lock);
      return collected;
    }
  }

  if (collected < budget) {
    H8Span* list = atomic_exchange_explicit(&owner->pending_head, NULL,
                                            memory_order_acq_rel);
    H8Span* remainder = NULL;
    collected += h8_collect_pending_list(owner, list, budget - collected, &remainder);
    if (remainder) {
      owner->pending_carry = remainder;
      H8_DEBUG_INC(pending_collect_requeue_count);
    }
  }

  h8_platform_mutex_unlock(&owner->pending_lock);
  return collected;
}

void h8_collect_owner_pending(H8OwnerRecord* owner) {
  (void)h8_collect_owner_pending_budget(owner, SIZE_MAX);
}
