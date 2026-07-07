#include "hz11_public_entry.h"

#include <string.h>

#if HZ11_TLS_FASTPATH
#if defined(__GNUC__) || defined(__clang__)
#define HZ11_NOINLINE __attribute__((noinline))
#define HZ11_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define HZ11_NOINLINE
#define HZ11_LIKELY(x) (x)
#endif

static inline void* hz11_malloc_fast_with_tc(H11ThreadCache* tc, size_t size) {
  uint8_t class_id = hz11_size_class(size);
  HZ11_COUNT_INC(tc->malloc_count);
#if HZ11_CLASSIFY_SPAN
  if (class_id == HZ11_LARGE_CLASS) {
    return hz11_sys_malloc(size); /* large: system; free misses arena -> sys_free */
  }
  void* p = hz11_thread_cache_pop(tc, class_id);
  if (p) {
    HZ11_COUNT_INC(tc->malloc_hit);
    return p;
  }
  return hz11_thread_cache_refill(tc, class_id); /* returned/bump/carve */
#else
  if (class_id == HZ11_LARGE_CLASS) {
    void* p = hz11_sys_malloc(size);
    if (p) {
      hz11_token_set(tc, p, HZ11_LARGE_CLASS);
    }
    return p;
  }
  void* p = hz11_thread_cache_pop(tc, class_id);
  if (p) {
    HZ11_COUNT_INC(tc->malloc_hit);
    return p;
  }
  p = hz11_thread_cache_refill(tc, class_id);
  if (!p) {
    return NULL;
  }
  hz11_token_set(tc, p, class_id);
  return p;
#endif
}

static HZ11_NOINLINE void* hz11_malloc_slow(size_t size) {
  if (hz11_in_resolver()) {
    return hz11_sys_malloc(size);
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  if (!tc) {
    return hz11_sys_malloc(size);
  }
  return hz11_malloc_fast_with_tc(tc, size);
}

static inline void hz11_free_fast_with_tc(H11ThreadCache* tc, void* ptr) {
#if HZ11_CLASSIFY_SPAN
  uint8_t class_id;
  if (hz11_span_classify(ptr, &class_id)) {
    HZ11_COUNT_INC(tc->free_count);
    HZ11_COUNT_INC(tc->direct_hit_count);
    hz11_thread_cache_push(tc, class_id, ptr);
    return;
  }
  HZ11_COUNT_INC(tc->direct_miss_count);
  hz11_sys_free(ptr);
#else
  uint8_t class_id;
  if (hz11_token_lookup(tc, ptr, &class_id)) {
    HZ11_COUNT_INC(tc->free_count);
    HZ11_COUNT_INC(tc->token_hit);
    if (class_id == HZ11_LARGE_CLASS) {
      hz11_sys_free(ptr);
      return;
    }
    hz11_thread_cache_push(tc, class_id, ptr); /* token NOT deleted (speed mode) */
    return;
  }
  HZ11_COUNT_INC(tc->token_miss);
  hz11_sys_free(ptr); /* cross-thread / evicted / foreign */
#endif
}

static HZ11_NOINLINE void hz11_free_slow(void* ptr) {
  if (hz11_in_resolver()) {
    hz11_sys_free(ptr);
    return;
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  if (tc) {
    hz11_free_fast_with_tc(tc, ptr);
    return;
  }
  hz11_sys_free(ptr);
}
#endif

void* hz11_malloc(size_t size) {
#if HZ11_TLS_FASTPATH
  H11ThreadCache* tc = hz11_tls;
  if (HZ11_LIKELY(tc != NULL)) {
    return hz11_malloc_fast_with_tc(tc, size);
  }
  return hz11_malloc_slow(size);
#else
  if (hz11_in_resolver()) {
    return hz11_sys_malloc(size);
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  if (!tc) {
    return hz11_sys_malloc(size);
  }
  uint8_t class_id = hz11_size_class(size);
  HZ11_COUNT_INC(tc->malloc_count);
#if HZ11_CLASSIFY_SPAN
  if (class_id == HZ11_LARGE_CLASS) {
    return hz11_sys_malloc(size); /* large: system; free misses arena -> sys_free */
  }
  void* p = hz11_thread_cache_pop(tc, class_id);
  if (p) {
    HZ11_COUNT_INC(tc->malloc_hit);
    return p;
  }
  return hz11_thread_cache_refill(tc, class_id); /* returned/bump/carve */
#else
  if (class_id == HZ11_LARGE_CLASS) {
    void* p = hz11_sys_malloc(size);
    if (p) {
      hz11_token_set(tc, p, HZ11_LARGE_CLASS);
    }
    return p;
  }
  void* p = hz11_thread_cache_pop(tc, class_id);
  if (p) {
    HZ11_COUNT_INC(tc->malloc_hit);
    /* Cache hit: the token was set when first refilled and is not deleted on
     * free, so it is already present. Skip the redundant token_set. */
    return p;
  }
  p = hz11_thread_cache_refill(tc, class_id);
  if (!p) {
    return NULL;
  }
  hz11_token_set(tc, p, class_id);
  return p;
#endif
#endif
}

void hz11_free(void* ptr) {
  if (!ptr) {
    return;
  }
#if HZ11_TLS_FASTPATH
  H11ThreadCache* tc = hz11_tls;
  if (HZ11_LIKELY(tc != NULL)) {
    hz11_free_fast_with_tc(tc, ptr);
    return;
  }
  hz11_free_slow(ptr);
#else
  if (hz11_in_resolver()) {
    hz11_sys_free(ptr);
    return;
  }
#if HZ11_CLASSIFY_SPAN
  uint8_t class_id;
  if (hz11_span_classify(ptr, &class_id)) {
    H11ThreadCache* tc = hz11_thread_cache_get();
    if (tc) {
      HZ11_COUNT_INC(tc->free_count);
      HZ11_COUNT_INC(tc->direct_hit_count);
      hz11_thread_cache_push(tc, class_id, ptr);
      return;
    }
  }
  H11ThreadCache* tc = hz11_thread_cache_get();
  if (tc) {
    HZ11_COUNT_INC(tc->direct_miss_count);
  }
  /* arena-miss / uncarved / foreign / large: sys_free is correct (system ptr). */
  hz11_sys_free(ptr);
#else
  H11ThreadCache* tc = hz11_thread_cache_get();
  uint8_t class_id;
  if (tc && hz11_token_lookup(tc, ptr, &class_id)) {
    HZ11_COUNT_INC(tc->free_count);
    HZ11_COUNT_INC(tc->token_hit);
    if (class_id == HZ11_LARGE_CLASS) {
      hz11_sys_free(ptr);
      return;
    }
    hz11_thread_cache_push(tc, class_id, ptr); /* token NOT deleted (speed mode) */
    return;
  }
  if (tc) {
    HZ11_COUNT_INC(tc->token_miss);
  }
  hz11_sys_free(ptr); /* cross-thread / evicted / foreign */
#endif
#endif
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
#if HZ11_CLASSIFY_SPAN
  if (hz11_arena_contains(ptr)) {
    /* arena ptr: fixed-size slot, sys_realloc is NOT valid. malloc+copy+free. */
    void* np = hz11_malloc(size);
    if (np) {
      uint8_t oc;
      size_t old_slot =
          hz11_span_classify(ptr, &oc) ? hz11_class_slot_size(oc) : 0u;
      size_t copy = old_slot < size ? old_slot : size;
      memcpy(np, ptr, copy);
      hz11_free(ptr);
    }
    return np;
  }
  return hz11_sys_realloc(ptr, size); /* non-arena (large/foreign) */
#else
  H11ThreadCache* tc = hz11_thread_cache_get();
  uint8_t old_class;
  int had_token = (tc != NULL) && hz11_token_lookup(tc, ptr, &old_class);
  void* np = hz11_sys_realloc(ptr, size); /* L0 backing is system malloc */
  if (!np) {
    return NULL; /* failure: old ptr + token stay valid */
  }
  if (tc) {
    if (had_token) {
      hz11_token_invalidate(tc, ptr);
    }
    hz11_token_set(tc, np, hz11_size_class(size));
  }
  return np;
#endif
}

void* hz11_calloc(size_t count, size_t size) {
  /* Route to system. The result is a system pointer; a later free misses the
   * arena/token table and hits sys_free, which is correct. */
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
  out->overflow_count = tc->overflow_count;
  out->flush_count = tc->flush_count;
  out->flush_items = tc->flush_items;
  out->cached_bytes = tc->cached_bytes;
#if HZ11_CLASSIFY_SPAN
  out->token_hit = 0;
  out->token_miss = 0;
  out->direct_hit_count = tc->direct_hit_count;
  out->direct_miss_count = tc->direct_miss_count;
  out->span_create_count = hz11_span_create_count_load();
#else
  out->token_hit = tc->token_hit;
  out->token_miss = tc->token_miss;
  out->direct_hit_count = 0;
  out->direct_miss_count = 0;
  out->span_create_count = 0;
#endif
}
