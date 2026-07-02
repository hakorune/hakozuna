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

#if defined(H8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1)
#if !defined(H8_MEDIUM_CAPACITY_COLLECT_BUDGET_VALUE)
#define H8_MEDIUM_CAPACITY_COLLECT_BUDGET_VALUE 16u
#endif
#else
#define H8_MEDIUM_CAPACITY_COLLECT_BUDGET_VALUE SIZE_MAX
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
