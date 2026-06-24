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

#if !defined(H8_MEDIUM_COLLECT_PERIOD)
#define H8_MEDIUM_COLLECT_PERIOD 32u
#endif

#if !defined(H8_MEDIUM_COLLECT_BUDGET)
#define H8_MEDIUM_COLLECT_BUDGET 8u
#endif

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

#if defined(H8_ENABLE_DEBUG_STATS)
void h8_medium_debug_writer_enter(H8MediumRun* run, H8OwnerRecord* owner,
                                  H8MediumWriterKind kind) {
  if (!run) {
    return;
  }
  uint64_t current =
      atomic_load_explicit(&run->owner_word, memory_order_acquire);
  uint64_t expected = h8_medium_owner_word_for(owner);
  bool attached = current != 0;
  if (attached &&
      (kind == H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC ||
       kind == H8_MEDIUM_WRITER_OWNER_LOCAL_FREE) &&
      current != expected) {
    H8_DEBUG_INC(medium_attached_foreign_mask_writer);
  }
  if (attached && kind == H8_MEDIUM_WRITER_OWNER_COLLECT &&
      current != expected) {
    H8_DEBUG_INC(medium_collect_wrong_owner);
  }
  if (attached && kind == H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE) {
    H8_DEBUG_INC(medium_detached_direct_free_while_attached);
  }
  unsigned old = atomic_fetch_add_explicit(&run->debug_writer_active, 1u,
                                           memory_order_acq_rel);
  if (old != 0 && attached) {
    H8_DEBUG_INC(medium_attached_writer_overlap);
  }
  if (old == 0) {
    atomic_store_explicit(&run->debug_writer_kind, (unsigned)kind,
                          memory_order_release);
    atomic_store_explicit(&run->debug_writer_token, current,
                          memory_order_release);
  }
}

void h8_medium_debug_writer_exit(H8MediumRun* run) {
  if (!run) {
    return;
  }
  unsigned old = atomic_fetch_sub_explicit(&run->debug_writer_active, 1u,
                                           memory_order_acq_rel);
  if (old == 1u) {
    unsigned kind =
        atomic_load_explicit(&run->debug_writer_kind, memory_order_acquire);
    uint64_t token =
        atomic_load_explicit(&run->debug_writer_token, memory_order_acquire);
    uint64_t current =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    if (kind != H8_MEDIUM_WRITER_OWNER_DETACH && token != 0 &&
        current != token) {
      H8_DEBUG_INC(medium_owner_token_changed_during_mutation);
    }
    atomic_store_explicit(&run->debug_writer_kind, 0, memory_order_release);
    atomic_store_explicit(&run->debug_writer_token, 0, memory_order_release);
  }
}
#endif

static void h8_medium_pending_queue_push(H8OwnerRecord* owner,
                                         H8MediumRun* run) {
  H8_DEBUG_INC(medium_remote_queue_push_count);
  H8_DEBUG_INC(medium_remote_queue_push_attempt_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t push_start = h8_medium_remote_now_ns();
#endif
  H8MediumRun* head =
      atomic_load_explicit(&owner->medium_pending_head, memory_order_relaxed);
  while (true) {
    run->next_pending = head;
    if (atomic_compare_exchange_weak_explicit(&owner->medium_pending_head,
                                              &head, run,
                                              memory_order_release,
                                              memory_order_relaxed)) {
      break;
    }
    H8_DEBUG_INC(medium_remote_queue_push_retry_count);
  }
  H8_DEBUG_INC(medium_remote_queue_push_success_count);
  atomic_fetch_add_explicit(&owner->medium_pending_count, 1,
                            memory_order_relaxed);
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

static void h8_medium_mark_dirty_if_draining(H8MediumRun* run) {
  uint8_t expected = H8_Q_DRAINING;
  if (atomic_compare_exchange_strong_explicit(&run->qstate, &expected,
                                              H8_Q_DRAINING_DIRTY,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    H8_DEBUG_INC(qstate_dirty_self_set);
  }
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
  if (!owner || owner->permanent || ow.state != H8_SPAN_OWNED_ACTIVE) {
    return H8_PUBLISH_OWNER_TRANSITION;
  }
  bool lease_entered = false;
  if (h8_remote_lease_elision_enabled()) {
    /*
     * Unsafe evidence mode: skipping the owner lifecycle lease removes the
     * owner-exit/reuse protection and must not be used for correctness claims.
     */
    H8_DEBUG_INC(remote_stage_regular_lease_elided);
  } else {
#if defined(H8_ENABLE_DEBUG_STATS)
    uint64_t lease_start = h8_medium_remote_now_ns();
    bool shadow_entered =
        h8_medium_owner_lease_shadow_enter(owner, ow.generation);
    bool actual_entered = h8_owner_publish_enter(owner, ow.generation);
    if (shadow_entered != actual_entered) {
      H8_DEBUG_INC(medium_lease_enter_decision_mismatch);
    }
    if (!actual_entered) {
      if (shadow_entered) {
        h8_medium_owner_lease_shadow_exit(owner);
      }
      return H8_PUBLISH_OWNER_TRANSITION;
    }
#else
    if (!h8_owner_publish_enter(owner, ow.generation)) {
      return H8_PUBLISH_OWNER_TRANSITION;
    }
#endif
    lease_entered = true;
#if defined(H8_ENABLE_DEBUG_STATS)
    size_t enter_ns = (size_t)(h8_medium_remote_now_ns() - lease_start);
    H8_DEBUG_ADD(medium_remote_owner_lease_enter_ns, enter_ns);
    H8_DEBUG_ADD(medium_remote_owner_lease_ns, enter_ns);
#endif
  }

  H8PublishResult result = H8_PUBLISH_INVALID;
  bool notify = false;
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

  H8_DEBUG_INC(medium_remote_lockless_claim_count);
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
    bool draining = h8_medium_claim_accepted_by_collector(run);
    uint64_t rollback = atomic_fetch_and_explicit(
        &run->pending_bits[0], ~bit, memory_order_acq_rel);
    if ((rollback & bit) == 0) {
      H8_DEBUG_INC(medium_remote_lockless_claim_rollback_accepted);
      result = H8_PUBLISH_OK;
      goto out;
    }
    if (draining) {
      H8_DEBUG_INC(medium_remote_lockless_claim_collector_accept);
      result = H8_PUBLISH_OK;
      goto out;
    }
    H8_DEBUG_INC(medium_remote_lockless_claim_rollback_invalid);
    result = H8_PUBLISH_INVALID;
    goto out;
  }

  notify = old_word == 0;
  result = H8_PUBLISH_OK;

out:
  if (notify) {
    H8_DEBUG_INC(medium_remote_notify_count);
    h8_medium_signal_work(owner, run);
  }
  if (lease_entered) {
#if defined(H8_ENABLE_DEBUG_STATS)
    uint64_t lease_exit_start = h8_medium_remote_now_ns();
    h8_medium_owner_lease_shadow_exit(owner);
#endif
    h8_owner_publish_exit(owner);
#if defined(H8_ENABLE_DEBUG_STATS)
    size_t exit_ns = (size_t)(h8_medium_remote_now_ns() - lease_exit_start);
    H8_DEBUG_ADD(medium_remote_owner_lease_exit_ns, exit_ns);
    H8_DEBUG_ADD(medium_remote_owner_lease_ns, exit_ns);
#endif
  }
  return result;
}

static bool h8_medium_collect_active_keep_shadow(H8OwnerRecord* owner,
                                                 H8MediumRun* run,
                                                 H8ThreadCtx* ctx,
                                                 uint64_t remaining) {
  if (!run || run->allocated_mask != 0 || remaining != 0) {
    return false;
  }
  if (!ctx) {
    H8_DEBUG_INC(medium_collect_ctx_missing);
    return false;
  }
  if (ctx != h8_tls_ctx || ctx->owner != owner) {
    H8_DEBUG_INC(medium_collect_ctx_owner_mismatch);
    return false;
  }
  if (run->class_id >= H8_MEDIUM_CLASS_COUNT ||
      ctx->active_medium_runs[run->class_id] != run) {
    if (run->active_live_empty_charge) {
      H8_DEBUG_INC(medium_empty_live_not_current_active);
    }
    H8_DEBUG_INC(medium_collect_active_hint_mismatch);
    return false;
  }
  if (!h8_medium_run_owned_by_ctx(run, ctx)) {
    H8_DEBUG_INC(medium_collect_active_not_owned);
    return false;
  }
  H8_DEBUG_INC(medium_collect_active_would_keep);
  return true;
}

static bool h8_medium_collect_run(H8OwnerRecord* owner, H8MediumRun* run,
                                  H8ThreadCtx* ctx) {
  uint8_t expected = H8_Q_QUEUED;
  if (!atomic_compare_exchange_strong_explicit(&run->qstate, &expected,
                                               H8_Q_DRAINING,
                                               memory_order_acq_rel,
                                               memory_order_acquire)) {
    return false;
  }

  h8_medium_debug_writer_enter(run, owner, H8_MEDIUM_WRITER_OWNER_COLLECT);
  pthread_mutex_lock(&run->lock);
  uint64_t bits = atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire);
  uint64_t valid = run->slot_count == 64u
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  bits &= valid;
  uint64_t collect = bits;
  uint64_t accepted = 0;
  size_t collected = 0;
  size_t rejected = 0;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t section_start = h8_medium_remote_now_ns();
#endif
  while (collect) {
    uint64_t bit = collect & (~collect + 1u);
    size_t slot = (size_t)__builtin_ctzll(bit);
    uint32_t state =
        atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
    if (h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
      atomic_store_explicit(&run->slot_state[slot],
                            H8_SLOT_FREE | H8_SLOT_NONE,
                            memory_order_release);
      accepted |= bit;
      ++collected;
    } else {
      H8_DEBUG_INC(invalid_count);
      ++rejected;
    }
    collect &= collect - 1u;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_collect_state_ns,
               (size_t)(h8_medium_remote_now_ns() - section_start));
#endif
  if (bits) {
#if defined(H8_ENABLE_DEBUG_STATS)
    section_start = h8_medium_remote_now_ns();
#endif
    run->allocated_mask &= ~accepted;
    run->free_mask |= accepted;
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_collect_mask_ns,
                 (size_t)(h8_medium_remote_now_ns() - section_start));
    section_start = h8_medium_remote_now_ns();
#endif
    atomic_fetch_and_explicit(&run->pending_bits[0], ~bits,
                              memory_order_acq_rel);
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_collect_pending_clear_ns,
                 (size_t)(h8_medium_remote_now_ns() - section_start));
#endif
  }
  uint64_t remaining =
      atomic_load_explicit(&run->pending_bits[0], memory_order_acquire) & valid;
  if (remaining) {
    h8_medium_mark_dirty_if_draining(run);
  }
  H8_DEBUG_ADD(medium_remote_collect_slot_count, collected);
  H8_DEBUG_ADD(medium_remote_collect_reject_count, rejected);
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
#if defined(H8_ENABLE_DEBUG_STATS)
  section_start = h8_medium_remote_now_ns();
#endif
  bool would_keep_live =
      h8_medium_collect_active_keep_shadow(owner, run, ctx, remaining);
  (void)would_keep_live;
  if (run->allocated_mask == 0 && remaining == 0 && would_keep_live) {
    h8_medium_note_active_live_empty(run);
  } else if (run->allocated_mask == 0 && remaining == 0) {
    h8_medium_mark_empty_locked(run);
  } else if (run->allocated_mask == 0 && remaining != 0) {
    H8_DEBUG_INC(medium_empty_with_pending);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_collect_empty_ns,
               (size_t)(h8_medium_remote_now_ns() - section_start));
#endif
  pthread_mutex_unlock(&run->lock);
  h8_medium_debug_writer_exit(run);

  uint8_t expected_finish = H8_Q_DRAINING;
  if (atomic_compare_exchange_strong_explicit(&run->qstate, &expected_finish,
                                              H8_Q_IDLE, memory_order_acq_rel,
                                              memory_order_acquire)) {
    uint64_t pending =
        atomic_load_explicit(&run->pending_bits[0], memory_order_acquire);
    if ((pending & valid) != 0) {
      H8_DEBUG_INC(medium_collect_finish_pending_rearm);
      h8_medium_signal_work(owner, run);
    }
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
  if (!owner->medium_pending_carry) {
    owner->medium_pending_carry = atomic_exchange_explicit(
        &owner->medium_pending_head, NULL, memory_order_acq_rel);
  }
  H8MediumRun* run = owner->medium_pending_carry;
  if (run) {
    owner->medium_pending_carry = run->next_pending;
    run->next_pending = NULL;
  }
  pthread_mutex_unlock(&owner->pending_lock);
  return run;
}

static inline bool h8_medium_owner_has_pending_fast(H8OwnerRecord* owner) {
  return owner &&
         (atomic_load_explicit(&owner->medium_pending_count,
                               memory_order_acquire) != 0 ||
          owner->medium_pending_carry != NULL ||
          atomic_load_explicit(&owner->medium_pending_head,
                               memory_order_acquire) != NULL);
}

bool h8_medium_owner_has_pending(H8OwnerRecord* owner) {
  return h8_medium_owner_has_pending_fast(owner);
}

static size_t h8_medium_collect_pending_budget_ctx(H8OwnerRecord* owner,
                                                   H8ThreadCtx* ctx,
                                                   size_t run_budget) {
  if (!owner) {
    return 0;
  }
  H8_DEBUG_INC(medium_remote_collect_call_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t collect_start = h8_medium_remote_now_ns();
#endif
  size_t processed = 0;
  if (!h8_medium_owner_has_pending_fast(owner)) {
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
    if (h8_medium_collect_run(owner, run, ctx)) {
      h8_medium_pending_queue_push(owner, run);
    }
  }
done:
  H8_DEBUG_ADD(medium_remote_collect_ns,
               (size_t)(h8_medium_remote_now_ns() - collect_start));
  return processed;
}

size_t h8_medium_collect_current_pending_budget(H8ThreadCtx* ctx,
                                                size_t run_budget) {
  return h8_medium_collect_pending_budget_ctx(ctx ? ctx->owner : NULL, ctx,
                                             run_budget);
}

size_t h8_medium_collect_owner_pending_budget(H8OwnerRecord* owner,
                                              size_t run_budget) {
  return h8_medium_collect_pending_budget_ctx(owner, NULL, run_budget);
}

static void h8_medium_collect_owner_pending_periodic_impl(H8ThreadCtx* ctx,
                                                          bool owner_list) {
  if (!ctx || !ctx->owner) {
    return;
  }
  if (owner_list) {
    H8_DEBUG_INC(medium_collect_periodic_from_owner_list);
  } else {
    H8_DEBUG_INC(medium_collect_periodic_from_active);
  }
  if (ctx->medium_collect_credit > 1u) {
    --ctx->medium_collect_credit;
    H8_DEBUG_INC(medium_collect_periodic_fast_skip);
    return;
  }
  H8_DEBUG_INC(medium_collect_periodic_slow_enter);
  ctx->medium_collect_credit = H8_MEDIUM_COLLECT_PERIOD;
  if (h8_medium_owner_has_pending_fast(ctx->owner)) {
    H8_DEBUG_INC(medium_collect_periodic_pending_hit);
    (void)h8_medium_collect_current_pending_budget(ctx,
                                                   H8_MEDIUM_COLLECT_BUDGET);
  } else {
    H8_DEBUG_INC(medium_collect_periodic_pending_miss);
  }
}

void h8_medium_collect_owner_pending_periodic(H8ThreadCtx* ctx) {
  h8_medium_collect_owner_pending_periodic_impl(ctx, false);
}

void h8_medium_collect_owner_pending_periodic_owner_list(H8ThreadCtx* ctx) {
  h8_medium_collect_owner_pending_periodic_impl(ctx, true);
}

void h8_medium_collect_owner_pending(H8OwnerRecord* owner) {
  (void)h8_medium_collect_owner_pending_budget(owner, SIZE_MAX);
}
