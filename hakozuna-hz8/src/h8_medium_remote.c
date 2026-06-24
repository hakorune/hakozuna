#include "h8_internal.h"
#include "h8_medium.h"

#include <stdlib.h>

static uint64_t h8_medium_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner || owner->slot >= H8_OWNER_MAX) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

bool h8_medium_run_owned_by_ctx(const H8MediumRun* run,
                                const H8ThreadCtx* ctx) {
  if (!run || !ctx || !ctx->owner) {
    return false;
  }
  uint64_t expected = h8_medium_owner_word_for(ctx->owner);
  uint64_t current =
      atomic_load_explicit(&run->owner_word, memory_order_acquire);
  return expected != 0 && current == expected;
}

static void h8_medium_pending_queue_push(H8OwnerRecord* owner,
                                         H8MediumRun* run) {
  pthread_mutex_lock(&owner->pending_lock);
  run->next_pending = owner->medium_pending_head;
  owner->medium_pending_head = run;
  atomic_fetch_add_explicit(&owner->medium_pending_count, 1,
                            memory_order_release);
  pthread_mutex_unlock(&owner->pending_lock);
}

static void h8_medium_signal_work(H8OwnerRecord* owner, H8MediumRun* run) {
  uint8_t cur = atomic_load_explicit(&run->qstate, memory_order_acquire);
  for (;;) {
    if (cur == H8_Q_IDLE) {
      uint8_t expected = H8_Q_IDLE;
      if (atomic_compare_exchange_weak_explicit(&run->qstate, &expected,
                                                H8_Q_QUEUED,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        h8_medium_pending_queue_push(owner, run);
        return;
      }
      cur = expected;
      continue;
    }
    if (cur == H8_Q_DRAINING) {
      uint8_t expected = H8_Q_DRAINING;
      if (atomic_compare_exchange_weak_explicit(&run->qstate, &expected,
                                                H8_Q_DRAINING_DIRTY,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        H8_DEBUG_INC(qstate_dirty_set);
        return;
      }
      cur = expected;
      continue;
    }
    return;
  }
}

static bool h8_medium_claim_accepted_by_collector(H8MediumRun* run) {
  uint8_t qstate = atomic_load_explicit(&run->qstate, memory_order_acquire);
  return qstate == H8_Q_DRAINING || qstate == H8_Q_DRAINING_DIRTY;
}

H8PublishResult h8_medium_remote_publish(H8MediumRun* run, void* ptr) {
  if (!run || !ptr) {
    return H8_PUBLISH_MISS;
  }
  uint64_t owner_raw =
      atomic_load_explicit(&run->owner_word, memory_order_acquire);
  H8OwnerWord ow = h8_owner_word_unpack(owner_raw);
  H8OwnerRecord* owner = h8_owner_by_slot(ow.slot);
  if (!owner || owner->permanent ||
      ow.state != H8_SPAN_OWNED_ACTIVE ||
      !h8_owner_publish_enter(owner, ow.generation)) {
    return H8_PUBLISH_OWNER_TRANSITION;
  }

  H8PublishResult result = H8_PUBLISH_INVALID;
  bool notify = false;
  pthread_mutex_lock(&run->lock);
  if (atomic_load_explicit(&run->owner_word, memory_order_acquire) != owner_raw ||
      atomic_load_explicit(&run->state, memory_order_acquire) !=
          H8_MEDIUM_RUN_ACTIVE) {
    result = H8_PUBLISH_OWNER_TRANSITION;
    goto out;
  }

  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    result = H8_PUBLISH_INVALID;
    goto out;
  }
  uint64_t bit = UINT64_C(1) << slot;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    result = H8_PUBLISH_INVALID;
    goto out;
  }

  uint64_t old_word =
      atomic_fetch_or_explicit(&run->pending_bits[0], bit, memory_order_acq_rel);
  if (old_word & bit) {
    H8_DEBUG_INC(remote_publish_pending_claim_duplicate_count);
    result = H8_PUBLISH_DOUBLE_FREE;
    goto out;
  }

  state = atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    if (h8_medium_claim_accepted_by_collector(run)) {
      result = H8_PUBLISH_OK;
      goto out;
    }
    uint64_t rollback = atomic_fetch_and_explicit(
        &run->pending_bits[0], ~bit, memory_order_acq_rel);
    result = (rollback & bit) ? H8_PUBLISH_INVALID : H8_PUBLISH_OK;
    goto out;
  }

  notify = old_word == 0;
  result = H8_PUBLISH_OK;

out:
  pthread_mutex_unlock(&run->lock);
  if (notify) {
    h8_medium_signal_work(owner, run);
  }
  h8_owner_publish_exit(owner);
  return result;
}

static bool h8_medium_collect_run(H8OwnerRecord* owner, H8MediumRun* run) {
  uint8_t expected = H8_Q_QUEUED;
  if (!atomic_compare_exchange_strong_explicit(&run->qstate, &expected,
                                               H8_Q_DRAINING,
                                               memory_order_acq_rel,
                                               memory_order_acquire)) {
    return false;
  }

  pthread_mutex_lock(&run->lock);
  uint64_t bits = atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire);
  uint64_t valid = run->slot_count == 64u
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  bits &= valid;
  uint64_t collect = bits;
  while (collect) {
    uint64_t bit = collect & (~collect + 1u);
    size_t slot = (size_t)__builtin_ctzll(bit);
    uint32_t state =
        atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
    if (h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
      atomic_store_explicit(&run->slot_state[slot],
                            H8_SLOT_FREE | H8_SLOT_NONE,
                            memory_order_release);
      run->allocated_mask &= ~bit;
      atomic_fetch_and_explicit(&run->pending_bits[0], ~bit,
                                memory_order_acq_rel);
      run->free_mask |= bit;
    } else {
      H8_DEBUG_INC(invalid_count);
      atomic_fetch_and_explicit(&run->pending_bits[0], ~bit,
                                memory_order_acq_rel);
    }
    collect &= collect - 1u;
  }
  if (run->allocated_mask == 0) {
    h8_medium_mark_empty_locked(run);
  }
  pthread_mutex_unlock(&run->lock);

  uint8_t expected_finish = H8_Q_DRAINING;
  if (atomic_compare_exchange_strong_explicit(&run->qstate, &expected_finish,
                                              H8_Q_IDLE, memory_order_acq_rel,
                                              memory_order_acquire)) {
    (void)owner;
    return false;
  }
  if (expected_finish == H8_Q_DRAINING_DIRTY) {
    uint8_t expected_dirty = H8_Q_DRAINING_DIRTY;
    if (atomic_compare_exchange_strong_explicit(&run->qstate, &expected_dirty,
                                                H8_Q_QUEUED,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
      H8_DEBUG_INC(qstate_dirty_requeue);
      return true;
    }
  }
  return false;
}

void h8_medium_collect_owner_pending(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  if (atomic_load_explicit(&owner->medium_pending_count,
                           memory_order_acquire) == 0) {
    return;
  }
  for (;;) {
    pthread_mutex_lock(&owner->pending_lock);
    H8MediumRun* list = owner->medium_pending_head;
    owner->medium_pending_head = NULL;
    pthread_mutex_unlock(&owner->pending_lock);
    if (!list) {
      return;
    }
    while (list) {
      H8MediumRun* run = list;
      list = run->next_pending;
      run->next_pending = NULL;
      atomic_fetch_sub_explicit(&owner->medium_pending_count, 1,
                                memory_order_acq_rel);
      if (h8_medium_collect_run(owner, run)) {
        h8_medium_pending_queue_push(owner, run);
      }
    }
  }
}
