#include "hz12_thread_cache.h"
#include "hz12_transfer_cache.h"
#if HZ12_FLUSH_OWNER_ROUTE
#include "hz12_flush_owner_route.h"
#endif
#if HZ12_OWNER_BATCH_COUNT_LEDGER
#include "hz12_owner_batch_count_ledger.h"
#elif HZ12_OWNER_BATCH_LEDGER && HZ12_OWNER_BATCH_LEDGER_ACQUIRE
#include "hz12_owner_batch_ledger.h"
#endif
#if !defined(_WIN32)
#include <pthread.h>
#endif
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
/* ---------- state ---------- */
HZ12_THREAD_LOCAL H12ThreadCache* hz12_tls = NULL;
static void hz12_thread_cache_flush_class(H12ThreadCache* tc, uint8_t class_id);
#if HZ12_OWNER_BATCH_COUNT_LEDGER
static H12OwnerToken hz12_thread_cache_ledger_owner(H12ThreadCache* tc) {
  H12OwnerToken owner = {0u, 0u};
  if (tc && tc->flush_owner_valid) {
    owner.slot = tc->flush_owner_id;
    owner.generation = tc->flush_owner_generation;
  }
  return owner;
}
static void hz12_thread_cache_ledger_acquire(H12ThreadCache* tc, void* ptr) {
  H12OwnerToken owner = hz12_thread_cache_ledger_owner(tc);
  void* items[1] = {ptr};
  if (owner.generation != 0u) {
    (void)h12_owner_batch_count_acquire_range(owner, items, 1u);
  }
}
#if HZ12_SPAN_BUMP_BATCH
static void hz12_thread_cache_ledger_acquire_contiguous(
    H12ThreadCache* tc, char* first, size_t slot, uint32_t count) {
  H12OwnerToken owner = hz12_thread_cache_ledger_owner(tc);
  if (owner.generation != 0u) {
    (void)h12_owner_batch_count_acquire_contiguous(
        owner, first, slot, count);
  }
}
#else
#define hz12_thread_cache_ledger_acquire_contiguous(tc, first, slot, count) \
  ((void)0)
#endif
static void hz12_thread_cache_ledger_reacquire_range(
    H12ThreadCache* tc, void* const* items, uint32_t count) {
  H12OwnerToken owner = hz12_thread_cache_ledger_owner(tc);
  if (owner.generation != 0u) {
    (void)h12_owner_batch_count_acquire_range(owner, items, count);
  }
}
#elif HZ12_OWNER_BATCH_LEDGER && HZ12_OWNER_BATCH_LEDGER_ACQUIRE
static H12OwnerToken hz12_thread_cache_ledger_owner(H12ThreadCache* tc) {
  H12OwnerToken owner = {0u, 0u};
  if (tc && tc->flush_owner_valid) {
    owner.slot = tc->flush_owner_id;
    owner.generation = tc->flush_owner_generation;
  }
  return owner;
}
static void hz12_thread_cache_ledger_acquire(H12ThreadCache* tc, void* ptr) {
  H12OwnerToken owner = hz12_thread_cache_ledger_owner(tc);
  if (owner.generation != 0u) {
    void* items[1] = {ptr};
    (void)h12_owner_batch_ledger_acquire_range(owner, items, 1u);
  }
}
static void hz12_thread_cache_ledger_reacquire_range(
    H12ThreadCache* tc, void* const* items, uint32_t count) {
  H12OwnerToken owner = hz12_thread_cache_ledger_owner(tc);
  if (owner.generation != 0u) {
    (void)h12_owner_batch_ledger_acquire_range(owner, items, count);
  }
}
#if HZ12_SPAN_BUMP_BATCH
static void hz12_thread_cache_ledger_acquire_contiguous(
    H12ThreadCache* tc, char* first, size_t slot, uint32_t count) {
  H12OwnerToken owner = hz12_thread_cache_ledger_owner(tc);
  void* items[HZ12_SPAN_BUMP_BATCH_COUNT];
  uint32_t offset = 0u;
  if (owner.generation == 0u || !first || slot == 0u) return;
  while (offset < count) {
    uint32_t batch = count - offset;
    if (batch > HZ12_SPAN_BUMP_BATCH_COUNT) {
      batch = HZ12_SPAN_BUMP_BATCH_COUNT;
    }
    for (uint32_t i = 0u; i < batch; ++i) {
      items[i] = first + (size_t)(offset + i) * slot;
    }
    (void)h12_owner_batch_ledger_acquire_range(owner, items, batch);
    offset += batch;
  }
}
#else
#define hz12_thread_cache_ledger_acquire_contiguous(tc, first, slot, count) \
  ((void)0)
#endif
#else
#define hz12_thread_cache_ledger_acquire(tc, ptr) ((void)0)
#define hz12_thread_cache_ledger_acquire_contiguous(tc, first, slot, count) \
  ((void)0)
#define hz12_thread_cache_ledger_reacquire_range(tc, items, count) ((void)0)
#endif
#if defined(_WIN32) && HZ12_FLUSH_OWNER_COLD_SPAN
static INIT_ONCE hz12_thread_cache_fls_once = INIT_ONCE_STATIC_INIT;
static DWORD hz12_thread_cache_fls_index = FLS_OUT_OF_INDEXES;
static VOID CALLBACK hz12_thread_cache_fls_destroy(PVOID value) {
  H12ThreadCache* tc = (H12ThreadCache*)value;
  if (!tc) return;
  if (hz12_tls == tc) hz12_tls = NULL;
  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    hz12_thread_cache_flush_class(tc, (uint8_t)class_id);
  }
  hz12_flush_owner_route_drain(tc);
  hz12_flush_owner_route_detach(tc);
  hz12_sys_free(tc);
}

static BOOL CALLBACK hz12_thread_cache_fls_init(PINIT_ONCE once,
                                                 PVOID parameter,
                                                 PVOID* context) {
  (void)once;
  (void)parameter;
  (void)context;
  hz12_thread_cache_fls_index = FlsAlloc(hz12_thread_cache_fls_destroy);
  return hz12_thread_cache_fls_index != FLS_OUT_OF_INDEXES;
}
#endif

#if HZ12_CURRENT_SPAN_THREAD_EXIT && HZ12_TRANSFER_CENTRAL_SPAN && \
    HZ12_CLASSIFY_SPAN
#ifndef HZ12_CURRENT_SPAN_POOL_CAP
#define HZ12_CURRENT_SPAN_POOL_CAP 4096u
#endif

typedef struct H12CurrentSpanPoolEntry {
  char* base;
  uint32_t bump_index;
  uint32_t slot_count;
} H12CurrentSpanPoolEntry;

typedef struct H12CurrentSpanPool {
  pthread_mutex_t lock;
  H12CurrentSpanPoolEntry entries[HZ12_CURRENT_SPAN_POOL_CAP];
  uint32_t count;
} H12CurrentSpanPool;

static H12CurrentSpanPool hz12_current_span_pool[HZ12_CLASS_COUNT];
static pthread_once_t hz12_current_span_pool_once = PTHREAD_ONCE_INIT;
static pthread_once_t hz12_thread_cache_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t hz12_thread_cache_key;
static _Atomic uint64_t hz12_current_span_pool_push_count;
static _Atomic uint64_t hz12_current_span_pool_pop_count;
static _Atomic uint64_t hz12_current_span_pool_drop_count;

static void hz12_thread_cache_destroy(void* value);

static void hz12_current_span_pool_init_once(void) {
  for (uint32_t i = 0u; i < HZ12_CLASS_COUNT; ++i) {
    (void)pthread_mutex_init(&hz12_current_span_pool[i].lock, NULL);
  }
}

static void hz12_thread_cache_key_init_once(void) {
  (void)pthread_key_create(&hz12_thread_cache_key, hz12_thread_cache_destroy);
}

static void hz12_current_span_pool_push(uint8_t class_id,
                                        H12SpanCurrent* current) {
  if (class_id >= HZ12_CLASS_COUNT || !current->base ||
      current->bump_index >= current->slot_count) {
    return;
  }
  (void)pthread_once(&hz12_current_span_pool_once,
                     hz12_current_span_pool_init_once);
  H12CurrentSpanPool* pool = &hz12_current_span_pool[class_id];
  pthread_mutex_lock(&pool->lock);
  if (pool->count < HZ12_CURRENT_SPAN_POOL_CAP) {
    H12CurrentSpanPoolEntry* entry = &pool->entries[pool->count++];
    entry->base = current->base;
    entry->bump_index = current->bump_index;
    entry->slot_count = current->slot_count;
    atomic_fetch_add_explicit(&hz12_current_span_pool_push_count, 1u,
                              memory_order_relaxed);
  } else {
    atomic_fetch_add_explicit(&hz12_current_span_pool_drop_count, 1u,
                              memory_order_relaxed);
  }
  pthread_mutex_unlock(&pool->lock);
  current->base = NULL;
  current->bump_index = 0u;
  current->slot_count = 0u;
}

static int hz12_current_span_pool_pop(uint8_t class_id,
                                      H12SpanCurrent* current) {
  if (class_id >= HZ12_CLASS_COUNT || !current) {
    return 0;
  }
  (void)pthread_once(&hz12_current_span_pool_once,
                     hz12_current_span_pool_init_once);
  H12CurrentSpanPool* pool = &hz12_current_span_pool[class_id];
  pthread_mutex_lock(&pool->lock);
  if (pool->count == 0u) {
    pthread_mutex_unlock(&pool->lock);
    return 0;
  }
  H12CurrentSpanPoolEntry entry = pool->entries[--pool->count];
  pthread_mutex_unlock(&pool->lock);
  current->base = entry.base;
  current->bump_index = entry.bump_index;
  current->slot_count = entry.slot_count;
  atomic_fetch_add_explicit(&hz12_current_span_pool_pop_count, 1u,
                            memory_order_relaxed);
  return 1;
}

static void hz12_thread_cache_destroy(void* value) {
  H12ThreadCache* tc = (H12ThreadCache*)value;
  if (!tc) {
    return;
  }
  if (hz12_tls == tc) {
    hz12_tls = NULL;
  }
  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    hz12_thread_cache_flush_class(tc, (uint8_t)class_id);
    hz12_current_span_pool_push((uint8_t)class_id, &tc->current[class_id]);
  }
  hz12_sys_free(tc);
}

void hz12_current_span_pool_dump_stats(void) {
  uint64_t push = atomic_load_explicit(&hz12_current_span_pool_push_count,
                                       memory_order_relaxed);
  uint64_t pop = atomic_load_explicit(&hz12_current_span_pool_pop_count,
                                      memory_order_relaxed);
  uint64_t drop = atomic_load_explicit(&hz12_current_span_pool_drop_count,
                                       memory_order_relaxed);
  fprintf(stderr,
          "hz12_current_span_pool push=%llu pop=%llu drop=%llu cap=%u\n",
          (unsigned long long)push, (unsigned long long)pop,
          (unsigned long long)drop, (unsigned)HZ12_CURRENT_SPAN_POOL_CAP);
}
#else
void hz12_current_span_pool_dump_stats(void) {}
#endif

/* ---------- TLS cache init + slow paths ---------- */

H12ThreadCache* hz12_thread_cache_init_slow(void) {
  void* raw = hz12_sys_malloc(sizeof(H12ThreadCache));
  if (!raw) {
    return NULL;
  }
  H12ThreadCache* tc = (H12ThreadCache*)raw;
  memset(tc, 0, sizeof(*tc));
#if HZ12_CLASSIFY_SPAN
  hz12_span_init(); /* ensure the arena is mapped before any span carve */
#endif
  hz12_tls = tc;
#if HZ12_FLUSH_OWNER_ROUTE
  hz12_flush_owner_route_attach(tc);
#endif
#if defined(_WIN32) && HZ12_FLUSH_OWNER_COLD_SPAN
  if (!InitOnceExecuteOnce(&hz12_thread_cache_fls_once,
                           hz12_thread_cache_fls_init, NULL, NULL) ||
      !FlsSetValue(hz12_thread_cache_fls_index, tc)) {
    hz12_flush_owner_route_detach(tc);
  }
#endif
#if HZ12_CURRENT_SPAN_THREAD_EXIT && HZ12_TRANSFER_CENTRAL_SPAN && \
    HZ12_CLASSIFY_SPAN
  (void)pthread_once(&hz12_thread_cache_key_once,
                     hz12_thread_cache_key_init_once);
  (void)pthread_setspecific(hz12_thread_cache_key, tc);
#endif
#if HZ12_CACHE_TOPPTR
  for (uint32_t c = 0u; c < HZ12_CLASS_COUNT; ++c) {
    tc->class_cache[c].top = tc->class_cache[c].items;
  }
#endif
  return tc;
}

#if HZ12_CLASSIFY_SPAN && (HZ12_SPAN_BUMP_BATCH || HZ12_MATRIX_ATTRIB_DIAG)
static uint32_t hz12_thread_cache_cached_count(H12ThreadCache* tc,
                                               uint8_t class_id) {
  if (!tc || class_id >= HZ12_CLASS_COUNT) {
    return 0u;
  }
#if HZ12_CACHE_SOA
  return tc->class_counts[class_id];
#else
#if HZ12_CACHE_TOPPTR
  H12ClassCache* cc = &tc->class_cache[class_id];
  return (uint32_t)(cc->top - cc->items);
#else
  return tc->class_cache[class_id].count;
#endif
#endif
}
#endif

#if HZ12_CLASSIFY_SPAN && HZ12_RETURNED_PUSH_RANGE
/* Diagnostic inert build: keep the returned-range implementation in the
 * binary, but execute the old per-object publication path. This separates
 * code-layout effects from returned-range behavior without touching speed rows. */
#ifndef HZ12_RETURNED_PUSH_RANGE_INERT
#define HZ12_RETURNED_PUSH_RANGE_INERT 0
#endif
static volatile int hz12_returned_push_range_inert =
    HZ12_RETURNED_PUSH_RANGE_INERT;

static void hz12_thread_cache_flush_span_items_legacy(uint8_t class_id,
                                                       void** items,
                                                       uint32_t count) {
  for (uint32_t i = 0u; i < count; ++i) {
    if (hz12_arena_contains(items[i])) {
      hz12_returned_push(class_id, items[i]);
    } else {
      hz12_sys_free(items[i]);
    }
  }
}

static void hz12_thread_cache_flush_span_items(uint8_t class_id,
                                                void** items,
                                                uint32_t count) {
  void* returned[HZ12_CACHE_CAP];
  uint32_t returned_count = 0u;
  for (uint32_t i = 0u; i < count; ++i) {
    if (hz12_arena_contains(items[i])) {
      returned[returned_count++] = items[i];
    } else {
      hz12_sys_free(items[i]);
    }
  }
  if (returned_count > 0u) {
    hz12_returned_push_range(class_id, returned, returned_count);
  }
}
#endif

static void hz12_thread_cache_flush_class(H12ThreadCache* tc, uint8_t class_id) {
#if HZ12_FLUSH_OWNER_ROUTE && HZ12_CLASSIFY_SPAN
  void** route_items;
  uint32_t route_count;
#if HZ12_CACHE_SOA
  route_items = tc->class_items[class_id];
  route_count = tc->class_counts[class_id];
#else
  H12ClassCache* route_cache = &tc->class_cache[class_id];
  route_items = route_cache->items;
#if HZ12_CACHE_TOPPTR
  route_count = (uint32_t)(route_cache->top - route_cache->items);
#else
  route_count = route_cache->count;
#endif
#endif
  HZ12_COUNT_ADD(tc->flush_items, route_count);
#if HZ12_FLUSH_OWNER_ROUTE_INERT
  for (uint32_t i = 0u; i < route_count; ++i) {
    if (hz12_arena_contains(route_items[i])) {
      hz12_returned_push(class_id, route_items[i]);
    } else {
      hz12_sys_free(route_items[i]);
    }
  }
#else
  hz12_flush_owner_route_batch(tc, class_id, route_items, route_count);
#endif
#if HZ12_CACHE_SOA
  tc->class_counts[class_id] = 0u;
#else
#if HZ12_CACHE_TOPPTR
  route_cache->top = route_cache->items;
#else
  route_cache->count = 0u;
#endif
#endif
#if HZ12_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz12_class_slot_size(class_id) * route_count;
#endif
  HZ12_COUNT_INC(tc->flush_count);
  return;
#endif
#if HZ12_CACHE_SOA
#if HZ12_TRANSFER_CENTRAL_SPAN && HZ12_CLASSIFY_SPAN
  /* Transfer lane: batch-move all cached objects into the transfer cache,
   * spilling any excess to the central stack. Never sys_free arena pointers. */
  uint32_t n = tc->class_counts[class_id];
  HZ12_COUNT_ADD(tc->flush_items, n);
  if (n > 0u) {
    uint32_t inserted = hz12_transfer_insert_range(class_id,
        tc->class_items[class_id], n);
    if (inserted < n) {
      hz12_central_stack_insert_range(class_id,
          tc->class_items[class_id] + inserted, n - inserted);
    }
  }
  tc->class_counts[class_id] = 0u;
#if HZ12_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz12_class_slot_size(class_id) * n;
#endif
#else
  uint32_t n = tc->class_counts[class_id];
  HZ12_COUNT_ADD(tc->flush_items, n);
#if HZ12_RETURNED_PUSH_RANGE
  if (hz12_returned_push_range_inert) {
    hz12_thread_cache_flush_span_items_legacy(class_id,
                                               tc->class_items[class_id], n);
  } else {
    hz12_thread_cache_flush_span_items(class_id, tc->class_items[class_id], n);
  }
#else
  for (uint32_t i = 0u; i < n; ++i) {
#if HZ12_CLASSIFY_SPAN
    if (hz12_arena_contains(tc->class_items[class_id][i])) {
      hz12_returned_push(class_id, tc->class_items[class_id][i]);
    } else {
      hz12_sys_free(tc->class_items[class_id][i]);
    }
#else
    hz12_sys_free(tc->class_items[class_id][i]);
#endif
    tc->class_items[class_id][i] = NULL;
  }
#endif
  tc->class_counts[class_id] = 0u;
#if HZ12_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz12_class_slot_size(class_id) * n;
#endif
#endif /* HZ12_TRANSFER_CENTRAL_SPAN */
#else
  H12ClassCache* cc = &tc->class_cache[class_id];
  size_t slot = hz12_class_slot_size(class_id);
#if HZ12_CACHE_TOPPTR
  void** p = cc->items;
  uint32_t n = (uint32_t)(cc->top - cc->items);
  HZ12_COUNT_ADD(tc->flush_items, n);
#if HZ12_RETURNED_PUSH_RANGE
  if (hz12_returned_push_range_inert) {
    hz12_thread_cache_flush_span_items_legacy(class_id, cc->items, n);
  } else {
    hz12_thread_cache_flush_span_items(class_id, cc->items, n);
  }
#else
  while (p < cc->top) {
#if HZ12_CLASSIFY_SPAN
    if (hz12_arena_contains(*p)) {
      hz12_returned_push(class_id, *p);
    } else {
      hz12_sys_free(*p);
    }
#else
    hz12_sys_free(*p);
#endif
    ++p;
  }
#endif
  cc->top = cc->items;
  tc->cached_bytes -= slot * n;
#else
  HZ12_COUNT_ADD(tc->flush_items, cc->count);
#if HZ12_RETURNED_PUSH_RANGE
  if (hz12_returned_push_range_inert) {
    hz12_thread_cache_flush_span_items_legacy(class_id, cc->items, cc->count);
  } else {
    hz12_thread_cache_flush_span_items(class_id, cc->items, cc->count);
  }
#else
  for (uint32_t i = 0; i < cc->count; ++i) {
#if HZ12_CLASSIFY_SPAN
    if (hz12_arena_contains(cc->items[i])) {
      hz12_returned_push(class_id, cc->items[i]);
    } else {
      hz12_sys_free(cc->items[i]);
    }
#else
    hz12_sys_free(cc->items[i]);
#endif
    cc->items[i] = NULL;
  }
#endif
  tc->cached_bytes -= slot * cc->count;
  cc->count = 0;
#endif
#endif /* HZ12_CACHE_SOA */
  HZ12_COUNT_INC(tc->flush_count);
}

void hz12_thread_cache_push_overflow_slow(H12ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  HZ12_COUNT_INC(tc->overflow_count);
  HZ12_CLASS_DIAG_OVERFLOW(class_id);
  hz12_thread_cache_flush_class(tc, class_id);
#if HZ12_CLASSIFY_SPAN && HZ12_RETURNED_REFILL_COLD_SKIP
  if (class_id < HZ12_CLASS_COUNT) {
    tc->returned_refill_cold_skip[class_id] = 0u;
  }
#endif
  if (class_id < HZ12_CLASS_COUNT) {
#if HZ12_CACHE_SOA
    tc->class_items[class_id][0] = ptr;
    tc->class_counts[class_id] = 1u;
#if HZ12_CACHE_BYTE_ACCOUNTING
    tc->cached_bytes += hz12_class_slot_size(class_id);
#endif
#else
    H12ClassCache* cc = &tc->class_cache[class_id];
#if HZ12_CACHE_TOPPTR
    *cc->top++ = ptr;
    tc->cached_bytes += hz12_class_slot_size(class_id);
#else
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += hz12_class_slot_size(class_id);
#endif
#endif /* HZ12_CACHE_SOA */
  } else {
    hz12_sys_free(ptr);
  }
}

void hz12_thread_cache_reclaim_checkpoint(void) {
  H12ThreadCache* tc = hz12_tls;
  uint32_t class_id;
  if (!tc) return;
  for (class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    hz12_thread_cache_flush_class(tc, (uint8_t)class_id);
#if HZ12_CLASSIFY_SPAN
    tc->current[class_id].base = NULL;
    tc->current[class_id].bump_index = 0u;
    tc->current[class_id].slot_count = 0u;
#endif
  }
}

void* hz12_thread_cache_refill(H12ThreadCache* tc, uint8_t class_id) {
#if HZ12_FLUSH_OWNER_ROUTE && !HZ12_FLUSH_OWNER_ROUTE_INERT && \
    !HZ12_FLUSH_OWNER_COLD_SPAN
  tc->flush_owner_refill_tick += 1u;
  if ((tc->flush_owner_refill_tick & 255u) == 0u) {
    hz12_flush_owner_route_drain(tc);
  }
#endif
#if !HZ12_CLASSIFY_SPAN && !HZ12_ENABLE_HOT_COUNTERS
  (void)tc; /* token lane: tc only used for the now-compiled-out refill_count */
#endif
  HZ12_COUNT_INC(tc->refill_count);
  HZ12_CLASS_DIAG_REFILL(class_id);
  HZ12_MATRIX_DIAG_CACHE_AT_REFILL(class_id,
                                   hz12_thread_cache_cached_count(tc, class_id));
#if HZ12_TRANSFER_CENTRAL_SPAN && HZ12_CLASSIFY_SPAN
  /* Transfer lane: batch refill from transfer cache -> central stack -> span.
   * One mutex lock per batch (vs per-object returned_pop). */
  void* tmp[HZ12_TRANSFER_BATCH];
  uint32_t n = hz12_transfer_remove_range(class_id, tmp, HZ12_TRANSFER_BATCH);
  if (n > 0u) {
    hz12_span_source_diag_transfer_refill(class_id, 1u);
    HZ12_COUNT_INC(tc->refill_from_transfer);
  } else {
    hz12_span_source_diag_transfer_refill(class_id, 0u);
    n = hz12_central_stack_remove_range(class_id, tmp, HZ12_TRANSFER_BATCH);
    if (n > 0u) {
      hz12_span_source_diag_central_refill(class_id, 1u);
      HZ12_COUNT_INC(tc->refill_from_central);
    } else {
      hz12_span_source_diag_central_refill(class_id, 0u);
      size_t slot = hz12_class_slot_size(class_id);
      H12SpanCurrent* cs = &tc->current[class_id];
      while (n < HZ12_TRANSFER_BATCH) {
        if (!cs->base || cs->bump_index >= cs->slot_count) {
#if HZ12_FLUSH_OWNER_COLD_SPAN
          void* routed = hz12_flush_owner_route_drain_for_class(tc, class_id);
          if (routed) {
            tmp[n++] = routed;
            continue;
          }
#endif
          if (cs->base && cs->bump_index >= cs->slot_count) {
            hz12_span_source_diag_current_exhaust(class_id);
          }
          char* base = NULL;
#if HZ12_CURRENT_SPAN_THREAD_EXIT
          if (hz12_current_span_pool_pop(class_id, cs)) {
            hz12_span_source_diag_span_reuse(class_id);
            hz12_span_source_diag_current_replace(class_id);
            continue;
          }
#endif
          base = (char*)hz12_span_return_pop_reusable_span(class_id);
          if (!base) {
            base = (char*)hz12_span_carve_for_class(class_id);
            if (base) {
              hz12_span_source_diag_arena_carve(class_id);
            }
          } else {
            hz12_span_source_diag_span_reuse(class_id);
          }
          if (!base) {
            break;
          }
          hz12_span_source_diag_current_replace(class_id);
          cs->base = base;
          cs->bump_index = 0u;
          cs->slot_count = (uint32_t)(HZ12_SPAN_BYTES / slot);
          hz12_span_return_register_active_span(class_id, base, cs->slot_count);
#if HZ12_FLUSH_OWNER_COLD_SPAN
          hz12_flush_owner_route_assign_span(tc, base);
#endif
        }
        tmp[n++] = cs->base + (size_t)cs->bump_index++ * slot;
      }
      if (n > 0u) {
        HZ12_COUNT_INC(tc->refill_from_span);
      }
    }
  }
  if (n == 0u) {
    return hz12_sys_malloc(hz12_class_slot_size(class_id)); /* arena full fallback */
  }
  /* push tmp[1..n-1] into thread cache, return tmp[0] */
  for (uint32_t i = 1u; i < n; ++i) {
    hz12_thread_cache_push(tc, class_id, tmp[i]);
  }
  return tmp[0];
#elif HZ12_CLASSIFY_SPAN
  size_t slot = hz12_class_slot_size(class_id);
  H12SpanCurrent* cs = &tc->current[class_id];
#if HZ12_FLUSH_OWNER_COLD_SPAN
  if (!cs->base || cs->bump_index >= cs->slot_count) {
    void* routed = hz12_flush_owner_route_drain_for_class(tc, class_id);
    if (routed) return routed;
  }
#endif
#if HZ12_RETURNED_REFILL_COLD_SKIP
  if (class_id < HZ12_CLASS_COUNT &&
      tc->returned_refill_cold_skip[class_id] > 0u && cs->base &&
      cs->bump_index < cs->slot_count) {
    tc->returned_refill_cold_skip[class_id]--;
    HZ12_MATRIX_DIAG_CURRENT_HIT(class_id);
    return cs->base + (size_t)cs->bump_index++ * slot;
  }
#endif
  /* 1. per-class returned-object sink first (reuse before carving a fresh span) */
#if HZ12_RETURNED_REFILL_BATCH
  if (class_id >= HZ12_RETURNED_REFILL_BATCH_MIN_CLASS &&
      class_id <= HZ12_RETURNED_REFILL_BATCH_MAX_CLASS) {
#if HZ12_RETURNED_REFILL_BATCH_PRESSURE_GATE
    uint8_t pressure = tc->returned_refill_pressure[class_id];
    if (pressure >= HZ12_RETURNED_REFILL_BATCH_PRESSURE_THRESHOLD) {
#endif
    void* tmp[HZ12_RETURNED_REFILL_BATCH_COUNT];
    uint32_t n = hz12_returned_pop_range(class_id, tmp,
                                         HZ12_RETURNED_REFILL_BATCH_COUNT);
    HZ12_MATRIX_DIAG_RETURNED_BATCH(class_id, n);
    if (n > 0u) {
      hz12_thread_cache_ledger_reacquire_range(tc, tmp, n);
      for (uint32_t i = 1u; i < n; ++i) {
        hz12_thread_cache_push(tc, class_id, tmp[i]);
      }
      HZ12_MATRIX_DIAG_CACHE_AFTER_BATCH(
          class_id, hz12_thread_cache_cached_count(tc, class_id));
#if HZ12_RETURNED_REFILL_BATCH_PRESSURE_GATE
      tc->returned_refill_pressure[class_id] = 0u;
#endif
      return tmp[0];
    }
#if HZ12_RETURNED_REFILL_COLD_SKIP
    if (cs->base && cs->bump_index < cs->slot_count) {
      tc->returned_refill_cold_skip[class_id] =
          (uint8_t)HZ12_RETURNED_REFILL_COLD_SKIP_BUDGET;
    }
#endif
#if HZ12_RETURNED_REFILL_BATCH_PRESSURE_GATE
    if (pressure < 255u) {
      tc->returned_refill_pressure[class_id] = (uint8_t)(pressure + 1u);
    }
    } else {
      void* reused = hz12_returned_pop(class_id);
      HZ12_MATRIX_DIAG_RETURNED_ONE(class_id, reused != NULL);
      if (reused) {
#if HZ12_OWNER_BATCH_COUNT_LEDGER || \
    (HZ12_OWNER_BATCH_LEDGER && HZ12_OWNER_BATCH_LEDGER_ACQUIRE)
        void* reacquired[1] = {reused};
        hz12_thread_cache_ledger_reacquire_range(tc, reacquired, 1u);
#endif
        if (pressure > 0u) {
          tc->returned_refill_pressure[class_id] = (uint8_t)(pressure - 1u);
        }
        return reused;
      }
#if HZ12_RETURNED_REFILL_COLD_SKIP
      if (cs->base && cs->bump_index < cs->slot_count) {
        tc->returned_refill_cold_skip[class_id] =
            (uint8_t)HZ12_RETURNED_REFILL_COLD_SKIP_BUDGET;
      }
#endif
      if (pressure < 255u) {
        tc->returned_refill_pressure[class_id] = (uint8_t)(pressure + 1u);
      }
    }
#endif
  } else
#endif
  {
    void* reused = hz12_returned_pop(class_id);
    HZ12_MATRIX_DIAG_RETURNED_ONE(class_id, reused != NULL);
    if (reused) {
#if HZ12_OWNER_BATCH_COUNT_LEDGER || \
    (HZ12_OWNER_BATCH_LEDGER && HZ12_OWNER_BATCH_LEDGER_ACQUIRE)
      void* reacquired[1] = {reused};
      hz12_thread_cache_ledger_reacquire_range(tc, reacquired, 1u);
#endif
      return reused;
    }
#if HZ12_RETURNED_REFILL_COLD_SKIP
    if (class_id < HZ12_CLASS_COUNT && cs->base &&
        cs->bump_index < cs->slot_count) {
      tc->returned_refill_cold_skip[class_id] =
          (uint8_t)HZ12_RETURNED_REFILL_COLD_SKIP_BUDGET;
    }
#endif
  }
  /* 2. bump from the per-thread current span */
  if (cs->base && cs->bump_index < cs->slot_count) {
#if HZ12_SPAN_BUMP_BATCH
    uint32_t remaining = cs->slot_count - cs->bump_index;
    uint32_t batch = HZ12_SPAN_BUMP_BATCH_COUNT;
    uint32_t cached = hz12_thread_cache_cached_count(tc, class_id);
    uint32_t cache_room = cached < HZ12_CACHE_CAP
        ? (uint32_t)HZ12_CACHE_CAP - cached : 0u;
    if (batch > remaining) {
      batch = remaining;
    }
    /* Keep one object for the caller; only the remainder enters the cache. */
    if (batch > cache_room + 1u) {
      batch = cache_room + 1u;
    }
#if HZ12_CACHE_BYTE_ACCOUNTING
    {
      size_t available = tc->cached_bytes < HZ12_MAX_CACHED_BYTES
          ? HZ12_MAX_CACHED_BYTES - tc->cached_bytes : 0u;
      size_t slot_bytes = hz12_class_slot_size(class_id);
      uint32_t byte_room = slot_bytes != 0u
          ? (uint32_t)(available / slot_bytes) : 0u;
      if (batch > byte_room + 1u) {
        batch = byte_room + 1u;
      }
    }
#endif
    if (batch > 1u) {
      void* first = cs->base + (size_t)cs->bump_index * slot;
      hz12_thread_cache_ledger_acquire_contiguous(
          tc, (char*)first, slot, batch);
      cs->bump_index += batch;
      for (uint32_t i = 1u; i < batch; ++i) {
        hz12_thread_cache_push(tc, class_id,
            cs->base + (size_t)(cs->bump_index - batch + i) * slot);
      }
      HZ12_MATRIX_DIAG_BUMP_BATCH(class_id, batch);
      HZ12_MATRIX_DIAG_CURRENT_HIT(class_id);
      return first;
    }
#endif
    HZ12_MATRIX_DIAG_CURRENT_HIT(class_id);
    {
      void* result = cs->base + (size_t)cs->bump_index++ * slot;
      hz12_thread_cache_ledger_acquire(tc, result);
      return result;
    }
  }
  /* 3. current span exhausted (or none yet): carve a fresh 64 KiB span */
  char* base = (char*)hz12_span_carve_for_class(class_id);
  if (!base) {
    HZ12_MATRIX_DIAG_SYS_FALLBACK(class_id);
    return hz12_sys_malloc(slot); /* arena full -- fall back (bench never hits) */
  }
  HZ12_MATRIX_DIAG_SPAN_NEW(class_id);
  cs->base = base;
  cs->bump_index = 0;
  cs->slot_count = (uint32_t)(HZ12_SPAN_BYTES / slot);
#if HZ12_FLUSH_OWNER_COLD_SPAN
  hz12_flush_owner_route_assign_span(tc, base);
#endif
  {
    void* result = cs->base + (size_t)cs->bump_index++ * slot;
    hz12_thread_cache_ledger_acquire(tc, result);
    return result;
  }
#else
  return hz12_sys_malloc(hz12_class_slot_size(class_id));
#endif
}
