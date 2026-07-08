#ifndef HZ11_THREAD_CACHE_H
#define HZ11_THREAD_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "hz11_size_class.h"
#include "hz11_sys_alloc.h"
#include "hz11_token_table.h"

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

/* HZ11TLSFastPath-L1: public entry reads hz11_tls once and sends the missing
 * TLS / resolver cases to a noinline slow helper. Default OFF; sibling lanes
 * build with -DHZ11_TLS_FASTPATH=1 for A/B. */
#ifndef HZ11_TLS_FASTPATH
#define HZ11_TLS_FASTPATH 0
#endif

/* HZ11CacheByteAccountingGate-L1: default keeps global per-thread cached-byte
 * accounting and the HZ11_MAX_CACHED_BYTES cap. Sibling speed-ceiling lanes can
 * build with -DHZ11_CACHE_BYTE_ACCOUNTING=0 to price that accounting cost while
 * leaving the per-class HZ11_CACHE_CAP in place. */
#ifndef HZ11_CACHE_BYTE_ACCOUNTING
#define HZ11_CACHE_BYTE_ACCOUNTING 1
#endif

/* HZ11CacheLayout-L1: SOA (structure-of-arrays) class cache.
 * Splits the AoS H11ClassCache[13] into two parallel arrays with power-of-2
 * strides (256B items + 4B counts), eliminating the *264 address chain.
 * Only valid with TLS_FASTPATH + no-bytes (speed-ceiling lane). */
#ifndef HZ11_CACHE_SOA
#define HZ11_CACHE_SOA 0
#endif
#if HZ11_CACHE_SOA && HZ11_CACHE_TOPPTR
#error "HZ11_CACHE_SOA and HZ11_CACHE_TOPPTR are alternative cache-shape lanes"
#endif
#if HZ11_CACHE_SOA && HZ11_CACHE_BYTE_ACCOUNTING
#error "HZ11_CACHE_SOA is only defined for the no-bytes speed-ceiling lane"
#endif

/* HZ11CurrentSpanPoolThreadExit-L1: opt-in thread-exit salvage for the
 * transfer span lane. The default path has no pthread key/destructor. */
#ifndef HZ11_CURRENT_SPAN_THREAD_EXIT
#define HZ11_CURRENT_SPAN_THREAD_EXIT 0
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
#if HZ11_CACHE_SOA
  void* class_items[HZ11_CLASS_COUNT][HZ11_CACHE_CAP]; /* stride 256B per class */
  uint32_t class_counts[HZ11_CLASS_COUNT];             /* stride 4B per class */
#else
  H11ClassCache class_cache[HZ11_CLASS_COUNT];          /* stride 264B (AoS) */
#endif
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
#if HZ11_TRANSFER_CENTRAL_SPAN
  uint64_t refill_from_transfer;
  uint64_t refill_from_central;
  uint64_t refill_from_span;
#endif
} H11ThreadCache;

/* State defined in hz11_thread_cache.c. */
extern _Thread_local H11ThreadCache* hz11_tls;

/* Cold paths (out-of-line). */
H11ThreadCache* hz11_thread_cache_init_slow(void);
void hz11_thread_cache_push_overflow_slow(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr);
void* hz11_thread_cache_refill(H11ThreadCache* tc, uint8_t class_id);
void hz11_current_span_pool_dump_stats(void);

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
#endif /* !HZ11_CLASSIFY_SPAN */

static inline void* hz11_thread_cache_pop(H11ThreadCache* tc,
                                          uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return NULL;
  }
#if HZ11_CACHE_SOA
  uint32_t cnt = tc->class_counts[class_id];
  if (cnt == 0u) {
    return NULL;
  }
  cnt -= 1u;
  tc->class_counts[class_id] = cnt;
  return tc->class_items[class_id][cnt];
#else
  H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
  if (cc->top == cc->items) {
    return NULL;
  }
  void* p = *--cc->top;
#if HZ11_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz11_class_slot_size(class_id);
#endif
  return p;
#else
  if (cc->count == 0u) {
    return NULL;
  }
  void* p = cc->items[--cc->count];
#if HZ11_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz11_class_slot_size(class_id);
#endif
  return p;
#endif
#endif /* HZ11_CACHE_SOA */
}

static inline void hz11_thread_cache_push(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
#if HZ11_CACHE_SOA
  uint32_t cnt = tc->class_counts[class_id];
  if (cnt < HZ11_CACHE_CAP) {
    tc->class_items[class_id][cnt] = ptr;
    tc->class_counts[class_id] = cnt + 1u;
    return;
  }
  hz11_thread_cache_push_overflow_slow(tc, class_id, ptr);
#else
  H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
#if HZ11_CACHE_BYTE_ACCOUNTING
  size_t slot = hz11_class_slot_size(class_id);
  if (cc->top < cc->items + HZ11_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ11_MAX_CACHED_BYTES) {
    *cc->top++ = ptr;
    tc->cached_bytes += slot;
    return;
  }
#else
  if (cc->top < cc->items + HZ11_CACHE_CAP) {
    *cc->top++ = ptr;
    return;
  }
#endif
  hz11_thread_cache_push_overflow_slow(tc, class_id, ptr);
#else
#if HZ11_CACHE_BYTE_ACCOUNTING
  size_t slot = hz11_class_slot_size(class_id);
  if (cc->count < HZ11_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ11_MAX_CACHED_BYTES) {
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += slot;
    return;
  }
#else
  if (cc->count < HZ11_CACHE_CAP) {
    cc->items[cc->count++] = ptr;
    return;
  }
#endif
  hz11_thread_cache_push_overflow_slow(tc, class_id, ptr);
#endif
#endif /* HZ11_CACHE_SOA */
}

#endif /* HZ11_THREAD_CACHE_H */
