#if defined(H8_MEDIUM_ENABLE_AVAILABLE_INDEX)
static void* h8_medium_try_available_index(H8ThreadCtx* ctx,
                                           uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8MediumRun* run = ctx->owner->medium_available_shadow[class_id];
  if (!run) {
    return NULL;
  }
  if (ctx->active_medium_runs[class_id] == run) {
    H8_DEBUG_INC(medium_available_active_indexed);
    h8_medium_available_shadow_remove(ctx->owner, run);
    return NULL;
  }
  h8_medium_debug_writer_enter(run, ctx->owner,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t section_start = h8_medium_now_ns();
#endif
  h8_medium_lock_run(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_lock_ns,
               (size_t)(h8_medium_now_ns() - section_start));
#endif
  if (!h8_medium_run_owned_by_ctx(run, ctx) ||
      !h8_medium_run_usable_locked(run, class_id)) {
    h8_medium_unlock_run(run);
    h8_medium_debug_writer_exit(run);
    return NULL;
  }
  h8_medium_debug_lock_elide_candidate(run, ctx, false);
  H8_DEBUG_INC(medium_run_reuse_owner_list_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_run_reuse_owner_class_8k,
                            &h8g.medium_run_reuse_owner_class_16k,
                            &h8g.medium_run_reuse_owner_class_32k,
                            &h8g.medium_run_reuse_owner_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  h8_medium_debug_v12_class_inc(class_id, &h8g.medium_run_reuse_owner_class_24k,
                                &h8g.medium_run_reuse_owner_class_48k);
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  section_start = h8_medium_now_ns();
#endif
  void* ptr = h8_medium_run_alloc_local_hot(run);
  if (ptr) {
    h8_medium_debug_note_alloc_collect_credit(ctx->owner, run);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_alloc_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  section_start = h8_medium_now_ns();
#endif
  h8_medium_set_active_run(ctx, class_id, run);
  h8_medium_record_alloc_run(ctx, run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_active_ns,
               (size_t)(h8_medium_now_ns() - section_start));
#endif
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  section_start = h8_medium_now_ns();
#endif
  h8_medium_collect_owner_pending_periodic_owner_list(ctx);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_collect_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  H8_DEBUG_INC(medium_available_hit_reuse);
#endif
  return ptr;
}
#else
#define h8_medium_try_available_index(ctx, class_id) \
  ((void)(ctx), (void)(class_id), (void*)NULL)
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_medium_refill_candidate_shadow_probe(H8ThreadCtx* ctx,
                                                    uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* run = ctx->owner->medium_refill_candidate[class_id];
  if (!run) {
    return;
  }
  H8_DEBUG_INC(medium_refill_candidate_attempt);
  h8_medium_lock_run(run);
  bool owner_match = h8_medium_run_owned_by_ctx(run, ctx);
  bool usable = owner_match && h8_medium_run_usable_locked(run, class_id);
  h8_medium_unlock_run(run);
  if (!owner_match) {
    H8_DEBUG_INC(medium_refill_candidate_owner_mismatch);
  } else if (!usable) {
    H8_DEBUG_INC(medium_refill_candidate_unusable);
  } else {
    H8_DEBUG_INC(medium_refill_candidate_hit);
  }
}
#else
#define h8_medium_refill_candidate_shadow_probe(ctx, class_id) \
  do {                                                        \
    (void)(ctx);                                              \
    (void)(class_id);                                         \
  } while (0)
#endif

#if defined(H8_MEDIUM_ENABLE_REFILL_CANDIDATE)
static void* h8_medium_try_refill_candidate(H8ThreadCtx* ctx,
                                            uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8MediumRun* run = ctx->owner->medium_refill_candidate[class_id];
  if (!run) {
    return NULL;
  }
  h8_medium_debug_writer_enter(run, ctx->owner,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
  h8_medium_lock_run(run);
  if (!h8_medium_run_owned_by_ctx(run, ctx) ||
      !h8_medium_run_usable_locked(run, class_id)) {
    h8_medium_unlock_run(run);
    h8_medium_debug_writer_exit(run);
    return NULL;
  }
  h8_medium_debug_lock_elide_candidate(run, ctx, false);
  void* ptr = h8_medium_run_alloc_local_hot(run);
  if (ptr) {
    h8_medium_debug_note_alloc_collect_credit(ctx->owner, run);
  }
  h8_medium_set_active_run(ctx, class_id, run);
  h8_medium_record_alloc_run(ctx, run);
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  H8_DEBUG_INC(medium_run_reuse_refill_candidate_count);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  h8_medium_debug_v12_class_inc(class_id, &h8g.medium_run_reuse_owner_class_24k,
                                &h8g.medium_run_reuse_owner_class_48k);
#endif
  h8_medium_collect_owner_pending_periodic_owner_list(ctx);
  return ptr;
}
#else
#define h8_medium_try_refill_candidate(ctx, class_id) \
  ((void)(ctx), (void)(class_id), (void*)NULL)
#endif

#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
static bool h8_medium_try_cached_local_free(H8ThreadCtx* ctx, void* ptr,
                                            bool* owned_out) {
  H8MediumRun* cached = ctx ? ctx->medium_last_alloc_run : NULL;
  if (!cached) {
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_attempt);

  uintptr_t base = (uintptr_t)cached->base;
  uintptr_t addr = (uintptr_t)ptr;
  size_t payload = (size_t)cached->slot_size * (size_t)cached->slot_count;
  if (addr < base || addr >= base + payload) {
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_range_hit);

  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked_fast(cached, ptr, &slot)) {
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_slot_hit);

  uint64_t owner_word =
      atomic_load_explicit(&cached->owner_word, memory_order_acquire);
  if (!h8_medium_owner_word_matches_ctx(owner_word, ctx)) {
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_owner_hit);

  uint64_t bit = UINT64_C(1) << slot;
  if ((cached->allocated_mask & bit) == 0 || (cached->free_mask & bit) != 0) {
    H8_DEBUG_INC(medium_free_cache_state_block);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }

  uint64_t pending =
      atomic_load_explicit(&cached->pending_bits[0], memory_order_acquire);
  if ((pending & bit) != 0) {
    H8_DEBUG_INC(medium_free_cache_pending_block);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }

  uint32_t state =
      atomic_load_explicit(&cached->slot_state[slot], memory_order_acquire);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    H8_DEBUG_INC(medium_free_cache_state_block);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }

#if defined(H8_ENABLE_DEBUG_STATS)
  H8MediumRun* directory_run = h8_medium_directory_find(ptr);
  if (directory_run != cached) {
    H8_DEBUG_INC(medium_free_cache_directory_mismatch);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
#endif

  H8_DEBUG_INC(medium_free_cache_would_succeed);

  H8_DEBUG_INC(medium_local_free_owner_match);
  h8_medium_debug_lock_elide_candidate_known(true, true);
  h8_medium_debug_writer_enter(cached, ctx ? ctx->owner : NULL,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_FREE);
  if (owned_out) {
    *owned_out = true;
  }
  bool keep_empty_live = cached->class_id < H8_MEDIUM_CLASS_COUNT &&
                         ctx->active_medium_runs[cached->class_id] == cached;
  bool ok = h8_medium_run_free_local_scaffold(cached, ptr, keep_empty_live);
  if (!ok) {
    H8_DEBUG_INC(medium_invalid_owned_count);
  }
  if (ok && cached->class_id < H8_MEDIUM_CLASS_COUNT) {
    h8_medium_debug_class_inc(cached->class_id,
                              &h8g.medium_local_free_class_8k,
                              &h8g.medium_local_free_class_16k,
                              &h8g.medium_local_free_class_32k,
                              &h8g.medium_local_free_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
    h8_medium_debug_v12_class_inc(cached->class_id,
                                  &h8g.medium_local_free_class_24k,
                                  &h8g.medium_local_free_class_48k);
#endif
    h8_medium_set_active_run(ctx, cached->class_id, cached);
  }
  h8_medium_debug_writer_exit(cached);
  return ok;
}
#endif

H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec) {
    return NULL;
  }
  H8MediumRun* run = h8_sys_calloc(1, sizeof(*run));
  if (!run) {
    return NULL;
  }
  h8_platform_mutex_init(&run->lock);
  run->slot_state = h8_sys_calloc(spec->slot_count, sizeof(*run->slot_state));
  run->pending_bits = h8_sys_calloc(spec->bitmap_words, sizeof(*run->pending_bits));
  if (!run->slot_state || !run->pending_bits) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  bool chunk_backed = false;
  void* payload = h8_medium_payload_alloc(spec->run_size, &chunk_backed);
  if (!payload) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  run->base = payload;
  run->payload_chunk_backed = chunk_backed;
  run->class_id = (uint16_t)class_id;
  run->slot_size = spec->slot_size;
  run->slot_shift = spec->slot_shift;
  run->slot_count = spec->slot_count;
  run->run_size = spec->run_size;
  run->free_mask = (run->slot_count == 64u)
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  run->allocated_mask = 0;
  run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED;
  atomic_store_explicit(&run->state, H8_MEDIUM_RUN_ACTIVE,
                        memory_order_relaxed);
  atomic_store_explicit(&run->qstate, H8_Q_IDLE, memory_order_relaxed);
  atomic_store_explicit(&run->pending_word_mask, 0, memory_order_relaxed);
  for (uint16_t i = 0; i < run->slot_count; ++i) {
    atomic_store_explicit(&run->slot_state[i], H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_relaxed);
  }
  return run;
}

void h8_medium_run_destroy_scaffold(H8MediumRun* run) {
  if (!run) {
    return;
  }
  h8_medium_debug_discard_collect_credit(run);
  h8_medium_lock_global();
  h8_medium_unregister_locked(run);
  h8_medium_unlock_global();
  if (run->base) {
#if defined(H8_MEDIUM_ENABLE_LOCAL_FAST_TIER)
    h8_medium_local_fast_flush_locked(run);
#endif
    h8_medium_release_empty_payload(run);
    if (run->lazy_purge_charge) {
      H8_DEBUG_INC(medium_lazy_drop_destroy);
    }
    h8_medium_lazy_purge_shadow_drop(run);
    h8_medium_payload_free(run->base,
                           run->run_size ? run->run_size
                                         : H8_MEDIUM_RUN_BYTES,
                           run->payload_chunk_backed);
  }
  h8_sys_free(run->pending_bits);
  h8_sys_free(run->slot_state);
  h8_platform_mutex_destroy(&run->lock);
  h8_sys_free(run);
}

static void* h8_medium_run_alloc_active_hit(H8ThreadCtx* ctx,
                                            H8MediumRun* run,
                                            uint32_t class_id) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
  uint64_t section_start = start;
#endif
  (void)ctx;
  if (!run || run->class_id != class_id) {
    return NULL;
  }
  if (atomic_load_explicit(&run->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(medium_alloc_state_check_fail);
    return NULL;
  }
#if defined(H8_MEDIUM_ENABLE_LOCAL_FAST_TIER)
  if (run->local_fast_mask != 0u) {
    size_t fast_slot = 0;
    if (h8_medium_local_fast_take_active(run, &fast_slot)) {
#if defined(H8_HZ9_MEDIUM_LOCAL_MAG_SHADOW)
      h8_hz9_local_mag_shadow_note_alloc(run, fast_slot);
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
      H8_DEBUG_INC(medium_active_alloc_hit_count);
      H8_DEBUG_INC(medium_alloc_slot_count);
      H8_DEBUG_ADD(medium_alloc_slot_ns,
                   (size_t)(h8_medium_now_ns() - start));
#endif
      return h8_medium_slot_ptr_known(run, fast_slot);
    }
  }
#endif
  if (run->free_mask == 0) {
    H8_DEBUG_INC(medium_alloc_free_mask_zero);
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_active_alloc_precheck_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  section_start = h8_medium_now_ns();
#endif
  h8_medium_debug_note_local_fast_alloc(run);
  h8_medium_mark_live_on_alloc_fast(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_active_alloc_mark_live_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  section_start = h8_medium_now_ns();
#endif
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_active_alloc_mask_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  section_start = h8_medium_now_ns();
#endif
#if !defined(H8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE)
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#endif
#if defined(H8_HZ9_MEDIUM_LOCAL_MAG_SHADOW)
  h8_hz9_local_mag_shadow_note_alloc(run, slot);
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_active_alloc_slot_store_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  section_start = h8_medium_now_ns();
  void* ptr = h8_medium_slot_ptr_known(run, slot);
  H8_DEBUG_ADD(medium_active_alloc_ptr_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  H8_DEBUG_INC(medium_active_alloc_hit_count);
  H8_DEBUG_INC(medium_alloc_slot_count);
  H8_DEBUG_ADD(medium_alloc_slot_ns, (size_t)(h8_medium_now_ns() - start));
  return ptr;
#endif
  return h8_medium_slot_ptr_known(run, slot);
}

#if defined(H8_MEDIUM_ENABLE_ACTIVE_MISS_DEMAND_COLLECT_64K)
static void* h8_medium_try_active_miss_demand_collect64(H8ThreadCtx* ctx,
                                                        uint32_t class_id,
                                                        H8MediumRun* active) {
  if (!ctx || !ctx->owner || !active ||
      class_id + 1u != H8_MEDIUM_CLASS_COUNT ||
      !h8_medium_run_owned_by_ctx(active, ctx)) {
    return NULL;
  }
  if (atomic_load_explicit(&active->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    return NULL;
  }
  if (active->free_mask != 0) {
    return NULL;
  }
  uint64_t valid = active->slot_count == 64u
                       ? UINT64_MAX
                       : ((UINT64_C(1) << active->slot_count) - 1u);
  uint64_t pending =
      atomic_load_explicit(&active->pending_bits[0], memory_order_acquire) &
      valid;
  if (!pending) {
    return NULL;
  }
  uint8_t qstate = atomic_load_explicit(&active->qstate, memory_order_acquire);
  if (qstate != H8_Q_QUEUED) {
    H8_DEBUG_INC(medium_demand64_qstate_not_queued);
    return NULL;
  }
  if (!h8_medium_owner_has_pending(ctx->owner)) {
    return NULL;
  }

  H8_DEBUG_INC(medium_demand64_trigger);
  size_t processed_total = 0;
  for (unsigned i = 0; i < 2u; ++i) {
    size_t processed = h8_medium_collect_current_pending_budget_source(
        ctx, 1u, H8_MEDIUM_COLLECT_ACTIVE_MISS_DEMAND);
    processed_total += processed;
    if (processed == 0 || active->free_mask != 0) {
      break;
    }
  }
  if (processed_total == 0) {
    H8_DEBUG_INC(medium_demand64_processed_0);
  } else if (processed_total == 1u) {
    H8_DEBUG_INC(medium_demand64_processed_1);
  } else {
    H8_DEBUG_INC(medium_demand64_processed_2);
  }

  if (active->free_mask == 0) {
    H8_DEBUG_INC(medium_demand64_target_not_reached);
    H8_DEBUG_INC(medium_demand64_owner_list_fallback);
    return NULL;
  }

  H8_DEBUG_INC(medium_demand64_target_opened);
  H8_DEBUG_ADD(medium_demand64_active_slots_created,
               (size_t)__builtin_popcountll(active->free_mask & valid));
  h8_medium_debug_writer_enter(active, ctx->owner,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
  void* ptr = h8_medium_run_alloc_active_hit(ctx, active, class_id);
  h8_medium_debug_writer_exit(active);
  if (!ptr) {
    H8_DEBUG_INC(medium_demand64_owner_list_fallback);
    return NULL;
  }

  H8_DEBUG_INC(medium_demand64_retry_hit);
  H8_DEBUG_INC(medium_demand64_periodic_tick_replaced);
  h8_medium_debug_note_alloc_collect_credit(ctx->owner, active);
  h8_medium_record_alloc_run(ctx, active);
  H8_DEBUG_INC(medium_run_reuse_active_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_run_reuse_active_class_8k,
                            &h8g.medium_run_reuse_active_class_16k,
                            &h8g.medium_run_reuse_active_class_32k,
                            &h8g.medium_run_reuse_active_class_64k);
  return ptr;
}
#else
#define h8_medium_try_active_miss_demand_collect64(ctx, class_id, active) \
  ((void)(ctx), (void)(class_id), (void)(active), (void*)NULL)
#endif

void* h8_medium_malloc_class_inner(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8_DEBUG_INC(medium_malloc_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_malloc_class_8k,
                            &h8g.medium_malloc_class_16k,
                            &h8g.medium_malloc_class_32k,
                            &h8g.medium_malloc_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  h8_medium_debug_v12_class_inc(class_id, &h8g.medium_malloc_class_24k,
                                &h8g.medium_malloc_class_48k);
#endif
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  h8_medium_debug_note_owner_medium_alloc(ctx ? ctx->owner : NULL);
  bool did_capacity_collect = false;
  bool saw_active_miss = false;
retry_owner_capacity:
  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  uint64_t expected_owner_word =
      (ctx && ctx->owner) ? h8_medium_owner_word_for(ctx->owner) : 0;
  bool active_owned = false;
  if (!active) {
    H8_DEBUG_INC(medium_active_miss_null);
    saw_active_miss = true;
  } else {
    active_owned =
        expected_owner_word != 0 &&
        atomic_load_explicit(&active->owner_word, memory_order_acquire) ==
            expected_owner_word;
    if (!active_owned) {
      H8_DEBUG_INC(medium_active_alloc_owner_mismatch);
      H8_DEBUG_INC(medium_active_miss_owner);
      h8_medium_set_active_run(ctx, class_id, NULL);
      active = NULL;
      saw_active_miss = true;
    }
  }
  if (active) {
    if (active_owned) {
      h8_medium_debug_lock_elide_candidate_known(true, false);
      h8_medium_debug_writer_enter(active, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      void* ptr = h8_medium_run_alloc_active_hit(ctx, active, class_id);
      h8_medium_debug_writer_exit(active);
      if (ptr) {
        h8_medium_debug_note_alloc_collect_credit(ctx ? ctx->owner : NULL,
                                                  active);
        h8_medium_record_alloc_run(ctx, active);
        H8_DEBUG_INC(medium_run_reuse_active_count);
        h8_medium_debug_class_inc(class_id,
                                  &h8g.medium_run_reuse_active_class_8k,
                                  &h8g.medium_run_reuse_active_class_16k,
                                  &h8g.medium_run_reuse_active_class_32k,
                                  &h8g.medium_run_reuse_active_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
        h8_medium_debug_v12_class_inc(
            class_id, &h8g.medium_run_reuse_active_class_24k,
            &h8g.medium_run_reuse_active_class_48k);
#endif
        h8_medium_collect_owner_pending_periodic(ctx);
        return ptr;
      }
    }
    H8_DEBUG_INC(medium_active_miss_unusable);
    saw_active_miss = true;
    h8_medium_debug_note_active_miss_pending(ctx, class_id, active);
    void* demand_ptr =
        h8_medium_try_active_miss_demand_collect64(ctx, class_id, active);
    if (demand_ptr) {
      return demand_ptr;
    }
  }
  h8_medium_refill_candidate_shadow_probe(ctx, class_id);
  void* refill_ptr = h8_medium_try_refill_candidate(ctx, class_id);
  if (refill_ptr) {
    return refill_ptr;
  }
  h8_medium_available_shadow_probe(ctx, class_id);
  void* available_ptr = h8_medium_try_available_index(ctx, class_id);
  if (available_ptr) {
    return available_ptr;
  }
  if (ctx && ctx->owner) {
    H8_DEBUG_INC(medium_owner_scan_count);
    size_t owner_scan_position = 0;
    for (H8MediumRun* run = ctx->owner->medium_by_class[class_id]; run;
         run = run->next_owner) {
      H8_DEBUG_INC(medium_owner_scan_step_count);
      ++owner_scan_position;
      if (!h8_medium_run_owned_by_ctx(run, ctx)) {
        H8_DEBUG_INC(medium_owner_list_owner_mismatch);
        continue;
      }
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      h8_medium_lock_run(run);
      if (h8_medium_run_owned_by_ctx(run, ctx) &&
          h8_medium_run_usable_locked(run, class_id)) {
        h8_medium_debug_lock_elide_candidate(run, ctx, false);
        h8_medium_available_shadow_note_owner_hit(ctx, class_id);
        H8_DEBUG_INC(medium_run_reuse_owner_list_count);
        h8_medium_debug_class_inc(class_id,
                                  &h8g.medium_run_reuse_owner_class_8k,
                                  &h8g.medium_run_reuse_owner_class_16k,
                                  &h8g.medium_run_reuse_owner_class_32k,
                                  &h8g.medium_run_reuse_owner_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
        h8_medium_debug_v12_class_inc(
            class_id, &h8g.medium_run_reuse_owner_class_24k,
            &h8g.medium_run_reuse_owner_class_48k);
#endif
        void* ptr = h8_medium_run_alloc_local_hot(run);
        if (ptr) {
          h8_medium_debug_note_owner_list_hit_position(owner_scan_position);
          h8_medium_debug_note_owner_list_hit_class(class_id);
          h8_medium_debug_note_alloc_collect_credit(ctx->owner, run);
        }
        h8_medium_set_active_run(ctx, class_id, run);
        h8_medium_record_alloc_run(ctx, run);
        h8_medium_unlock_run(run);
        h8_medium_debug_writer_exit(run);
        h8_medium_collect_owner_pending_periodic_owner_list(ctx);
        return ptr;
      }
      if (!h8_medium_run_owned_by_ctx(run, ctx)) {
        H8_DEBUG_INC(medium_owner_list_owner_mismatch);
      }
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
    }
  }
  if (ctx && ctx->owner && !did_capacity_collect &&
      h8_medium_owner_has_pending(ctx->owner)) {
    did_capacity_collect = true;
    (void)h8_medium_collect_current_pending_budget(ctx, SIZE_MAX);
    goto retry_owner_capacity;
  }
  h8_medium_lock_global();
  H8_DEBUG_INC(medium_global_scan_count);
  for (H8MediumRun* run = h8_medium_detached_head_locked(class_id); run;
       run = run->next_detached) {
    H8_DEBUG_INC(medium_global_scan_step_count);
    h8_medium_lock_run(run);
    if (run->class_id != class_id || run->owner_attached ||
        atomic_load_explicit(&run->owner_word, memory_order_acquire) != 0) {
      H8_DEBUG_INC(medium_global_skip_foreign_attached);
      h8_medium_unlock_run(run);
      continue;
    }
    if (h8_medium_run_usable_locked(run, class_id)) {
      H8_DEBUG_INC(medium_run_reuse_global_count);
      if (saw_active_miss) {
        H8_DEBUG_INC(medium_active_miss_detached_reuse);
      }
      h8_medium_debug_class_inc(class_id,
                                &h8g.medium_run_reuse_global_class_8k,
                                &h8g.medium_run_reuse_global_class_16k,
                                &h8g.medium_run_reuse_global_class_32k,
                                &h8g.medium_run_reuse_global_class_64k);
      if (ctx && ctx->owner) {
        h8_medium_owner_add_run(ctx, run);
      }
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_GLOBAL_ATTACH);
      void* ptr = h8_medium_run_alloc_local_hot(run);
      h8_medium_debug_note_alloc_collect_credit(ctx ? ctx->owner : NULL, run);
      h8_medium_debug_writer_exit(run);
      if (ctx && h8_medium_run_owned_by_ctx(run, ctx)) {
        h8_medium_set_active_run(ctx, class_id, run);
        h8_medium_record_alloc_run(ctx, run);
      }
      h8_medium_unlock_run(run);
      h8_medium_unlock_global();
      return ptr;
    }
    h8_medium_unlock_run(run);
  }
  h8_medium_unlock_global();

  H8MediumRun* run = h8_medium_run_create_scaffold(class_id);
  if (!run) {
    return NULL;
  }
  H8_DEBUG_INC(medium_run_create_count);
  if (saw_active_miss) {
    H8_DEBUG_INC(medium_active_miss_create);
  }
  h8_medium_debug_class_inc(class_id, &h8g.medium_run_create_class_8k,
                            &h8g.medium_run_create_class_16k,
                            &h8g.medium_run_create_class_32k,
                            &h8g.medium_run_create_class_64k);
  h8_medium_lock_global();
  h8_medium_register_locked(run);
  h8_medium_owner_add_run(ctx, run);
  h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                               H8_MEDIUM_WRITER_GLOBAL_ATTACH);
  h8_medium_lock_run(run);
  void* ptr = h8_medium_run_alloc_local_hot(run);
  h8_medium_debug_note_alloc_collect_credit(ctx ? ctx->owner : NULL, run);
  if (ctx) {
    h8_medium_set_active_run(ctx, class_id, run);
    h8_medium_record_alloc_run(ctx, run);
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  h8_medium_unlock_global();
  return ptr;
}

void* h8_medium_malloc_inner(size_t size) {
  if (!h8_medium_size_supported_fast(size)) {
    return NULL;
  }
  return h8_medium_malloc_class_inner(h8_medium_class_for_size_fast(size));
}
