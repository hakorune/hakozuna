#ifndef HZ12_THREAD_CACHE_H
#define HZ12_THREAD_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "hz12_port.h"
#include "hz12_size_class.h"
#include "hz12_sys_alloc.h"
#include "hz12_token_table.h"
#include "hz12_class_diag.h"

/* Per-thread front-end cache. Hot-path helpers below are static inline so
 * malloc/free do not pay a call per helper; only the cold paths (init, refill,
 * overflow flush, dlsym resolver) are out-of-line in hz12_thread_cache.c.
 *
 * Two classify lanes, selected by HZ12_CLASSIFY_SPAN:
 *  - 0 (L0, default): system-malloc backing + pointer->class token table.
 *  - 1 (L1): HZ12 arena/span backing + direct-index classify (hz12_span.h). */

#ifndef HZ12_CLASSIFY_SPAN
#define HZ12_CLASSIFY_SPAN 0
#endif

/* HZ12StatsCompileGate-L1: hot-path counter increments are compile-time opt-in.
 * Default OFF (speed lane). Diagnostic siblings (libhz12_stats.so etc.) build
 * with -DHZ12_ENABLE_HOT_COUNTERS=1. NO runtime branch -- the macros expand to
 * ((void)0) when off, so the compiler eliminates the increment entirely. */
#ifndef HZ12_ENABLE_HOT_COUNTERS
#define HZ12_ENABLE_HOT_COUNTERS 0
#endif
#if HZ12_ENABLE_HOT_COUNTERS
#define HZ12_COUNT_INC(x) ((x) += 1u)
#define HZ12_COUNT_ADD(x, n) ((x) += (n))
#else
#define HZ12_COUNT_INC(x) ((void)0)
#define HZ12_COUNT_ADD(x, n) ((void)0)
#endif

#ifndef HZ12_CACHE_CAP
#define HZ12_CACHE_CAP 32u
#endif
#ifndef HZ12_MAX_CACHED_BYTES
#define HZ12_MAX_CACHED_BYTES (2u * 1024u * 1024u)
#endif

/* HZ12CacheShape-L1: pointer-top pop/push (A/B vs count-indexed).
 * Default OFF (count-indexed). Sibling .so lanes build with
 * -DHZ12_CACHE_TOPPTR=1 to use a moving void** top pointer instead. */
#ifndef HZ12_CACHE_TOPPTR
#define HZ12_CACHE_TOPPTR 0
#endif

/* HZ12TLSFastPath-L1: public entry reads hz12_tls once and sends the missing
 * TLS / resolver cases to a noinline slow helper. Default OFF; sibling lanes
 * build with -DHZ12_TLS_FASTPATH=1 for A/B. */
#ifndef HZ12_TLS_FASTPATH
#define HZ12_TLS_FASTPATH 0
#endif

/* HZ12CacheByteAccountingGate-L1: default keeps global per-thread cached-byte
 * accounting and the HZ12_MAX_CACHED_BYTES cap. Sibling speed-ceiling lanes can
 * build with -DHZ12_CACHE_BYTE_ACCOUNTING=0 to price that accounting cost while
 * leaving the per-class HZ12_CACHE_CAP in place. */
#ifndef HZ12_CACHE_BYTE_ACCOUNTING
#define HZ12_CACHE_BYTE_ACCOUNTING 1
#endif

/* WindowsSpanClassAwareRefill-L1: diagnostic/sibling behavior for the returned
 * object sink. Instead of one mutex pop per refill, pop a small batch for the
 * dominant mid classes and seed the front cache. */
#ifndef HZ12_RETURNED_REFILL_BATCH
#define HZ12_RETURNED_REFILL_BATCH 0u
#endif
#ifndef HZ12_RETURNED_REFILL_BATCH_MIN_CLASS
#define HZ12_RETURNED_REFILL_BATCH_MIN_CLASS 4u
#endif
#ifndef HZ12_RETURNED_REFILL_BATCH_MAX_CLASS
#define HZ12_RETURNED_REFILL_BATCH_MAX_CLASS (HZ12_CLASS_COUNT - 1u)
#endif
#ifndef HZ12_RETURNED_REFILL_BATCH_COUNT
#define HZ12_RETURNED_REFILL_BATCH_COUNT 32u
#endif
#ifndef HZ12_RETURNED_REFILL_BATCH_PRESSURE_GATE
#define HZ12_RETURNED_REFILL_BATCH_PRESSURE_GATE 0u
#endif
#ifndef HZ12_RETURNED_REFILL_BATCH_PRESSURE_THRESHOLD
#define HZ12_RETURNED_REFILL_BATCH_PRESSURE_THRESHOLD 4u
#endif
/* HZ12ReturnedColdSkip-L1: opt-in matrix probe. When returned-pop recently
 * missed and the current span still has slots, skip a few returned-sink lock
 * attempts and bump from current directly. Overflow flush clears the hint. */
#ifndef HZ12_RETURNED_REFILL_COLD_SKIP
#define HZ12_RETURNED_REFILL_COLD_SKIP 0u
#endif
#ifndef HZ12_RETURNED_REFILL_COLD_SKIP_BUDGET
#define HZ12_RETURNED_REFILL_COLD_SKIP_BUDGET 8u
#endif

/* HZ12SpanBumpBatch-L1: opt-in span refill batching. The span lane keeps the
 * returned sink first, then carves a bounded batch from the current span and
 * seeds the local cache. The selected Windows row remains unchanged. */
#ifndef HZ12_SPAN_BUMP_BATCH
#define HZ12_SPAN_BUMP_BATCH 0u
#endif
#ifndef HZ12_SPAN_BUMP_BATCH_COUNT
#define HZ12_SPAN_BUMP_BATCH_COUNT 16u
#endif

/* HZ12ReturnedPushRange-L1: opt-in flush-side splice. */
#ifndef HZ12_RETURNED_PUSH_RANGE
#define HZ12_RETURNED_PUSH_RANGE 0u
#endif
#ifndef HZ12_RETURNED_PUSH_RANGE_CHUNK
#define HZ12_RETURNED_PUSH_RANGE_CHUNK 0u
#endif

#ifndef HZ12_FLUSH_OWNER_ROUTE
#define HZ12_FLUSH_OWNER_ROUTE 0u
#endif

#ifndef HZ12_FLUSH_OWNER_ROUTE_INERT
#define HZ12_FLUSH_OWNER_ROUTE_INERT 0u
#endif

#ifndef HZ12_FLUSH_OWNER_COLD_SPAN
#define HZ12_FLUSH_OWNER_COLD_SPAN 0u
#endif

/* L6-B diagnostic sibling. Normal and speed lanes leave this compiled out. */
#ifndef HZ12_OWNER_BATCH_LEDGER_DIAG
#define HZ12_OWNER_BATCH_LEDGER_DIAG 0u
#endif
#ifndef HZ12_OWNER_BATCH_LEDGER
#define HZ12_OWNER_BATCH_LEDGER HZ12_OWNER_BATCH_LEDGER_DIAG
#endif

/* HZ12CacheLayout-L1: SOA (structure-of-arrays) class cache.
 * Splits the AoS H12ClassCache[13] into two parallel arrays with power-of-2
 * strides (256B items + 4B counts), eliminating the *264 address chain.
 * Only valid with TLS_FASTPATH + no-bytes (speed-ceiling lane). */
#ifndef HZ12_CACHE_SOA
#define HZ12_CACHE_SOA 0
#endif
#if HZ12_CACHE_SOA && HZ12_CACHE_TOPPTR
#error "HZ12_CACHE_SOA and HZ12_CACHE_TOPPTR are alternative cache-shape lanes"
#endif
/* HZ12ThreadCacheCapacityByteCap-L1: SOA + BYTE_ACCOUNTING is now allowed. The byte
 * cap is enforced on the SOA push/pop when HZ12_CACHE_BYTE_ACCOUNTING=1; existing
 * no-bytes lanes (BYTE_ACCOUNTING=0) are unaffected (they hit the #else SOA branch). */

/* HZ12CurrentSpanPoolThreadExit-L1: opt-in thread-exit salvage for the
 * transfer span lane. The default path has no pthread key/destructor. */
#ifndef HZ12_CURRENT_SPAN_THREAD_EXIT
#define HZ12_CURRENT_SPAN_THREAD_EXIT 0
#endif

typedef struct H12ClassCache {
  void* items[HZ12_CACHE_CAP];
  uint32_t count;
#if HZ12_CACHE_TOPPTR
  void** top;  /* pointer-top: init to items; top==items means empty */
#endif
} H12ClassCache;

#if !HZ12_CLASSIFY_SPAN
/* L0 token lane. */
#else
/* L1 span lane: per-thread current span per class (bump source). */
#include "hz12_span.h"
typedef struct H12SpanCurrent {
  char* base;
  uint32_t bump_index;
  uint32_t slot_count;
} H12SpanCurrent;
#endif

typedef struct H12ThreadCache {
#if HZ12_CACHE_SOA
  void* class_items[HZ12_CLASS_COUNT][HZ12_CACHE_CAP]; /* stride 256B per class */
  uint32_t class_counts[HZ12_CLASS_COUNT];             /* stride 4B per class */
#else
  H12ClassCache class_cache[HZ12_CLASS_COUNT];          /* stride 264B (AoS) */
#endif
#if !HZ12_CLASSIFY_SPAN
  H12Token tokens[HZ12_TOKEN_COUNT];
#else
  H12SpanCurrent current[HZ12_CLASS_COUNT];
#if HZ12_RETURNED_REFILL_BATCH && HZ12_RETURNED_REFILL_BATCH_PRESSURE_GATE
  uint8_t returned_refill_pressure[HZ12_CLASS_COUNT];
#endif
#if HZ12_RETURNED_REFILL_COLD_SKIP
  uint8_t returned_refill_cold_skip[HZ12_CLASS_COUNT];
#endif
#endif
#if HZ12_FLUSH_OWNER_ROUTE
  uint32_t flush_owner_id;
  uint32_t flush_owner_generation;
  uint32_t flush_owner_refill_tick;
  uint8_t flush_owner_valid;
#endif
  size_t cached_bytes;
  uint64_t malloc_count;
  uint64_t malloc_hit;
  uint64_t refill_count;
  uint64_t free_count;
#if !HZ12_CLASSIFY_SPAN
  uint64_t token_hit;
  uint64_t token_miss;
#else
  uint64_t direct_hit_count;
  uint64_t direct_miss_count;
#endif
  uint64_t overflow_count;
  uint64_t flush_count;
  uint64_t flush_items;
#if HZ12_TRANSFER_CENTRAL_SPAN
  uint64_t refill_from_transfer;
  uint64_t refill_from_central;
  uint64_t refill_from_span;
#endif
} H12ThreadCache;

/* State defined in hz12_thread_cache.c. */
extern HZ12_THREAD_LOCAL H12ThreadCache* hz12_tls;

/* Cold paths (out-of-line). */
H12ThreadCache* hz12_thread_cache_init_slow(void);
void hz12_thread_cache_push_overflow_slow(H12ThreadCache* tc, uint8_t class_id,
                                          void* ptr);
void* hz12_thread_cache_refill(H12ThreadCache* tc, uint8_t class_id);
void hz12_thread_cache_reclaim_checkpoint(void);
void hz12_current_span_pool_dump_stats(void);

/* ---------- hot path (static inline) ---------- */

static inline int hz12_in_resolver(void) {
  return hz12_resolving;
}

static inline H12ThreadCache* hz12_thread_cache_get(void) {
  if (hz12_tls) {
    return hz12_tls;
  }
  return hz12_thread_cache_init_slow();
}

#if !HZ12_CLASSIFY_SPAN
#endif /* !HZ12_CLASSIFY_SPAN */

static inline void* hz12_thread_cache_pop(H12ThreadCache* tc,
                                          uint8_t class_id) {
  if (class_id >= HZ12_CLASS_COUNT) {
    return NULL;
  }
#if HZ12_CACHE_SOA
  uint32_t cnt = tc->class_counts[class_id];
  if (cnt == 0u) {
    return NULL;
  }
  cnt -= 1u;
  tc->class_counts[class_id] = cnt;
#if HZ12_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz12_class_slot_size(class_id);
#endif
  return tc->class_items[class_id][cnt];
#else
  H12ClassCache* cc = &tc->class_cache[class_id];
#if HZ12_CACHE_TOPPTR
  if (cc->top == cc->items) {
    return NULL;
  }
  void* p = *--cc->top;
#if HZ12_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz12_class_slot_size(class_id);
#endif
  return p;
#else
  if (cc->count == 0u) {
    return NULL;
  }
  void* p = cc->items[--cc->count];
#if HZ12_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz12_class_slot_size(class_id);
#endif
  return p;
#endif
#endif /* HZ12_CACHE_SOA */
}

static inline void hz12_thread_cache_push(H12ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
#if HZ12_CACHE_SOA
  uint32_t cnt = tc->class_counts[class_id];
#if HZ12_CACHE_BYTE_ACCOUNTING
  size_t slot = hz12_class_slot_size(class_id);
  /* overflow-safe: subtractive form with a slot<=MAX guard avoids cached_bytes+slot wrap */
  if (cnt < HZ12_CACHE_CAP && slot <= HZ12_MAX_CACHED_BYTES &&
      tc->cached_bytes <= HZ12_MAX_CACHED_BYTES - slot) {
    tc->class_items[class_id][cnt] = ptr;
    tc->class_counts[class_id] = cnt + 1u;
    tc->cached_bytes += slot;
    return;
  }
#else
  if (cnt < HZ12_CACHE_CAP) {
    tc->class_items[class_id][cnt] = ptr;
    tc->class_counts[class_id] = cnt + 1u;
    return;
  }
#endif
  hz12_thread_cache_push_overflow_slow(tc, class_id, ptr);
#else
  H12ClassCache* cc = &tc->class_cache[class_id];
#if HZ12_CACHE_TOPPTR
#if HZ12_CACHE_BYTE_ACCOUNTING
  size_t slot = hz12_class_slot_size(class_id);
  if (cc->top < cc->items + HZ12_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ12_MAX_CACHED_BYTES) {
    *cc->top++ = ptr;
    tc->cached_bytes += slot;
    return;
  }
#else
  if (cc->top < cc->items + HZ12_CACHE_CAP) {
    *cc->top++ = ptr;
    return;
  }
#endif
  hz12_thread_cache_push_overflow_slow(tc, class_id, ptr);
#else
#if HZ12_CACHE_BYTE_ACCOUNTING
  size_t slot = hz12_class_slot_size(class_id);
  if (cc->count < HZ12_CACHE_CAP &&
      tc->cached_bytes + slot <= HZ12_MAX_CACHED_BYTES) {
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += slot;
    return;
  }
#else
  if (cc->count < HZ12_CACHE_CAP) {
    cc->items[cc->count++] = ptr;
    return;
  }
#endif
  hz12_thread_cache_push_overflow_slow(tc, class_id, ptr);
#endif
#endif /* HZ12_CACHE_SOA */
}

#endif /* HZ12_THREAD_CACHE_H */
