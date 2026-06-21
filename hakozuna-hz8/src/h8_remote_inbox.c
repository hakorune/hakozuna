#include "h8_internal.h"

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

static void h8_used_count_sub(H8Span* span, size_t count) {
  size_t used = atomic_load_explicit(&span->used_count, memory_order_relaxed);
  if (used < count) {
    abort();
  }
  atomic_store_explicit(&span->used_count, used - count, memory_order_relaxed);
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

static void h8_collect_one_slot(H8Span* span, size_t word_index, uint64_t bit) {
  size_t bit_index = (size_t)__builtin_ctzll(bit);
  size_t slot = (word_index << 6u) + bit_index;
  uint64_t clear_mask = ~bit;
  _Atomic uint64_t* live_word = &((_Atomic uint64_t*)span->live_bits)[word_index];
  _Atomic uint64_t* pending_word = &((_Atomic uint64_t*)span->pending_bits)[word_index];

  H8_DEBUG_INC(pending_collect_bit_count);
  uint64_t old_live =
      atomic_fetch_and_explicit(live_word, clear_mask, memory_order_acq_rel);
  if ((old_live & bit) == 0) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
  uint64_t old_pending =
      atomic_fetch_and_explicit(pending_word, clear_mask, memory_order_acq_rel);
  if ((old_pending & bit) == 0) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
  h8_pending_count_dec(span);
  span->next_free[slot] = atomic_load_explicit(&span->local_free_head,
                                               memory_order_relaxed);
  atomic_store_explicit(&span->local_free_head, (uint32_t)slot,
                        memory_order_release);
  h8_used_count_sub(span, 1);
  H8_DEBUG_INC(remote_collect_count);
  H8_DEBUG_INC(pending_dequeue_count);
}

static void h8_collect_bulk_word(H8Span* span, size_t word_index, uint64_t claimed,
                                 size_t count) {
  _Atomic uint64_t* live_word = &((_Atomic uint64_t*)span->live_bits)[word_index];
  _Atomic uint64_t* pending_word = &((_Atomic uint64_t*)span->pending_bits)[word_index];
  uint64_t clear_mask = ~claimed;

  uint64_t old_live =
      atomic_fetch_and_explicit(live_word, clear_mask, memory_order_acq_rel);
  if ((old_live & claimed) != claimed) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }
  uint64_t old_pending =
      atomic_fetch_and_explicit(pending_word, clear_mask, memory_order_acq_rel);
  if ((old_pending & claimed) != claimed) {
    H8_DEBUG_INC(invalid_count);
    abort();
  }

  uint32_t head = atomic_load_explicit(&span->local_free_head, memory_order_relaxed);
  uint64_t slots = claimed;
  while (slots) {
    uint64_t bit = slots & (~slots + 1ull);
    size_t bit_index = (size_t)__builtin_ctzll(slots);
    size_t slot = (word_index << 6u) + bit_index;
    slots ^= bit;
    span->next_free[slot] = head;
    head = (uint32_t)slot;
  }
  atomic_store_explicit(&span->local_free_head, head, memory_order_release);
  h8_pending_count_sub(span, count);
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
  } else {
    atomic_fetch_and_explicit(&span->pending_word_mask, ~summary_bit,
                              memory_order_release);
  }
}

void h8_span_notify(H8OwnerRecord* owner, H8Span* span) {
  H8_DEBUG_INC(pending_notify_count);
  H8_DEBUG_INC(qstate_notify_attempt_count);
  uint8_t expected = H8_Q_IDLE;
  if (atomic_compare_exchange_strong_explicit(&span->qstate, &expected, H8_Q_QUEUED,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    H8_DEBUG_INC(qstate_notify_success_count);
    h8_pending_queue_push(owner, span);
  } else {
    H8_DEBUG_INC(qstate_notify_skip_nonidle_count);
  }
}

static H8PublishResult h8_remote_free_publish_locked(H8Span* span, H8OwnerRecord* owner,
                                                     size_t slot) {
  size_t word_index = slot >> 6u;
  uint64_t slot_bit = UINT64_C(1) << (slot & 63u);
  uint64_t word_bit = UINT64_C(1) << word_index;
  _Atomic uint64_t* pending_word = &((_Atomic uint64_t*)span->pending_bits)[word_index];
  uint64_t old_word = atomic_fetch_or_explicit(
      pending_word, slot_bit, memory_order_acq_rel);
  if (old_word & slot_bit) {
    H8_DEBUG_INC(remote_publish_pending_claim_duplicate_count);
    return H8_PUBLISH_DOUBLE_FREE;
  }
  if (!h8_bitmap_test((_Atomic uint64_t*)span->live_bits, slot)) {
    atomic_fetch_and_explicit(pending_word, ~slot_bit, memory_order_acq_rel);
    return H8_PUBLISH_INVALID;
  }
  H8_DEBUG_INC(remote_publish_count);
  if (old_word == 0) {
    atomic_fetch_or_explicit(&span->pending_word_mask, word_bit,
                             memory_order_release);
    H8_DEBUG_INC(pending_word_summary_set);
  }
  size_t prev = atomic_fetch_add_explicit(&span->pending_count, 1, memory_order_acq_rel);
  if (prev == 0) {
    h8_span_notify(owner, span);
  }
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
  if (!owner) {
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  if (owner->permanent) {
    if (!h8_span_publish_enter(span)) {
      H8_DEBUG_INC(owner_transition_count);
      return H8_PUBLISH_OWNER_TRANSITION;
    }
    H8_DEBUG_INC(remote_orphan_admission_count);
    H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot);
    h8_span_publish_exit(span);
    return res;
  }
  if (!h8_owner_publish_enter(owner, ow.generation)) {
    H8_DEBUG_INC(owner_transition_count);
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8_DEBUG_INC(remote_regular_admission_count);
  H8PublishResult res = h8_remote_free_publish_locked(span, owner, slot);
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

  if (atomic_load_explicit(&span->pending_count, memory_order_acquire) != 0 &&
      atomic_load_explicit(&span->pending_word_mask, memory_order_acquire) == 0) {
    H8_DEBUG_INC(pending_word_summary_repair);
    for (size_t word_index = 0; word_index < words; ++word_index) {
      h8_collect_pending_word(span, word_index, false);
    }
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

void h8_collect_owner_pending_budget(H8OwnerRecord* owner, size_t budget) {
  if (!owner || budget == 0) {
    return;
  }
  H8_DEBUG_INC(pending_collect_call_count);

  pthread_mutex_lock(&owner->pending_lock);

  H8Span* carry = owner->pending_carry;
  owner->pending_carry = NULL;
  size_t collected = 0;

  if (carry) {
    H8_DEBUG_INC(pending_collect_carry_hit_count);
    collected += h8_collect_pending_list(owner, carry, budget, &carry);
    if (carry) {
      owner->pending_carry = carry;
      H8_DEBUG_INC(pending_collect_requeue_count);
      pthread_mutex_unlock(&owner->pending_lock);
      return;
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

  pthread_mutex_unlock(&owner->pending_lock);
}

void h8_collect_owner_pending(H8OwnerRecord* owner) {
  h8_collect_owner_pending_budget(owner, SIZE_MAX);
}
