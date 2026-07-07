#ifndef HZ11_THREAD_CACHE_H
#define HZ11_THREAD_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "hz11_size_class.h"

/* Per-thread front-end cache + pointer->class token table (L0).
 * Hot-path helpers below are static inline so malloc/free do not pay a call per
 * helper; only the cold paths (init, refill, overflow flush, dlsym resolver) are
 * out-of-line in hz11_thread_cache.c. */

#define HZ11_CACHE_CAP 32u
#define HZ11_TOKEN_COUNT 1024u                        /* power of two */
#define HZ11_MAX_CACHED_BYTES (2u * 1024u * 1024u)

typedef struct H11Token {
  void* ptr;
  uint8_t class_id;
} H11Token;

typedef struct H11ClassCache {
  void* items[HZ11_CACHE_CAP];
  uint32_t count;
} H11ClassCache;

typedef struct H11ThreadCache {
  H11ClassCache class_cache[HZ11_CLASS_COUNT];
  H11Token tokens[HZ11_TOKEN_COUNT];
  size_t cached_bytes;
  uint64_t malloc_count;
  uint64_t malloc_hit;
  uint64_t refill_count;
  uint64_t free_count;
  uint64_t token_hit;
  uint64_t token_miss;
  uint64_t overflow_count;
  uint64_t flush_count;
  uint64_t flush_items;
} H11ThreadCache;

/* State defined in hz11_thread_cache.c. */
extern _Thread_local H11ThreadCache* hz11_tls;
extern _Thread_local int hz11_resolving;

/* Cold paths (out-of-line). */
H11ThreadCache* hz11_thread_cache_init_slow(void);
void hz11_thread_cache_push_overflow_slow(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr);
void* hz11_thread_cache_refill(H11ThreadCache* tc, uint8_t class_id);
void hz11_resolver_ensure(void);
void* hz11_sys_malloc(size_t n);
void hz11_sys_free(void* p);
void* hz11_sys_realloc(void* p, size_t n);
void* hz11_sys_calloc(size_t count, size_t size);
size_t hz11_sys_usable_size(void* p);
int hz11_sys_posix_memalign(void** memptr, size_t alignment, size_t size);
void* hz11_sys_aligned_alloc(size_t alignment, size_t size);
void* hz11_sys_memalign(size_t alignment, size_t size);

/* ---------- hot path (static inline) ---------- */

static inline int hz11_in_resolver(void) {
  return hz11_resolving;
}

static inline H11ThreadCache* hz11_thread_cache_get(void) {
  if (hz11_tls) {
    return hz11_tls;
  }
  return hz11_thread_cache_init_slow();
}

static inline size_t hz11_token_hash(const void* ptr) {
  uintptr_t x = (uintptr_t)ptr;
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  return (size_t)(x & (HZ11_TOKEN_COUNT - 1u));
}

static inline void hz11_token_set(H11ThreadCache* tc, void* ptr,
                                  uint8_t class_id) {
  size_t i = hz11_token_hash(ptr);
  tc->tokens[i].ptr = ptr;
  tc->tokens[i].class_id = class_id;
}

static inline int hz11_token_lookup(H11ThreadCache* tc, void* ptr,
                                    uint8_t* class_id_out) {
  size_t i = hz11_token_hash(ptr);
  if (tc->tokens[i].ptr == ptr) {
    *class_id_out = tc->tokens[i].class_id;
    return 1;
  }
  return 0;
}

static inline void hz11_token_invalidate(H11ThreadCache* tc, void* ptr) {
  size_t i = hz11_token_hash(ptr);
  if (tc->tokens[i].ptr == ptr) {
    tc->tokens[i].ptr = NULL;
  }
}

static inline void* hz11_thread_cache_pop(H11ThreadCache* tc,
                                          uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return NULL;
  }
  H11ClassCache* cc = &tc->class_cache[class_id];
  if (cc->count == 0u) {
    return NULL;
  }
  void* p = cc->items[--cc->count];
  tc->cached_bytes -= hz11_class_slot_size(class_id);
  return p;
}

static inline void hz11_thread_cache_push(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  H11ClassCache* cc = &tc->class_cache[class_id];
  size_t slot = hz11_class_slot_size(class_id);
  if (cc->count < HZ11_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ11_MAX_CACHED_BYTES) {
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += slot;
    return;
  }
  hz11_thread_cache_push_overflow_slow(tc, class_id, ptr);
}

#endif /* HZ11_THREAD_CACHE_H */
