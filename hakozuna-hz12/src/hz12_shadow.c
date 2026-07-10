#include "hz12_shadow.h"

#include "hz12_span.h"

#include <stdatomic.h>
#include <string.h>

#ifndef HZ12_SHADOW_DIAG_COUNTERS
#define HZ12_SHADOW_DIAG_COUNTERS 1
#endif

#ifndef HZ12_SHADOW_OWNER_FAST_LOAD
#define HZ12_SHADOW_OWNER_FAST_LOAD 0
#endif

typedef struct H12ShadowCounters {
  _Atomic uint64_t alloc_span_seen;
  _Atomic uint64_t alloc_owner_first;
  _Atomic uint64_t alloc_owner_reuse_foreign;
  _Atomic uint64_t flush_objects_total;
  _Atomic uint64_t flush_owner_local;
  _Atomic uint64_t flush_owner_foreign;
  _Atomic uint64_t flush_owner_unknown;
  _Atomic uint64_t would_route_batches;
  _Atomic uint64_t would_route_objects;
  _Atomic uint64_t would_keep_ownerless_overflow;
  _Atomic uint64_t projected_orphan_objects;
  _Atomic uint64_t projected_reclaim_blocked_spans;
  _Atomic uint64_t projected_inbox_current_max;
} H12ShadowCounters;

static _Atomic uint64_t h12_span_owner[HZ12_SPAN_COUNT];
static _Atomic uint8_t h12_span_foreign_seen[HZ12_SPAN_COUNT];
static _Atomic uint32_t h12_owner_inbox_current[HZ12_SHADOW_MAX_OWNERS];
static H12ShadowCounters h12_counters;
static uint32_t h12_owner_count;

static int h12_span_id(const void* ptr, uint32_t* out_id) {
  uintptr_t p;
  uintptr_t base;
  uintptr_t off;
  if (!ptr || !out_id || !hz12_arena_base) {
    return 0;
  }
  p = (uintptr_t)ptr;
  base = (uintptr_t)hz12_arena_base;
  if (p < base) {
    return 0;
  }
  off = p - base;
  if (off >= (uintptr_t)HZ12_ARENA_BYTES) {
    return 0;
  }
  *out_id = (uint32_t)(off >> HZ12_SPAN_SHIFT);
  return 1;
}

#if HZ12_SHADOW_DIAG_COUNTERS
static void h12_atomic_max(_Atomic uint64_t* destination, uint64_t value) {
  uint64_t current = atomic_load_explicit(destination, memory_order_relaxed);
  while (current < value &&
         !atomic_compare_exchange_weak_explicit(destination, &current, value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}
#endif

#if HZ12_SHADOW_DIAG_COUNTERS
#define H12_SHADOW_COUNTER_ADD(field, value)                              \
  atomic_fetch_add_explicit(&h12_counters.field, (value), memory_order_relaxed)
#else
#define H12_SHADOW_COUNTER_ADD(field, value) ((void)0)
#endif

int h12_shadow_init(uint32_t owner_count) {
  if (owner_count == 0u || owner_count > HZ12_SHADOW_MAX_OWNERS) {
    return 0;
  }
  h12_owner_count = owner_count;
  h12_shadow_reset();
  return 1;
}

void h12_shadow_reset(void) {
  uint32_t i;
  for (i = 0u; i < HZ12_SPAN_COUNT; ++i) {
    atomic_store_explicit(&h12_span_owner[i], 0u, memory_order_relaxed);
    atomic_store_explicit(&h12_span_foreign_seen[i], 0u, memory_order_relaxed);
  }
  for (i = 0u; i < HZ12_SHADOW_MAX_OWNERS; ++i) {
    atomic_store_explicit(&h12_owner_inbox_current[i], 0u,
                          memory_order_relaxed);
  }
  atomic_store_explicit(&h12_counters.alloc_span_seen, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_counters.alloc_owner_first, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_counters.alloc_owner_reuse_foreign, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.flush_objects_total, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.flush_owner_local, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_counters.flush_owner_foreign, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.flush_owner_unknown, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.would_route_batches, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.would_route_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.would_keep_ownerless_overflow, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.projected_orphan_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.projected_reclaim_blocked_spans, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_counters.projected_inbox_current_max, 0u,
                        memory_order_relaxed);
}

void h12_shadow_on_alloc_token(void* ptr, uint32_t owner_id,
                               uint32_t generation) {
  uint32_t span_id;
  uint64_t expected;
  uint64_t token;
  if (owner_id >= h12_owner_count || generation == 0u ||
      !h12_span_id(ptr, &span_id)) {
    return;
  }
  token = ((uint64_t)generation << 32) | (uint64_t)(owner_id + 1u);
  H12_SHADOW_COUNTER_ADD(alloc_span_seen, 1u);
#if HZ12_SHADOW_OWNER_FAST_LOAD
  expected = atomic_load_explicit(&h12_span_owner[span_id],
                                  memory_order_relaxed);
  if (expected == token) {
    return;
  }
  if (expected != 0u) {
    H12_SHADOW_COUNTER_ADD(alloc_owner_reuse_foreign, 1u);
    return;
  }
#endif
  expected = 0u;
  if (atomic_compare_exchange_strong_explicit(&h12_span_owner[span_id],
                                              &expected, token,
                                              memory_order_relaxed,
                                              memory_order_relaxed)) {
    H12_SHADOW_COUNTER_ADD(alloc_owner_first, 1u);
  } else if (expected != token) {
    H12_SHADOW_COUNTER_ADD(alloc_owner_reuse_foreign, 1u);
  }
}

void h12_shadow_on_alloc(void* ptr, uint32_t owner_id) {
  h12_shadow_on_alloc_token(ptr, owner_id, 1u);
}

int h12_shadow_owner_token_for_ptr(const void* ptr, uint32_t* owner_id,
                                   uint32_t* generation) {
  uint32_t span_id;
  uint64_t token;
  uint32_t encoded_owner;
  if (!owner_id || !generation || !h12_span_id(ptr, &span_id)) return 0;
  token = atomic_load_explicit(&h12_span_owner[span_id], memory_order_relaxed);
  encoded_owner = (uint32_t)token;
  if (encoded_owner == 0u || encoded_owner > h12_owner_count) return 0;
  *generation = (uint32_t)(token >> 32);
  if (*generation == 0u) return 0;
  *owner_id = encoded_owner - 1u;
  return 1;
}

int h12_shadow_rehome_token(const void* ptr, uint32_t owner_id,
                            uint32_t generation) {
  uint32_t span_id;
  uint64_t token;
  if (owner_id >= h12_owner_count || generation == 0u ||
      !h12_span_id(ptr, &span_id)) {
    return 0;
  }
  token = ((uint64_t)generation << 32) | (uint64_t)(owner_id + 1u);
  atomic_store_explicit(&h12_span_owner[span_id], token,
                        memory_order_release);
  return 1;
}

int h12_shadow_clear_token_if(const void* ptr, uint32_t owner_id,
                              uint32_t generation) {
  uint32_t span_id;
  uint64_t expected;
  if (owner_id >= h12_owner_count || generation == 0u ||
      !h12_span_id(ptr, &span_id)) {
    return 0;
  }
  expected = ((uint64_t)generation << 32) | (uint64_t)(owner_id + 1u);
  return atomic_compare_exchange_strong_explicit(
      &h12_span_owner[span_id], &expected, 0u,
      memory_order_acq_rel, memory_order_acquire);
}

int h12_shadow_owner_for_ptr(const void* ptr, uint32_t* owner_id) {
  uint32_t generation;
  return h12_shadow_owner_token_for_ptr(ptr, owner_id, &generation);
}

int h12_shadow_batch_all_owner(void** items, uint32_t count,
                               uint32_t owner_id) {
  uint32_t span_ids[8];
  uint32_t owners[8];
  uint32_t wanted;
  if (!items || owner_id >= h12_owner_count) return 0;
  for (uint32_t i = 0u; i < 8u; ++i) {
    span_ids[i] = UINT32_MAX;
    owners[i] = 0u;
  }
  wanted = owner_id + 1u;
  for (uint32_t i = 0u; i < count; ++i) {
    uint32_t span_id;
    uint32_t slot;
    uint32_t owner;
    if (!items[i] || !h12_span_id(items[i], &span_id)) return 0;
    slot = span_id & 7u;
    if (span_ids[slot] == span_id) {
      owner = owners[slot];
    } else {
      owner = (uint32_t)atomic_load_explicit(&h12_span_owner[span_id],
                                             memory_order_relaxed);
      span_ids[slot] = span_id;
      owners[slot] = owner;
    }
    if (owner != wanted) return 0;
  }
  return 1;
}

int h12_shadow_batch_all_owner_token(void** items, uint32_t count,
                                     uint32_t owner_id,
                                     uint32_t generation) {
  uint32_t span_ids[8];
  uint64_t tokens[8];
  uint64_t wanted;
  if (!items || owner_id >= h12_owner_count || generation == 0u) return 0;
  for (uint32_t i = 0u; i < 8u; ++i) {
    span_ids[i] = UINT32_MAX;
    tokens[i] = 0u;
  }
  wanted = ((uint64_t)generation << 32) | (uint64_t)(owner_id + 1u);
  for (uint32_t i = 0u; i < count; ++i) {
    uint32_t span_id;
    uint32_t slot;
    uint64_t token;
    if (!items[i] || !h12_span_id(items[i], &span_id)) return 0;
    slot = span_id & 7u;
    if (span_ids[slot] == span_id) {
      token = tokens[slot];
    } else {
      token = atomic_load_explicit(&h12_span_owner[span_id],
                                   memory_order_relaxed);
      span_ids[slot] = span_id;
      tokens[slot] = token;
    }
    if (token != wanted) return 0;
  }
  return 1;
}

void h12_shadow_cache_init(H12ShadowCache* cache, uint32_t consumer_id) {
  if (!cache) {
    return;
  }
  cache->count = 0u;
  cache->consumer_id = consumer_id;
}

static void h12_shadow_project_batch(uint32_t owner_id, uint32_t count) {
#if HZ12_SHADOW_DIAG_COUNTERS
  uint32_t now;
  if (count == 0u || owner_id >= h12_owner_count) {
    return;
  }
  if (count > HZ12_SHADOW_INBOX_CAP) {
    H12_SHADOW_COUNTER_ADD(would_keep_ownerless_overflow, count);
    return;
  }
  H12_SHADOW_COUNTER_ADD(would_route_batches, 1u);
  H12_SHADOW_COUNTER_ADD(would_route_objects, count);
  /* L0 models immediate owner drain. This records a bounded batch peak without
   * creating an actual inbox or changing HZ12 ownership. */
  now = atomic_fetch_add_explicit(&h12_owner_inbox_current[owner_id], count,
                                  memory_order_relaxed) + count;
  h12_atomic_max(&h12_counters.projected_inbox_current_max, now);
  atomic_fetch_sub_explicit(&h12_owner_inbox_current[owner_id], count,
                            memory_order_relaxed);
#else
  (void)owner_id;
  (void)count;
#endif
}

void h12_shadow_flush(H12ShadowCache* cache) {
  uint32_t owner_counts[HZ12_SHADOW_MAX_OWNERS];
  uint32_t i;
  if (!cache || cache->count == 0u) {
    return;
  }
  memset(owner_counts, 0, sizeof(owner_counts));
  for (i = 0u; i < cache->count; ++i) {
    uint32_t span_id;
    uint32_t token;
    if (!h12_span_id(cache->items[i], &span_id)) {
      H12_SHADOW_COUNTER_ADD(flush_owner_unknown, 1u);
      continue;
    }
    H12_SHADOW_COUNTER_ADD(flush_objects_total, 1u);
    token = atomic_load_explicit(&h12_span_owner[span_id], memory_order_relaxed);
    if (token == 0u || token > h12_owner_count) {
      H12_SHADOW_COUNTER_ADD(flush_owner_unknown, 1u);
      H12_SHADOW_COUNTER_ADD(projected_orphan_objects, 1u);
      continue;
    }
    if ((token - 1u) == cache->consumer_id) {
      H12_SHADOW_COUNTER_ADD(flush_owner_local, 1u);
      continue;
    }
    H12_SHADOW_COUNTER_ADD(flush_owner_foreign, 1u);
    owner_counts[token - 1u] += 1u;
#if HZ12_SHADOW_DIAG_COUNTERS
    if (atomic_exchange_explicit(&h12_span_foreign_seen[span_id], 1u,
                                 memory_order_relaxed) == 0u) {
      /* L0 cannot prove a span is wholly free; record the spans that would
       * require a future free-count authority before reclaim is legal. */
      H12_SHADOW_COUNTER_ADD(projected_reclaim_blocked_spans, 1u);
    }
#endif
  }
  for (i = 0u; i < h12_owner_count; ++i) {
    h12_shadow_project_batch(i, owner_counts[i]);
  }
  cache->count = 0u;
}

void h12_shadow_on_free(H12ShadowCache* cache, void* ptr) {
  if (!cache || !ptr) {
    return;
  }
  cache->items[cache->count++] = ptr;
  if (cache->count == HZ12_SHADOW_FLUSH_CAP) {
    h12_shadow_flush(cache);
  }
}

void h12_shadow_dump(FILE* out) {
  if (!out) {
    return;
  }
  fprintf(out,
          "[HZ12_OWNER_SHADOW] alloc_span_seen=%llu alloc_owner_first=%llu "
          "alloc_owner_reuse_foreign=%llu flush_objects_total=%llu "
          "flush_owner_local=%llu flush_owner_foreign=%llu "
          "flush_owner_unknown=%llu would_route_batches=%llu "
          "would_route_objects=%llu would_keep_ownerless_overflow=%llu "
          "projected_inbox_current_max=%llu projected_orphan_objects=%llu "
          "projected_reclaim_blocked_spans=%llu\n",
          (unsigned long long)atomic_load_explicit(&h12_counters.alloc_span_seen,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.alloc_owner_first,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.alloc_owner_reuse_foreign,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.flush_objects_total,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.flush_owner_local,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.flush_owner_foreign,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.flush_owner_unknown,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.would_route_batches,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.would_route_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.would_keep_ownerless_overflow,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.projected_inbox_current_max,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.projected_orphan_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_counters.projected_reclaim_blocked_spans,
                                                    memory_order_relaxed));
}
