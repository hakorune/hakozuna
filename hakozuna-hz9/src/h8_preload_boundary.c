#include "h8_internal.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#if (defined(H9_LOCAL_ENTRY_SPLIT_L1) && defined(H9_SLAB_ENTRY_SPLIT_L1)) || \
    (defined(H9_LOCAL_SLAB_PUBLIC_ENTRY_L0) &&                         \
     (defined(H9_LOCAL_ENTRY_SPLIT_L1) || defined(H9_SLAB_ENTRY_SPLIT_L1)))
#error "HZ9 public entry variants are exclusive"
#endif

#if defined(H9_SLAB_ENTRY_SPLIT_L1) && defined(H9_SLAB_ENTRY_SIZE_BYPASS_L1)
#if defined(H9_SLAB_CLASS_COVERAGE_L1) && !defined(H9_SLAB_CLASS_MIN_ID)
#define H9_SLAB_CLASS_MIN_ID 2u
#endif
static bool h9_slab_public_size_maybe_candidate(size_t size) {
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

static void* h8_public_malloc_dispatch(size_t size) {
#if defined(H9_LOCAL_SLAB_PUBLIC_ENTRY_L0)
  if (size > H8_MAX_SMALL_SIZE && size <= H8_MEDIUM_MAX_SIZE) {
    if (h8_thread_ctx_fast()) {
      void* ptr = h9_lsp_public_malloc(size);
      if (ptr) {
        return ptr;
      }
    }
  }
#endif
#if defined(H9_LOCAL_ENTRY_SPLIT_L1)
  if (size > H8_MAX_SMALL_SIZE && size <= H8_MEDIUM_MAX_SIZE) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_MALLOC_MEDIUM]);
    return h9_local_malloc_medium_inner(size);
  }
#endif
#if defined(H9_SLAB_ENTRY_SPLIT_L1)
  if (size > H8_MAX_SMALL_SIZE && size <= H8_MEDIUM_MAX_SIZE) {
#if defined(H9_SLAB_ENTRY_SIZE_BYPASS_L1)
    if (!h9_slab_public_size_maybe_candidate(size)) {
      H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_MALLOC_FALLBACK]);
      return h8_malloc_inner(size);
    }
#endif
    H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_MALLOC_MEDIUM]);
    return h9_slab_malloc_medium_inner(size);
  }
  H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_MALLOC_FALLBACK]);
#endif
  return h8_malloc_inner(size);
}

static void h8_public_free_dispatch(void* ptr) {
#if defined(H9_LOCAL_SLAB_PUBLIC_ENTRY_L0)
  if (ptr && atomic_load_explicit(&h8g.ready, memory_order_acquire) &&
      h8_arena_contains(ptr)) {
    h8_free_arena_inner(ptr);
    return;
  }
#endif
#if defined(H9_LOCAL_ENTRY_SPLIT_L1)
  if (ptr && atomic_load_explicit(&h8g.ready, memory_order_acquire)) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_READY]);
    if (h8_arena_contains(ptr)) {
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_ARENA_SKIP]);
      h8_free_inner(ptr);
      return;
    }
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_CHECK]);
    if (h9_local_maybe_contains_hot(ptr)) {
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_LOCAL_OUTER]);
      h9_local_free_outer(ptr);
      return;
    }
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_MAYBE_FALSE]);
  }
#endif
#if defined(H9_SLAB_ENTRY_SPLIT_L1)
  if (ptr && atomic_load_explicit(&h8g.ready, memory_order_acquire)) {
    H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_FREE_READY]);
    if (h8_arena_contains(ptr)) {
      H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_FREE_ARENA]);
      h8_free_arena_inner(ptr);
      return;
    }
    bool slab_maybe = h9_slab_maybe_contains_hot(ptr);
    if (slab_maybe) {
      H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_FREE_SLAB_OUTER]);
      h9_slab_free_outer(ptr);
      return;
    }
    H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_FREE_MAYBE_FALSE]);
  }
#endif
  h8_free_inner(ptr);
}

static void* h8_public_realloc_dispatch(void* ptr, size_t size) {
#if defined(H9_LOCAL_ENTRY_SPLIT_L1)
  if (!ptr) {
    return h8_public_malloc_dispatch(size);
  }
  if (size == 0) {
    h8_public_free_dispatch(ptr);
    return NULL;
  }
  if (h9_local_maybe_contains_hot(ptr)) {
    size_t old_size = 0;
    bool local_owned = false;
    if (h9_local_usable_size_inner(ptr, &old_size, &local_owned)) {
      void* next = h8_public_malloc_dispatch(size);
      if (!next) {
        return NULL;
      }
      size_t copy_size = old_size < size ? old_size : size;
      memcpy(next, ptr, copy_size);
      h8_public_free_dispatch(ptr);
      return next;
    }
    if (local_owned) {
      errno = EINVAL;
      return NULL;
    }
  }
#endif
#if defined(H9_SLAB_ENTRY_SPLIT_L1)
  if (!ptr) {
    return h8_public_malloc_dispatch(size);
  }
  if (size == 0) {
    h8_public_free_dispatch(ptr);
    return NULL;
  }
  H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_REALLOC_CHECK]);
  if (h9_slab_maybe_contains_hot(ptr)) {
    H8_DEBUG_INC(h9_slab_entry_event[H9_SLAB_ENTRY_REALLOC_SLAB]);
    size_t old_size = 0;
    bool slab_owned = false;
    if (h9_slab_usable_size_inner(ptr, &old_size, &slab_owned)) {
      void* next = h8_public_malloc_dispatch(size);
      if (!next) {
        return NULL;
      }
      size_t copy_size = old_size < size ? old_size : size;
      memcpy(next, ptr, copy_size);
      h8_public_free_dispatch(ptr);
      return next;
    }
    if (slab_owned) {
      errno = EINVAL;
      return NULL;
    }
  }
#endif
  return h8_realloc_inner(ptr, size);
}

#ifdef H8_BUILD_LD_PRELOAD
__attribute__((visibility("default"))) void* malloc(size_t size) {
  return h8_public_malloc_dispatch(size);
}

__attribute__((visibility("default"))) void* calloc(size_t count, size_t size) {
  return h8_calloc(count, size);
}

__attribute__((visibility("default"))) void* realloc(void* ptr, size_t size) {
  return h8_public_realloc_dispatch(ptr, size);
}

__attribute__((visibility("default"))) void free(void* ptr) {
  h8_public_free_dispatch(ptr);
}
#endif

void* h8_malloc(size_t size) {
  return h8_public_malloc_dispatch(size);
}

void* h8_calloc(size_t count, size_t size) {
  if (count != 0 && size > SIZE_MAX / count) {
    return NULL;
  }
  size_t total = count * size;
  void* ptr = h8_public_malloc_dispatch(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

void* h8_realloc(void* ptr, size_t size) {
  return h8_public_realloc_dispatch(ptr, size);
}

void h8_free(void* ptr) {
  h8_public_free_dispatch(ptr);
}
