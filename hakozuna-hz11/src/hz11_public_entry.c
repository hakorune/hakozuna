#include "hz11_public_entry.h"

#include <string.h>

void* hz11_malloc(size_t size) {
  /* Re-entry guard: if we are inside the dlsym resolver, never touch the cache. */
  if (hz11_in_resolver()) {
    return hz11_sys_malloc(size);
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  if (!tc) {
    return hz11_sys_malloc(size);
  }
  uint8_t class_id = hz11_size_class(size);
  tc->malloc_count += 1u;
  if (class_id == HZ11_LARGE_CLASS) {
    void* p = hz11_sys_malloc(size);
    if (p) {
      hz11_token_set(tc, p, HZ11_LARGE_CLASS);
    }
    return p;
  }
  void* p = hz11_thread_cache_pop(tc, class_id);
  if (p) {
    tc->malloc_hit += 1u;
    /* Cache hit: this address was token-set when it was first refilled and is
     * NOT deleted on free, so its (ptr->class) entry is already in the table.
     * Skip the redundant token_set on the hot path. If a collision evicted it
     * since, the next free token-misses and falls back to system free, which is
     * safe (the object is system-malloc'd) -- a cache-miss, never corruption. */
    return p;
  }
  p = hz11_thread_cache_refill(tc, class_id);
  if (!p) {
    return NULL;
  }
  hz11_token_set(tc, p, class_id);
  return p;
}

void hz11_free(void* ptr) {
  if (!ptr) {
    return;
  }
  if (hz11_in_resolver()) {
    hz11_sys_free(ptr);
    return;
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  uint8_t class_id;
  if (tc && hz11_token_lookup(tc, ptr, &class_id)) {
    tc->free_count += 1u;
    tc->token_hit += 1u;
    if (class_id == HZ11_LARGE_CLASS) {
      hz11_sys_free(ptr);
      return;
    }
    /* same-thread token hit: cache locally. Token is NOT deleted (speed mode). */
    hz11_thread_cache_push(tc, class_id, ptr);
    return;
  }
  if (tc) {
    tc->token_miss += 1u;
  }
  /* token miss (cross-thread / evicted / foreign): system free is always safe. */
  hz11_sys_free(ptr);
}

void* hz11_realloc(void* ptr, size_t size) {
  if (!ptr) {
    return hz11_malloc(size);
  }
  if (size == 0u) {
    hz11_free(ptr);
    return NULL;
  }
  if (hz11_in_resolver()) {
    return hz11_sys_realloc(ptr, size);
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  uint8_t old_class;
  int had_token = (tc != NULL) && hz11_token_lookup(tc, ptr, &old_class);

  /* The backing is system malloc, so sys_realloc is valid for an HZ11 pointer. */
  void* np = hz11_sys_realloc(ptr, size);
  if (!np) {
    /* failure: old ptr unchanged, its token stays valid. */
    return NULL;
  }
  /* success: old ptr is gone; reclassify and re-token the new ptr. */
  if (tc) {
    if (had_token) {
      hz11_token_invalidate(tc, ptr);
    }
    uint8_t nc = hz11_size_class(size);
    hz11_token_set(tc, np, nc);
  }
  return np;
}

void* hz11_calloc(size_t count, size_t size) {
  /* L0: route to system. The result is a system pointer; a later free
   * token-misses and hits system free, which is correct. */
  return hz11_sys_calloc(count, size);
}

size_t hz11_malloc_usable_size(void* ptr) {
  return hz11_sys_usable_size(ptr);
}

int hz11_posix_memalign(void** memptr, size_t alignment, size_t size) {
  return hz11_sys_posix_memalign(memptr, alignment, size);
}

void* hz11_aligned_alloc(size_t alignment, size_t size) {
  return hz11_sys_aligned_alloc(alignment, size);
}

void* hz11_memalign(size_t alignment, size_t size) {
  return hz11_sys_memalign(alignment, size);
}

void hz11_stats(H11Stats* out) {
  if (!out) {
    return;
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  if (!tc) {
    memset(out, 0, sizeof(*out));
    return;
  }
  out->malloc_count = tc->malloc_count;
  out->malloc_hit = tc->malloc_hit;
  out->refill_count = tc->refill_count;
  out->free_count = tc->free_count;
  out->token_hit = tc->token_hit;
  out->token_miss = tc->token_miss;
  out->overflow_count = tc->overflow_count;
  out->flush_count = tc->flush_count;
  out->flush_items = tc->flush_items;
  out->cached_bytes = tc->cached_bytes;
}
