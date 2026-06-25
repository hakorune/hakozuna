#include "h8_internal.h"
#include "h8_medium.h"

#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

static const H8MediumClassSpec k_h8_medium_classes[H8_MEDIUM_CLASS_COUNT] = {
    {8192u, H8_MEDIUM_RUN_BYTES, 13u, 8u, 1u},
    {16384u, H8_MEDIUM_RUN_BYTES, 14u, 4u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 15u, 2u, 1u},
#if defined(H8_MEDIUM_UPPER48_CLASS)
    {49152u, H8_MEDIUM_RUN_BYTES, 14u, 1u, 1u},
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#else
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#endif
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
#if defined(H8_MEDIUM_UPPER48_CLASS)
_Static_assert(H8_MEDIUM_CLASS_COUNT == 5u,
               "upper48 medium candidate expects five classes");
#else
_Static_assert(H8_MEDIUM_CLASS_COUNT == 4u,
               "default medium scaffold expects four classes");
#endif
_Static_assert((H8_MEDIUM_64K_RUN_BYTES % H8_MEDIUM_QUANTUM_BYTES) == 0u,
               "medium run size must be quantum-aligned");

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_medium_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void h8_medium_debug_class_inc(uint32_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k) {
  atomic_size_t* target = NULL;
  if (class_id == 0u) {
    target = class_8k;
  } else if (class_id == 1u) {
    target = class_16k;
  } else if (class_id == 2u) {
    target = class_32k;
  } else if (class_id + 1u == H8_MEDIUM_CLASS_COUNT) {
    target = class_64k;
  }
  if (target) {
    atomic_fetch_add_explicit(target, 1u, memory_order_relaxed);
  }
}
#else
static void h8_medium_debug_class_inc(uint32_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k) {
  (void)class_id;
  (void)class_8k;
  (void)class_16k;
  (void)class_32k;
  (void)class_64k;
}
#endif

static void h8_medium_lock_run(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  pthread_mutex_lock(&run->lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_run_lock_wait_ns,
               (size_t)(h8_medium_now_ns() - start));
#endif
}

static void h8_medium_unlock_run(H8MediumRun* run) {
  pthread_mutex_unlock(&run->lock);
}

static uint64_t h8_medium_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner || owner->slot >= H8_OWNER_MAX) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

static bool h8_medium_owner_word_matches_ctx(uint64_t owner_word,
                                             const H8ThreadCtx* ctx) {
  if (!ctx || !ctx->owner) {
    return false;
  }
  uint64_t expected = h8_medium_owner_word_for(ctx->owner);
  return expected != 0 && owner_word == expected;
}

static void h8_medium_debug_lock_elide_candidate(const H8MediumRun* run,
                                                 const H8ThreadCtx* ctx,
                                                 bool is_free) {
#if defined(H8_ENABLE_DEBUG_STATS)
  if (!run || !ctx || !ctx->owner) {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
    return;
  }
  if (h8_medium_run_owned_by_ctx(run, ctx)) {
    if (is_free) {
      H8_DEBUG_INC(medium_lock_elide_free_candidate);
    } else {
      H8_DEBUG_INC(medium_lock_elide_alloc_candidate);
    }
  } else {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
  }
#else
  (void)run;
  (void)ctx;
  (void)is_free;
#endif
}

static void h8_medium_debug_lock_elide_candidate_known(bool same_owner,
                                                       bool is_free) {
#if defined(H8_ENABLE_DEBUG_STATS)
  if (same_owner) {
    if (is_free) {
      H8_DEBUG_INC(medium_lock_elide_free_candidate);
    } else {
      H8_DEBUG_INC(medium_lock_elide_alloc_candidate);
    }
  } else {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
  }
#else
  (void)same_owner;
  (void)is_free;
#endif
}

bool h8_medium_size_supported(size_t size) {
  return h8_medium_size_supported_fast(size);
}

uint32_t h8_medium_class_for_size(size_t size) {
  return h8_medium_class_for_size_fast(size);
}

const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return &k_h8_medium_classes[class_id];
}

uint32_t h8_medium_rounded_size(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return 0u;
  }
  return k_h8_medium_classes[h8_medium_class_for_size(size)].slot_size;
}

static void h8_medium_owner_add_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (!ctx || !ctx->owner || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  h8_medium_detached_remove_locked(run);
  run->owner_attached = true;
  atomic_store_explicit(&run->owner_word,
                        h8_medium_owner_word_for(ctx->owner),
                        memory_order_release);
  run->next_owner = ctx->owner->medium_by_class[run->class_id];
  ctx->owner->medium_by_class[run->class_id] = run;
}

static bool h8_medium_run_usable_locked(const H8MediumRun* run,
                                        uint32_t class_id) {
  return run && run->class_id == class_id && run->free_mask != 0 &&
         atomic_load_explicit(&run->state, memory_order_acquire) ==
             H8_MEDIUM_RUN_ACTIVE;
}

static void h8_medium_demote_active_empty_live(H8MediumRun* run) {
  if (!run || pthread_mutex_trylock(&run->lock) != 0) {
    return;
  }
  uint64_t pending =
      atomic_load_explicit(&run->pending_bits[0], memory_order_acquire);
  if (run->allocated_mask == 0 && pending == 0 &&
      run->payload_state == H8_MEDIUM_PAYLOAD_LIVE) {
    h8_medium_mark_empty_locked(run);
  }
  h8_medium_unlock_run(run);
}

static void h8_medium_set_active_run(H8ThreadCtx* ctx, uint32_t class_id,
                                     H8MediumRun* run) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* old = ctx->active_medium_runs[class_id];
  if (old != run && h8_medium_run_owned_by_ctx(old, ctx)) {
    h8_medium_demote_active_empty_live(old);
  }
  ctx->active_medium_runs[class_id] = run;
}

#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
static void h8_medium_record_alloc_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (ctx) {
    ctx->medium_last_alloc_run = run;
  }
}
#else
#define h8_medium_record_alloc_run(ctx, run) \
  do {                                      \
    (void)(ctx);                            \
    (void)(run);                            \
  } while (0)
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
  pthread_mutex_init(&run->lock, NULL);
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
  h8_medium_lock_global();
  h8_medium_unregister_locked(run);
  h8_medium_unlock_global();
  if (run->base) {
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
  pthread_mutex_destroy(&run->lock);
  h8_sys_free(run);
}

static void* h8_medium_run_alloc_active_hit(H8MediumRun* run,
                                            uint32_t class_id) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  if (!run || run->class_id != class_id) {
    return NULL;
  }
  if (run->free_mask == 0) {
    H8_DEBUG_INC(medium_alloc_free_mask_zero);
    return NULL;
  }
  if (atomic_load_explicit(&run->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(medium_alloc_state_check_fail);
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
  h8_medium_mark_live_on_alloc_fast(run);
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_alloc_slot_count);
  H8_DEBUG_ADD(medium_alloc_slot_ns, (size_t)(h8_medium_now_ns() - start));
#endif
  return h8_medium_slot_ptr_known(run, slot);
}

void* h8_medium_malloc_class_inner(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8_DEBUG_INC(medium_malloc_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_malloc_class_8k,
                            &h8g.medium_malloc_class_16k,
                            &h8g.medium_malloc_class_32k,
                            &h8g.medium_malloc_class_64k);
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  bool did_capacity_collect = false;
retry_owner_capacity:
  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  uint64_t expected_owner_word =
      (ctx && ctx->owner) ? h8_medium_owner_word_for(ctx->owner) : 0;
  bool active_owned = false;
  if (!active) {
    H8_DEBUG_INC(medium_active_miss_null);
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
    }
  }
  if (active) {
    if (active_owned) {
      h8_medium_debug_lock_elide_candidate_known(true, false);
      h8_medium_debug_writer_enter(active, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      void* ptr = h8_medium_run_alloc_active_hit(active, class_id);
      h8_medium_debug_writer_exit(active);
      if (ptr) {
        h8_medium_record_alloc_run(ctx, active);
        H8_DEBUG_INC(medium_run_reuse_active_count);
        h8_medium_debug_class_inc(class_id,
                                  &h8g.medium_run_reuse_active_class_8k,
                                  &h8g.medium_run_reuse_active_class_16k,
                                  &h8g.medium_run_reuse_active_class_32k,
                                  &h8g.medium_run_reuse_active_class_64k);
        h8_medium_collect_owner_pending_periodic(ctx);
        return ptr;
      }
    }
    H8_DEBUG_INC(medium_active_miss_unusable);
  }
  if (ctx && ctx->owner) {
    H8_DEBUG_INC(medium_owner_scan_count);
    for (H8MediumRun* run = ctx->owner->medium_by_class[class_id]; run;
         run = run->next_owner) {
      H8_DEBUG_INC(medium_owner_scan_step_count);
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
        H8_DEBUG_INC(medium_run_reuse_owner_list_count);
        h8_medium_debug_class_inc(class_id,
                                  &h8g.medium_run_reuse_owner_class_8k,
                                  &h8g.medium_run_reuse_owner_class_16k,
                                  &h8g.medium_run_reuse_owner_class_32k,
                                  &h8g.medium_run_reuse_owner_class_64k);
        void* ptr = h8_medium_run_alloc_local_scaffold(run);
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
      void* ptr = h8_medium_run_alloc_local_scaffold(run);
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
  void* ptr = h8_medium_run_alloc_local_scaffold(run);
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

bool h8_medium_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H8ThreadCtx* ctx = h8_tls_ctx;
#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
  if (h8_medium_try_cached_local_free(ctx, ptr, owned_out)) {
    return true;
  }
#else
  (void)ctx;
#endif

  H8_DEBUG_INC(medium_free_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    uint64_t owner_word =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    bool same_owner = h8_medium_owner_word_matches_ctx(owner_word, ctx);
    if (!same_owner && owner_word != 0) {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        sched_yield();
      }
    }
    h8_medium_debug_lock_elide_candidate_known(same_owner, true);
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_FREE);
      if (owned_out) {
        *owned_out = true;
      }
      bool keep_empty_live =
          ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
          ctx->active_medium_runs[run->class_id] == run;
      bool ok = h8_medium_run_free_local_scaffold(run, ptr, keep_empty_live);
      if (!ok) {
        H8_DEBUG_INC(medium_invalid_owned_count);
      }
      if (ok) {
        h8_medium_debug_class_inc(run->class_id,
                                  &h8g.medium_local_free_class_8k,
                                  &h8g.medium_local_free_class_16k,
                                  &h8g.medium_local_free_class_32k,
                                  &h8g.medium_local_free_class_64k);
      }
      if (ok && ctx && run->class_id < H8_MEDIUM_CLASS_COUNT) {
        h8_medium_set_active_run(ctx, run->class_id, run);
      }
      h8_medium_debug_writer_exit(run);
      return ok;
    }
    H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                 H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    ctx = NULL;
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, false);
    if (!run) {
      h8_medium_unlock_global();
      return false;
    }
    uint64_t owner_word =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    bool same_owner = h8_medium_owner_word_matches_ctx(owner_word, ctx);
    if (!same_owner && owner_word != 0) {
      h8_medium_unlock_global();
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        sched_yield();
      }
    }
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
    } else {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    }
    h8_medium_debug_writer_enter(
        run, ctx ? ctx->owner : NULL,
        same_owner ? H8_MEDIUM_WRITER_OWNER_LOCAL_FREE
                   : H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
    if (!same_owner) {
      ctx = NULL;
    }
  }
  if (owned_out) {
    *owned_out = true;
  }
  bool keep_empty_live =
      ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
      ctx->active_medium_runs[run->class_id] == run;
  bool ok = h8_medium_run_free_local_scaffold(run, ptr, keep_empty_live);
  if (!ok) {
    H8_DEBUG_INC(medium_invalid_owned_count);
  }
  if (ok) {
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
        ctx->active_medium_runs[run->class_id] == run) {
      h8_medium_debug_class_inc(run->class_id,
                                &h8g.medium_local_free_class_8k,
                                &h8g.medium_local_free_class_16k,
                                &h8g.medium_local_free_class_32k,
                                &h8g.medium_local_free_class_64k);
      h8_medium_set_active_run(ctx, run->class_id, run);
    }
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  return ok;
}

H8RouteKind h8_medium_route_inner(void* ptr) {
  H8_DEBUG_INC(medium_route_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, true);
    if (!run) {
      h8_medium_unlock_global();
      return H8_ROUTE_MISS;
    }
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
  }
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    h8_medium_unlock_run(run);
    return H8_ROUTE_INVALID;
  }
  uint64_t bit = UINT64_C(1) << slot;
  bool pending = (atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire) &
                  bit) != 0;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  bool slot_authority_valid =
      h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) &&
      !pending;
#if defined(H8_ENABLE_DEBUG_STATS)
  bool mask_authority_valid =
      (run->allocated_mask & bit) != 0 && (run->free_mask & bit) == 0 &&
      !pending;
  if (slot_authority_valid != mask_authority_valid) {
    H8_DEBUG_INC(medium_route_authority_mismatch);
  }
#endif
  H8RouteKind route = slot_authority_valid ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
  h8_medium_unlock_run(run);
  return route;
}

void h8_medium_owner_detach_all(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  h8_medium_collect_owner_pending(owner);
  h8_medium_lock_global();
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H8MediumRun* run = owner->medium_by_class[c];
    owner->medium_by_class[c] = NULL;
    while (run) {
      H8MediumRun* next = run->next_owner;
      h8_medium_debug_writer_enter(run, owner, H8_MEDIUM_WRITER_OWNER_DETACH);
      h8_medium_lock_run(run);
      if (run->allocated_mask == 0 &&
          run->payload_state != H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED) {
        H8_DEBUG_INC(medium_owner_exit_drain_count);
        h8_medium_decommit_empty_owner_exit_locked(run);
        if (run->active_live_empty_charge) {
          H8_DEBUG_INC(medium_owner_exit_active_live_remaining);
        }
      }
      if (run->lazy_purge_charge) {
        H8_DEBUG_INC(medium_lazy_drop_detach);
      }
      h8_medium_lazy_purge_shadow_drop(run);
      run->owner_attached = false;
      atomic_store_explicit(&run->owner_word, 0, memory_order_release);
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
      run->next_owner = NULL;
      h8_medium_detached_add_locked(run);
      run = next;
    }
  }
  h8_medium_unlock_global();
}
