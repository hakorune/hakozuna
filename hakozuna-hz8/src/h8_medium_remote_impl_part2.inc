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
    uint64_t old_free_mask = run->free_mask;
    run->allocated_mask &= ~accepted;
    run->free_mask |= accepted;
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
        ctx->active_medium_runs[run->class_id] == run) {
      H8_DEBUG_INC(medium_collect_active_run);
    } else {
      H8_DEBUG_INC(medium_collect_nonactive_run);
      if (source == H8_MEDIUM_COLLECT_ACTIVE_MISS_DEMAND) {
        H8_DEBUG_INC(medium_demand64_nonactive_runs_processed);
      }
    }
    h8_medium_debug_note_collect_capacity(owner, run, accepted, old_free_mask,
                                          source);
    h8_medium_available_shadow_after_mask_change_ctx(run, ctx);
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
  h8_medium_refill_candidate_install(owner, run, accepted);
#if defined(H8_MEDIUM_ENABLE_COLLECT_ACTIVE_REFILL_HINT)
  h8_medium_collect_refill_active_hint(owner, run, ctx, accepted);
#else
  (void)owner;
  (void)ctx;
#endif
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
    h8_medium_note_active_live_empty_fast(run);
  } else if (run->allocated_mask == 0 && remaining == 0) {
    h8_medium_mark_empty_locked(run);
  } else if (run->allocated_mask == 0 && remaining != 0) {
    H8_DEBUG_INC(medium_empty_with_pending);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_collect_empty_ns,
               (size_t)(h8_medium_remote_now_ns() - section_start));
#endif
  h8_platform_mutex_unlock(&run->lock);
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
  h8_platform_mutex_lock(&owner->pending_lock);
  if (!owner->medium_pending_carry) {
    owner->medium_pending_carry = atomic_exchange_explicit(
        &owner->medium_pending_head, NULL, memory_order_acq_rel);
  }
  H8MediumRun* run = owner->medium_pending_carry;
  if (run) {
    owner->medium_pending_carry = run->next_pending;
    run->next_pending = NULL;
  }
  h8_platform_mutex_unlock(&owner->pending_lock);
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
                                                   size_t run_budget,
                                                   H8MediumCollectSource source) {
  if (!owner) {
    return 0;
  }
  H8_DEBUG_INC(medium_remote_collect_call_count);
  h8_medium_debug_note_collect_source_call(source);
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
    if (h8_medium_collect_run(owner, run, ctx, source)) {
      h8_medium_pending_queue_push(owner, run);
    }
  }
  if (processed == 0) {
    H8_DEBUG_INC(medium_collect_zero_run_call);
  }
done:
  H8_DEBUG_ADD(medium_remote_collect_ns,
               (size_t)(h8_medium_remote_now_ns() - collect_start));
  return processed;
}

size_t h8_medium_collect_current_pending_budget(H8ThreadCtx* ctx,
                                                size_t run_budget) {
  return h8_medium_collect_current_pending_budget_source(
      ctx, run_budget, H8_MEDIUM_COLLECT_CAPACITY_MISS);
}

size_t h8_medium_collect_current_pending_budget_source(
    H8ThreadCtx* ctx, size_t run_budget, H8MediumCollectSource source) {
  return h8_medium_collect_pending_budget_ctx(ctx ? ctx->owner : NULL, ctx,
                                             run_budget, source);
}

size_t h8_medium_collect_owner_pending_budget(H8OwnerRecord* owner,
                                              size_t run_budget) {
  return h8_medium_collect_pending_budget_ctx(owner, NULL, run_budget,
                                             H8_MEDIUM_COLLECT_OWNER_EXIT);
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
    (void)h8_medium_collect_pending_budget_ctx(
        ctx->owner, ctx, H8_MEDIUM_COLLECT_BUDGET,
        owner_list ? H8_MEDIUM_COLLECT_PERIODIC_OWNER_LIST
                   : H8_MEDIUM_COLLECT_PERIODIC_ACTIVE);
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
