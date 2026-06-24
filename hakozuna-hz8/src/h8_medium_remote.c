#include "h8_internal.h"
#include "h8_medium.h"

#include <stdlib.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
static uint64_t h8_medium_remote_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

enum {
  H8_MEDIUM_COLLECT_PERIOD = 8u,
  H8_MEDIUM_COLLECT_BUDGET = 4u,
};

static uint64_t h8_medium_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner || owner->slot >= H8_OWNER_MAX) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

static void h8_medium_debug_class_add(uint16_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k,
                                      size_t value) {
#if defined(H8_ENABLE_DEBUG_STATS)
  atomic_size_t* target = NULL;
  if (class_id == 0) {
    target = class_8k;
  } else if (class_id == 1) {
    target = class_16k;
  } else if (class_id == 2) {
    target = class_32k;
  } else if (class_id == 3) {
    target = class_64k;
  }
  if (target) {
    atomic_fetch_add_explicit(target, value, memory_order_relaxed);
  }
#else
  (void)class_id;
  (void)class_8k;
  (void)class_16k;
  (void)class_32k;
  (void)class_64k;
  (void)value;
#endif
}

static void h8_medium_debug_class_inc(uint16_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k) {
  h8_medium_debug_class_add(class_id, class_8k, class_16k, class_32k,
                            class_64k, 1);
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
  H8_DEBUG_INC(medium_remote_queue_push_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t push_start = h8_medium_remote_now_ns();
#endif
  pthread_mutex_lock(&owner->pending_lock);
  run->next_pending = owner->medium_pending_head;
  owner->medium_pending_head = run;
  atomic_fetch_add_explicit(&owner->medium_pending_count, 1,
                            memory_order_release);
  pthread_mutex_unlock(&owner->pending_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_remote_queue_push_ns,
               (size_t)(h8_medium_remote_now_ns() - push_start));
#endif
  h8_medium_debug_class_inc(run->class_id, &h8g.medium_remote_qpush_class_8k,
                            &h8g.medium_remote_qpush_class_16k,
                            &h8g.medium_remote_qpush_class_32k,
                            &h8g.medium_remote_qpush_class_64k);
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
  H8_DEBUG_INC(medium_remote_publish_count);
  h8_medium_debug_class_inc(run->class_id,
                            &h8g.medium_remote_publish_class_8k,
                            &h8g.medium_remote_publish_class_16k,
                            &h8g.medium_remote_publish_class_32k,
                            &h8g.medium_remote_publish_class_64k);
  uint64_t owner_raw =
      atomic_load_explicit(&run->owner_word, memory_order_acquire);
  H8OwnerWord ow = h8_owner_word_unpack(owner_raw);
  H8OwnerRecord* owner = h8_owner_by_slot(ow.slot);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t lease_start = h8_medium_remote_now_ns();
#endif
  if (!owner || owner->permanent ||
      ow.state != H8_SPAN_OWNED_ACTIVE ||
      !h8_owner_publish_enter(owner, ow.generation)) {
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  H8_DEBUG_ADD(medium_remote_owner_lease_ns,
               (size_t)(h8_medium_remote_now_ns() - lease_start));

  H8PublishResult result = H8_PUBLISH_INVALID;
  bool notify = false;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t lock_start = h8_medium_remote_now_ns();
#endif
  pthread_mutex_lock(&run->lock);
  H8_DEBUG_ADD(medium_remote_run_lock_ns,
               (size_t)(h8_medium_remote_now_ns() - lock_start));
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

  H8_DEBUG_INC(medium_remote_pending_claim_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t claim_start = h8_medium_remote_now_ns();
#endif
  uint64_t old_word =
      atomic_fetch_or_explicit(&run->pending_bits[0], bit, memory_order_acq_rel);
  H8_DEBUG_ADD(medium_remote_pending_claim_ns,
               (size_t)(h8_medium_remote_now_ns() - claim_start));
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
    H8_DEBUG_INC(medium_remote_notify_count);
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
  size_t collected = 0;
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
      ++collected;
    } else {
      H8_DEBUG_INC(invalid_count);
      atomic_fetch_and_explicit(&run->pending_bits[0], ~bit,
                                memory_order_acq_rel);
    }
    collect &= collect - 1u;
  }
  H8_DEBUG_ADD(medium_remote_collect_slot_count, collected);
  H8_DEBUG_ADD(medium_remote_collect_run_count, 1);
  h8_medium_debug_class_inc(run->class_id,
                            &h8g.medium_remote_collect_run_class_8k,
                            &h8g.medium_remote_collect_run_class_16k,
                            &h8g.medium_remote_collect_run_class_32k,
                            &h8g.medium_remote_collect_run_class_64k);
  h8_medium_debug_class_add(run->class_id,
                            &h8g.medium_remote_collect_slot_class_8k,
                            &h8g.medium_remote_collect_slot_class_16k,
                            &h8g.medium_remote_collect_slot_class_32k,
                            &h8g.medium_remote_collect_slot_class_64k,
                            collected);
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

static H8MediumRun* h8_medium_pending_pop_one(H8OwnerRecord* owner) {
  pthread_mutex_lock(&owner->pending_lock);
  if (!owner->medium_pending_carry && owner->medium_pending_head) {
    owner->medium_pending_carry = owner->medium_pending_head;
    owner->medium_pending_head = NULL;
  }
  H8MediumRun* run = owner->medium_pending_carry;
  if (run) {
    owner->medium_pending_carry = run->next_pending;
    run->next_pending = NULL;
  }
  pthread_mutex_unlock(&owner->pending_lock);
  return run;
}

size_t h8_medium_collect_owner_pending_budget(H8OwnerRecord* owner,
                                              size_t run_budget) {
  if (!owner) {
    return 0;
  }
  H8_DEBUG_INC(medium_remote_collect_call_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t collect_start = h8_medium_remote_now_ns();
#endif
  size_t processed = 0;
  if (atomic_load_explicit(&owner->medium_pending_count,
                           memory_order_acquire) == 0) {
    goto done;
  }
  while (processed < run_budget) {
    H8MediumRun* run = h8_medium_pending_pop_one(owner);
    if (!run) {
      break;
    }
    atomic_fetch_sub_explicit(&owner->medium_pending_count, 1,
                              memory_order_acq_rel);
    ++processed;
    if (h8_medium_collect_run(owner, run)) {
      h8_medium_pending_queue_push(owner, run);
    }
  }
done:
  H8_DEBUG_ADD(medium_remote_collect_ns,
               (size_t)(h8_medium_remote_now_ns() - collect_start));
  return processed;
}

void h8_medium_collect_owner_pending_periodic(H8ThreadCtx* ctx) {
  if (!ctx || !ctx->owner) {
    return;
  }
  if (ctx->medium_collect_credit > 1u) {
    --ctx->medium_collect_credit;
    return;
  }
  ctx->medium_collect_credit = H8_MEDIUM_COLLECT_PERIOD;
  if (atomic_load_explicit(&ctx->owner->medium_pending_count,
                           memory_order_acquire) != 0) {
    (void)h8_medium_collect_owner_pending_budget(
        ctx->owner, H8_MEDIUM_COLLECT_BUDGET);
  }
}

void h8_medium_collect_owner_pending(H8OwnerRecord* owner) {
  (void)h8_medium_collect_owner_pending_budget(owner, SIZE_MAX);
}
