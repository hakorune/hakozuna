#include "hz12_public_entry.h"
#include "hz12_live_footprint.h"
#include "hz12_transfer_cache.h"
#include "hz12_percpu_cache.h"
#if HZ12_FLUSH_OWNER_ROUTE
#include "hz12_flush_owner_route.h"
#include "hz12_shadow.h"
#endif

#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define HZ12_NOINLINE __attribute__((noinline))
#define HZ12_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define HZ12_NOINLINE
#define HZ12_LIKELY(x) (x)
#endif

static inline void* hz12_malloc_fast_with_tc(H12ThreadCache* tc, size_t size) {
  uint8_t class_id = hz12_size_class(size);
  HZ12_COUNT_INC(tc->malloc_count);
  HZ12_CLASS_DIAG_MALLOC(class_id);
#if HZ12_CLASSIFY_SPAN
  if (class_id == HZ12_LARGE_CLASS) {
    return hz12_sys_malloc(size); /* large: system; free misses arena -> sys_free */
  }
#if HZ12_PERCPU_RSEQ
  /* Per-CPU front cache (locked prototype): rseq-selected CPU-local slab for
   * eligible small classes, before the thread cache. pop self-probes and
   * self-disables, so call it unconditionally for eligible classes. */
  if (class_id < HZ12_PERCPU_NCLASSES) {
    void* q = NULL;
    if (hz12_percpu_pop(class_id, &q)) {
      HZ12_COUNT_INC(tc->malloc_hit);
      hz12_live_footprint_alloc(class_id, q, size);
      return q;
    }
  }
#endif
  void* p = hz12_thread_cache_pop(tc, class_id);
  if (p) {
    HZ12_COUNT_INC(tc->malloc_hit);
    HZ12_CLASS_DIAG_HIT(class_id);
    hz12_live_footprint_alloc(class_id, p, size);
    return p;
  }
  p = hz12_thread_cache_refill(tc, class_id); /* returned/bump/carve */
  if (p && hz12_arena_contains(p)) {
    hz12_live_footprint_alloc(class_id, p, size);
  }
  return p;
#else
  if (class_id == HZ12_LARGE_CLASS) {
    void* p = hz12_sys_malloc(size);
    if (p) {
      hz12_token_set(tc->tokens, p, HZ12_LARGE_CLASS);
    }
    return p;
  }
  void* p = hz12_thread_cache_pop(tc, class_id);
  if (p) {
    HZ12_COUNT_INC(tc->malloc_hit);
    HZ12_CLASS_DIAG_HIT(class_id);
    return p;
  }
  p = hz12_thread_cache_refill(tc, class_id);
  if (!p) {
    return NULL;
  }
  hz12_token_set(tc->tokens, p, class_id);
  return p;
#endif
}

#if HZ12_TLS_FASTPATH
static HZ12_NOINLINE void* hz12_malloc_slow(size_t size) {
  if (hz12_in_resolver()) {
    return hz12_sys_malloc(size);
  }
  H12ThreadCache* tc = hz12_thread_cache_get();
  if (!tc) {
    return hz12_sys_malloc(size);
  }
  return hz12_malloc_fast_with_tc(tc, size);
}
#endif

static inline void hz12_free_foreign_or_fail_closed(void* ptr) {
#if HZ12_CLASSIFY_SPAN
  if (hz12_arena_contains(ptr)) {
    return;
  }
#endif
  hz12_sys_free(ptr);
}

static inline void hz12_free_fast_with_tc(H12ThreadCache* tc, void* ptr) {
#if HZ12_CLASSIFY_SPAN
  uint8_t class_id;
  if (hz12_span_classify(ptr, &class_id)) {
    HZ12_COUNT_INC(tc->free_count);
    HZ12_COUNT_INC(tc->direct_hit_count);
    hz12_live_footprint_free(class_id, ptr);
#if HZ12_PERCPU_RSEQ
    /* Per-CPU front cache (locked prototype): push to current CPU-local slab for
     * eligible small classes; on overflow fall through to the thread cache. push
     * self-probes and self-disables, so call it unconditionally for eligible. */
    if (class_id < HZ12_PERCPU_NCLASSES) {
      if (hz12_percpu_push(class_id, ptr)) {
        return;
      }
    }
#endif
    hz12_thread_cache_push(tc, class_id, ptr);
    return;
  }
  HZ12_COUNT_INC(tc->direct_miss_count);
  hz12_free_foreign_or_fail_closed(ptr);
#else
  uint8_t class_id;
  if (hz12_token_lookup(tc->tokens, ptr, &class_id)) {
    HZ12_COUNT_INC(tc->free_count);
    HZ12_COUNT_INC(tc->token_hit);
    if (class_id == HZ12_LARGE_CLASS) {
      hz12_sys_free(ptr);
      return;
    }
    hz12_thread_cache_push(tc, class_id, ptr); /* token NOT deleted (speed mode) */
    return;
  }
  HZ12_COUNT_INC(tc->token_miss);
  hz12_sys_free(ptr); /* cross-thread / evicted / foreign */
#endif
}

#if HZ12_TLS_FASTPATH
static HZ12_NOINLINE void hz12_free_slow(void* ptr) {
  if (hz12_in_resolver()) {
    hz12_free_foreign_or_fail_closed(ptr);
    return;
  }
  H12ThreadCache* tc = hz12_thread_cache_get();
  if (tc) {
    hz12_free_fast_with_tc(tc, ptr);
    return;
  }
  hz12_free_foreign_or_fail_closed(ptr);
}
#endif

void* hz12_malloc(size_t size) {
  void* result;
#if HZ12_TLS_FASTPATH
  H12ThreadCache* tc = hz12_tls;
  if (HZ12_LIKELY(tc != NULL)) {
    result = hz12_malloc_fast_with_tc(tc, size);
  } else {
    result = hz12_malloc_slow(size);
  }
#else
  if (hz12_in_resolver()) {
    return hz12_sys_malloc(size);
  }
  H12ThreadCache* tc = hz12_thread_cache_get();
  if (!tc) {
    return hz12_sys_malloc(size);
  }
  result = hz12_malloc_fast_with_tc(tc, size);
#endif
#if HZ12_FLUSH_OWNER_ROUTE
  if (result && hz12_tls && hz12_tls->flush_owner_valid) {
    h12_shadow_on_alloc(result, hz12_tls->flush_owner_id);
  }
#endif
  return result;
}

void hz12_free(void* ptr) {
  if (!ptr) {
    return;
  }
#if HZ12_TLS_FASTPATH
  H12ThreadCache* tc = hz12_tls;
  if (HZ12_LIKELY(tc != NULL)) {
    hz12_free_fast_with_tc(tc, ptr);
    return;
  }
  hz12_free_slow(ptr);
#else
  if (hz12_in_resolver()) {
    hz12_free_foreign_or_fail_closed(ptr);
    return;
  }
  H12ThreadCache* tc = hz12_thread_cache_get();
  if (tc) {
    hz12_free_fast_with_tc(tc, ptr);
    return;
  }
  hz12_free_foreign_or_fail_closed(ptr);
#endif
}

void* hz12_realloc(void* ptr, size_t size) {
  if (!ptr) {
    return hz12_malloc(size);
  }
  if (size == 0u) {
    hz12_free(ptr);
    return NULL;
  }
  if (hz12_in_resolver()) {
    return hz12_sys_realloc(ptr, size);
  }
#if HZ12_CLASSIFY_SPAN
  if (hz12_arena_contains(ptr)) {
    /* arena ptr: fixed-size slot, sys_realloc is NOT valid. malloc+copy+free. */
    void* np = hz12_malloc(size);
    if (np) {
      uint8_t oc;
      size_t old_slot =
          hz12_span_classify(ptr, &oc) ? hz12_class_slot_size(oc) : 0u;
      size_t copy = old_slot < size ? old_slot : size;
      memcpy(np, ptr, copy);
      hz12_free(ptr);
    }
    return np;
  }
  return hz12_sys_realloc(ptr, size); /* non-arena (large/foreign) */
#else
  H12ThreadCache* tc = hz12_thread_cache_get();
  uint8_t old_class;
  int had_token = (tc != NULL) && hz12_token_lookup(tc->tokens, ptr, &old_class);
  void* np = hz12_sys_realloc(ptr, size); /* L0 backing is system malloc */
  if (!np) {
    return NULL; /* failure: old ptr + token stay valid */
  }
  if (tc) {
    if (had_token) {
      hz12_token_invalidate(tc->tokens, ptr);
    }
    hz12_token_set(tc->tokens, np, hz12_size_class(size));
  }
  return np;
#endif
}

void* hz12_calloc(size_t count, size_t size) {
  /* Route to system. The result is a system pointer; a later free misses the
   * arena/token table and hits sys_free, which is correct. */
  return hz12_sys_calloc(count, size);
}

size_t hz12_malloc_usable_size(void* ptr) {
  if (!ptr) {
    return 0u;
  }
#if HZ12_CLASSIFY_SPAN
  /* Arena pointers are not libc chunks: hz12_sys_usable_size would read the arena
   * slot data as a libc chunk header and fault. Return the real slot size instead.
   * (HZ12RocksdbReadrandomCrashRootCause-L1) */
  uint8_t class_id;
  if (hz12_span_classify(ptr, &class_id)) {
    return hz12_class_slot_size(class_id);
  }
  if (hz12_arena_contains(ptr)) {
    return 0u;
  }
#endif
  return hz12_sys_usable_size(ptr);
}

int hz12_posix_memalign(void** memptr, size_t alignment, size_t size) {
  return hz12_sys_posix_memalign(memptr, alignment, size);
}

void* hz12_aligned_alloc(size_t alignment, size_t size) {
  return hz12_sys_aligned_alloc(alignment, size);
}

void* hz12_memalign(size_t alignment, size_t size) {
  return hz12_sys_memalign(alignment, size);
}

void hz12_stats(H12Stats* out) {
  if (!out) {
    return;
  }
  H12ThreadCache* tc = hz12_thread_cache_get();
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
#if HZ12_CLASSIFY_SPAN
  out->token_hit = 0;
  out->token_miss = 0;
  out->direct_hit_count = tc->direct_hit_count;
  out->direct_miss_count = tc->direct_miss_count;
  out->span_create_count = hz12_span_create_count_load();
  out->returned_push = hz12_returned_push_count_load();
  out->returned_pop_hit = hz12_returned_pop_hit_count_load();
  out->returned_pop_miss = hz12_returned_pop_miss_count_load();
#else
  out->token_hit = tc->token_hit;
  out->token_miss = tc->token_miss;
  out->direct_hit_count = 0;
  out->direct_miss_count = 0;
  out->span_create_count = 0;
  out->returned_push = 0;
  out->returned_pop_hit = 0;
  out->returned_pop_miss = 0;
#endif
#if HZ12_TRANSFER_CENTRAL_SPAN
  out->refill_from_transfer = tc->refill_from_transfer;
  out->refill_from_central = tc->refill_from_central;
  out->refill_from_span = tc->refill_from_span;
  out->transfer_remove_hit = hz12_transfer_remove_hit_count_load();
  out->transfer_remove_miss = hz12_transfer_remove_miss_count_load();
  out->transfer_insert = hz12_transfer_insert_count_load();
  out->transfer_insert_spill = hz12_transfer_insert_spill_count_load();
  out->central_remove_hit = hz12_central_remove_hit_count_load();
  out->central_remove_miss = hz12_central_remove_miss_count_load();
  out->central_insert = hz12_central_insert_count_load();
  out->span_return_count = hz12_span_return_count_load();
  out->span_reuse_count = hz12_span_reuse_count_load();
  out->central_full_span_count = hz12_central_full_span_count_load();
  out->central_partial_span_count = hz12_central_partial_span_count_load();
  out->central_objects = hz12_central_object_count_load();
#else
  out->refill_from_transfer = 0;
  out->refill_from_central = 0;
  out->refill_from_span = 0;
  out->transfer_remove_hit = 0;
  out->transfer_remove_miss = 0;
  out->transfer_insert = 0;
  out->transfer_insert_spill = 0;
  out->central_remove_hit = 0;
  out->central_remove_miss = 0;
  out->central_insert = 0;
  out->span_return_count = 0;
  out->span_reuse_count = 0;
  out->central_full_span_count = 0;
  out->central_partial_span_count = 0;
  out->central_objects = 0;
#endif
}
