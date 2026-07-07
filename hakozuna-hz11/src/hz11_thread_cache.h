#ifndef HZ11_THREAD_CACHE_H
#define HZ11_THREAD_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "hz11_size_class.h"

/* Per-thread front-end cache. Hot-path helpers below are static inline so
 * malloc/free do not pay a call per helper; only the cold paths (init, refill,
 * overflow flush, dlsym resolver) are out-of-line in hz11_thread_cache.c.
 *
 * Two classify lanes, selected by HZ11_CLASSIFY_SPAN:
 *  - 0 (L0, default): system-malloc backing + pointer->class token table.
 *  - 1 (L1): HZ11 arena/span backing + direct-index classify (hz11_span.h). */

#ifndef HZ11_CLASSIFY_SPAN
#define HZ11_CLASSIFY_SPAN 0
#endif

/* HZ11StatsCompileGate-L1: hot-path counter increments are compile-time opt-in.
 * Default OFF (speed lane). Diagnostic siblings (libhz11_stats.so etc.) build
 * with -DHZ11_ENABLE_HOT_COUNTERS=1. NO runtime branch -- the macros expand to
 * ((void)0) when off, so the compiler eliminates the increment entirely. */
#ifndef HZ11_ENABLE_HOT_COUNTERS
#define HZ11_ENABLE_HOT_COUNTERS 0
#endif
#if HZ11_ENABLE_HOT_COUNTERS
#define HZ11_COUNT_INC(x) ((x) += 1u)
#define HZ11_COUNT_ADD(x, n) ((x) += (n))
#else
#define HZ11_COUNT_INC(x) ((void)0)
#define HZ11_COUNT_ADD(x, n) ((void)0)
#endif

#define HZ11_CACHE_CAP 32u
#define HZ11_MAX_CACHED_BYTES (2u * 1024u * 1024u)

/* HZ11CacheShape-L1: pointer-top pop/push (A/B vs count-indexed).
 * Default OFF (count-indexed). Sibling .so lanes build with
 * -DHZ11_CACHE_TOPPTR=1 to use a moving void** top pointer instead. */
#ifndef HZ11_CACHE_TOPPTR
#define HZ11_CACHE_TOPPTR 0
#endif

typedef struct H11ClassCache {
  void* items[HZ11_CACHE_CAP];
  uint32_t count;
#if HZ11_CACHE_TOPPTR
  void** top;  /* pointer-top: init to items; top==items means empty */
#endif
} H11ClassCache;

#if !HZ11_CLASSIFY_SPAN
/* L0 token lane. */
#define HZ11_TOKEN_COUNT 1024u /* power of two */
typedef struct H11Token {
  void* ptr;
  uint8_t class_id;
} H11Token;
#else
/* L1 span lane: per-thread current span per class (bump source). */
#include "hz11_span.h"
typedef struct H11SpanCurrent {
  char* base;
  uint32_t bump_index;
  uint32_t slot_count;
} H11SpanCurrent;
#endif

typedef struct H11ThreadCache {
  H11ClassCache class_cache[HZ11_CLASS_COUNT];
#if !HZ11_CLASSIFY_SPAN
  H11Token tokens[HZ11_TOKEN_COUNT];
#else
  H11SpanCurrent current[HZ11_CLASS_COUNT];
#endif
  size_t cached_bytes;
  uint64_t malloc_count;
  uint64_t malloc_hit;
  uint64_t refill_count;
  uint64_t free_count;
#if !HZ11_CLASSIFY_SPAN
  uint64_t token_hit;
  uint64_t token_miss;
#else
  uint64_t direct_hit_count;
  uint64_t direct_miss_count;
#endif
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

#if !HZ11_CLASSIFY_SPAN
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
#endif /* !HZ11_CLASSIFY_SPAN */

static inline void* hz11_thread_cache_pop(H11ThreadCache* tc,
                                          uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return NULL;
  }
  H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
  if (cc->top == cc->items) {
    return NULL;
  }
  void* p = *--cc->top;
  tc->cached_bytes -= hz11_class_slot_size(class_id);
  return p;
#else
  if (cc->count == 0u) {
    return NULL;
  }
  void* p = cc->items[--cc->count];
  tc->cached_bytes -= hz11_class_slot_size(class_id);
  return p;
#endif
}

static inline void hz11_thread_cache_push(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
  size_t slot = hz11_class_slot_size(class_id);
  if (cc->top < cc->items + HZ11_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ11_MAX_CACHED_BYTES) {
    *cc->top++ = ptr;
    tc->cached_bytes += slot;
    return;
  }
  hz11_thread_cache_push_overflow_slow(tc, class_id, ptr);
#else
  size_t slot = hz11_class_slot_size(class_id);
  if (cc->count < HZ11_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ11_MAX_CACHED_BYTES) {
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += slot;
    return;
  }
  hz11_thread_cache_push_overflow_slow(tc, class_id, ptr);
#endif
}

#endif /* HZ11_THREAD_CACHE_H */
