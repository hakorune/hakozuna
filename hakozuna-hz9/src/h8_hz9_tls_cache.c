#include "h8_internal.h"
#include "h8_medium.h"

#if defined(H9_MEDIUM_TLS_OBJECT_CACHE)
#if defined(H9_MEDIUM_TLS_OBJECT_CACHE_REMOTE_CLASS_ADMISSION)
static atomic_bool
    h9_medium_cache_remote_seen[H8_MEDIUM_CLASS_COUNT];
#endif

static uint16_t h9_medium_cache_capacity(uint32_t class_id) {
  (void)class_id;
  return H9_MEDIUM_CACHE_DEPTH_MAX;
}

static H9MediumClassCache* h9_medium_cache_class(H8ThreadCtx* ctx,
                                                 uint32_t class_id) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H9MediumClassCache* cache = &ctx->h9_medium_cache.by_class[class_id];
  if (cache->capacity == 0u) {
    cache->capacity = h9_medium_cache_capacity(class_id);
  }
  return cache;
}

static bool h9_medium_cache_owner_matches(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (!ctx || !ctx->owner || !run || ctx->owner->slot >= H8_OWNER_MAX) {
    return false;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)ctx->owner->slot,
                                        (uint16_t)ctx->owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  uint64_t expected = h8_owner_word_pack(word);
  return atomic_load_explicit(&run->owner_word, memory_order_acquire) ==
         expected;
}

static void h9_medium_cache_note_empty_after_flush(H8ThreadCtx* ctx,
                                                   H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
      ctx->active_medium_runs[run->class_id] == run) {
    h8_medium_note_active_live_empty_fast(run);
  } else {
    h8_medium_mark_empty_locked(run);
  }
}

static void h9_medium_cache_flush_class(H8ThreadCtx* ctx, uint32_t class_id) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H9MediumClassCache* cache = &ctx->h9_medium_cache.by_class[class_id];
  for (uint16_t i = 0; i < cache->count; ++i) {
    H9MediumCacheEntry entry = cache->entries[i];
    H8MediumRun* run = entry.run;
    if (!run || entry.slot >= run->slot_count) {
      H8_DEBUG_INC(h9_medium_cache_state_mismatch);
      continue;
    }
    uint32_t state =
        atomic_load_explicit(&run->slot_state[entry.slot],
                             memory_order_acquire);
    if (!h8_slot_state_is_local_cache(state)) {
      H8_DEBUG_INC(h9_medium_cache_state_mismatch);
      continue;
    }
    uint64_t bit = UINT64_C(1) << entry.slot;
    atomic_store_explicit(&run->slot_state[entry.slot],
                          H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_release);
    run->allocated_mask &= ~bit;
    run->free_mask |= bit;
    h9_medium_cache_note_empty_after_flush(ctx, run);
    H8_DEBUG_INC(h9_medium_cache_flush_slots);
  }
  cache->count = 0;
  ctx->h9_medium_cache.cached_bytes -= cache->cached_bytes;
  cache->cached_bytes = 0;
}

void* h9_medium_cache_pop(H8ThreadCtx* ctx, uint32_t class_id) {
#if defined(H9_MEDIUM_TLS_OBJECT_CACHE_PENDING_GATE)
  if (ctx && ctx->owner && h8_medium_owner_has_pending(ctx->owner)) {
    h9_medium_cache_flush_all(ctx);
    return NULL;
  }
#endif
#if defined(H9_MEDIUM_TLS_OBJECT_CACHE_REMOTE_CLASS_ADMISSION)
  if (class_id < H8_MEDIUM_CLASS_COUNT &&
      atomic_load_explicit(&h9_medium_cache_remote_seen[class_id],
                           memory_order_acquire)) {
    h9_medium_cache_flush_class(ctx, class_id);
    H8_DEBUG_INC(h9_medium_cache_remote_pop_block);
    return NULL;
  }
#endif
  H9MediumClassCache* cache = h9_medium_cache_class(ctx, class_id);
  if (!cache || cache->count == 0u) {
    return NULL;
  }

  H9MediumCacheEntry entry = cache->entries[--cache->count];
  if (!entry.run || entry.class_id != class_id ||
      entry.slot >= entry.run->slot_count ||
      !h9_medium_cache_owner_matches(ctx, entry.run) ||
      atomic_load_explicit(&entry.run->state, memory_order_acquire) !=
          H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(h9_medium_cache_state_mismatch);
    return NULL;
  }
  uint32_t state =
      atomic_load_explicit(&entry.run->slot_state[entry.slot],
                           memory_order_acquire);
  if (!h8_slot_state_is_local_cache(state)) {
    H8_DEBUG_INC(h9_medium_cache_state_mismatch);
    return NULL;
  }

  atomic_store_explicit(&entry.run->slot_state[entry.slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
  size_t bytes = entry.run->slot_size;
  cache->cached_bytes -= bytes;
  ctx->h9_medium_cache.cached_bytes -= bytes;
  H8_DEBUG_INC(h9_medium_cache_pop_hit);
  return entry.ptr;
}

bool h9_medium_cache_try_push(H8ThreadCtx* ctx, H8MediumRun* run, void* ptr,
                              size_t slot) {
  if (!ctx || !run || !ptr || run->class_id >= H8_MEDIUM_CLASS_COUNT ||
      slot >= run->slot_count || !h9_medium_cache_owner_matches(ctx, run)) {
    return false;
  }
#if defined(H9_MEDIUM_TLS_OBJECT_CACHE_REMOTE_CLASS_ADMISSION)
  if (atomic_load_explicit(&h9_medium_cache_remote_seen[run->class_id],
                           memory_order_acquire)) {
    H8_DEBUG_INC(h9_medium_cache_remote_push_block);
    return false;
  }
#endif

  H9MediumClassCache* cache = h9_medium_cache_class(ctx, run->class_id);
  if (!cache || cache->count >= cache->capacity ||
      cache->count >= H9_MEDIUM_CACHE_DEPTH_MAX) {
    H8_DEBUG_INC(h9_medium_cache_push_full);
    return false;
  }

  uint64_t bit = UINT64_C(1) << slot;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  if (!h8_slot_state_is_allocated_tag(state) ||
      (run->allocated_mask & bit) == 0 || (run->free_mask & bit) != 0) {
    H8_DEBUG_INC(h9_medium_cache_state_mismatch);
    return false;
  }
  if ((atomic_load_explicit(&run->pending_bits[slot >> 6u],
                            memory_order_acquire) &
       (UINT64_C(1) << (slot & 63u))) != 0) {
    return false;
  }

  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_LOCAL_CACHE,
                        memory_order_release);
  cache->entries[cache->count++] = (H9MediumCacheEntry){
      .run = run,
      .ptr = ptr,
      .slot = (uint16_t)slot,
      .class_id = (uint16_t)run->class_id,
  };

  size_t bytes = run->slot_size;
  cache->cached_bytes += bytes;
  ctx->h9_medium_cache.cached_bytes += bytes;
  if (ctx->h9_medium_cache.cached_bytes >
      ctx->h9_medium_cache.cached_bytes_peak) {
    ctx->h9_medium_cache.cached_bytes_peak = ctx->h9_medium_cache.cached_bytes;
  }
  H8_DEBUG_INC(h9_medium_cache_push_hit);
  return true;
}

void h9_medium_cache_note_remote_free(H8ThreadCtx* ctx, H8MediumRun* run) {
#if defined(H9_MEDIUM_TLS_OBJECT_CACHE_REMOTE_CLASS_ADMISSION)
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  bool was_seen = atomic_exchange_explicit(
      &h9_medium_cache_remote_seen[run->class_id], true, memory_order_acq_rel);
  if (!was_seen) {
    H8_DEBUG_INC(h9_medium_cache_remote_mark);
  }
  h9_medium_cache_flush_class(ctx, run->class_id);
#else
  (void)ctx;
  (void)run;
#endif
}

void h9_medium_cache_flush_all(H8ThreadCtx* ctx) {
  if (!ctx) {
    return;
  }
  for (uint32_t class_id = 0; class_id < H8_MEDIUM_CLASS_COUNT; ++class_id) {
    h9_medium_cache_flush_class(ctx, class_id);
  }
  ctx->h9_medium_cache.cached_bytes = 0;
}

void h9_medium_cache_flush_owner(H8ThreadCtx* ctx, H8OwnerRecord* owner) {
  if (ctx && ctx->owner == owner) {
    h9_medium_cache_flush_all(ctx);
  }
}

void h9_medium_cache_flush_run_locked(H8MediumRun* run) {
  if (!run) {
    return;
  }
  for (size_t slot = 0; slot < run->slot_count; ++slot) {
    uint32_t state =
        atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
    if (!h8_slot_state_is_local_cache(state)) {
      continue;
    }
    uint64_t bit = UINT64_C(1) << slot;
    atomic_store_explicit(&run->slot_state[slot],
                          H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_release);
    run->allocated_mask &= ~bit;
    run->free_mask |= bit;
    h8_medium_mark_empty_locked(run);
    H8_DEBUG_INC(h9_medium_cache_flush_slots);
  }
}
#endif
