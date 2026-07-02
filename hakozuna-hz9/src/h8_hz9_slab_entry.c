#include "h8_internal.h"

#if defined(H9_SLAB_ENTRY_SPLIT_L1) && defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)

#if defined(H9_SLAB_CLASS_COVERAGE_L1) && !defined(H9_SLAB_CLASS_MIN_ID)
#define H9_SLAB_CLASS_MIN_ID 2u
#endif

#if defined(H9_SLAB_ENTRY_SIZE_BYPASS_L1)
static bool h9_slab_entry_size_maybe_candidate(size_t size) {
  if (size <= H8_MAX_SMALL_SIZE || size > H8_MEDIUM_MAX_SIZE) {
    return false;
  }
#if defined(H9_SLAB_CLASS_COVERAGE_L1)
#if H9_SLAB_CLASS_MIN_ID >= 5u
  return size > 49152u;
#elif H9_SLAB_CLASS_MIN_ID >= 4u
  return size > 32768u;
#elif H9_SLAB_CLASS_MIN_ID >= 3u
  return size > 24576u;
#elif H9_SLAB_CLASS_MIN_ID >= 2u
  return size > 16384u;
#elif H9_SLAB_CLASS_MIN_ID >= 1u
  return size > 8192u;
#else
  return true;
#endif
#else
  return size > 49152u;
#endif
}
#endif

static bool h9_slab_entry_candidate(size_t size, uint32_t class_id) {
#if defined(H9_SLAB_CLASS_COVERAGE_L1)
  (void)size;
#if H9_SLAB_CLASS_MIN_ID > 0u
  return class_id >= H9_SLAB_CLASS_MIN_ID;
#else
  (void)class_id;
  return true;
#endif
#else
  (void)class_id;
  return size > 49152u;
#endif
}

#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
static bool h9_slab_entry_remote_hot(H8ThreadCtx* ctx, uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
#if defined(H9_SLAB_REMOTE_HOT_POLL_L1)
#ifndef H9_SLAB_REMOTE_HOT_POLL_INTERVAL
#define H9_SLAB_REMOTE_HOT_POLL_INTERVAL 32u
#endif
  uint8_t bit = (uint8_t)(1u << class_id);
  if ((ctx->h9_slab_remote_hot_cache & bit) != 0) {
    return true;
  }
  if (ctx->h9_slab_remote_hot_poll != 0) {
    --ctx->h9_slab_remote_hot_poll;
    return false;
  }
  ctx->h9_slab_remote_hot_poll = (uint8_t)H9_SLAB_REMOTE_HOT_POLL_INTERVAL;
  ctx->h9_slab_remote_hot_cache |= h9_slab_owner_remote_hot_mask(ctx->owner);
  return (ctx->h9_slab_remote_hot_cache & bit) != 0;
#elif defined(H9_SLAB_REMOTE_HOT_MASK_L1)
  return h9_slab_owner_remote_hot(ctx->owner, class_id, true);
#else
  return h9_slab_owner_remote_hot(ctx->owner, class_id, false);
#endif
}
#endif

void* h9_slab_malloc_medium_inner(size_t size) {
  if (size <= H8_MAX_SMALL_SIZE || size > H8_MEDIUM_MAX_SIZE) {
    return h8_malloc_inner(size);
  }
#if defined(H9_SLAB_ENTRY_SIZE_BYPASS_L1)
  if (!h9_slab_entry_size_maybe_candidate(size)) {
    return h8_malloc_inner(size);
  }
#endif

  uint32_t class_id = h8_medium_class_for_size_fast(size);
  if (h9_slab_entry_candidate(size, class_id)) {
    H8ThreadCtx* ctx = h8_thread_ctx_fast();
#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
    if (!h9_slab_entry_remote_hot(ctx, class_id)) {
      H8_DEBUG_INC(h9_slab_adaptive_alloc_blocked);
    } else
#endif
    {
      H8_DEBUG_INC(h9_slab_adaptive_alloc_enabled);
      void* ptr = h9_slab_alloc(ctx, class_id);
      if (ptr) {
        return ptr;
      }
    }
  }
  return h8_medium_malloc_class_inner(class_id);
}

void h9_slab_free_outer(void* ptr) {
  if (!ptr) {
    return;
  }
  if (H8_UNLIKELY(!atomic_load_explicit(&h8g.ready, memory_order_acquire))) {
    H8_DEBUG_INC(miss_count);
    h8_sys_free(ptr);
    return;
  }
  if (!h8_arena_contains(ptr) && h9_slab_maybe_contains_hot(ptr)) {
    bool slab_owned = false;
    if (h9_slab_free_inner(ptr, &slab_owned)) {
      return;
    }
    if (slab_owned) {
      H8_DEBUG_INC(invalid_count);
      return;
    }
  }
  h8_free_inner(ptr);
}

#else

void* h9_slab_malloc_medium_inner(size_t size) {
  return h8_malloc_inner(size);
}

void h9_slab_free_outer(void* ptr) {
  h8_free_inner(ptr);
}

#endif
