#include "hz11_thread_cache.h"
#include "hz11_transfer_cache.h"

#if !defined(_WIN32)
#include <pthread.h>
#endif
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

/* ---------- state ---------- */

HZ11_THREAD_LOCAL H11ThreadCache* hz11_tls = NULL;

static void hz11_thread_cache_flush_class(H11ThreadCache* tc, uint8_t class_id);

#if HZ11_CLASS_DIAG
typedef struct H11ClassDiagCounter {
  _Atomic uint64_t malloc_count;
  _Atomic uint64_t hit_count;
  _Atomic uint64_t refill_count;
  _Atomic uint64_t overflow_count;
  _Atomic uint64_t returned_pop_hit;
  _Atomic uint64_t returned_pop_miss;
} H11ClassDiagCounter;

static H11ClassDiagCounter hz11_class_diag_counters[HZ11_CLASS_COUNT];

static void hz11_class_diag_inc(_Atomic uint64_t* value) {
  atomic_fetch_add_explicit(value, 1u, memory_order_relaxed);
}

void hz11_class_diag_malloc(uint8_t class_id) {
  if (class_id < HZ11_CLASS_COUNT) {
    hz11_class_diag_inc(&hz11_class_diag_counters[class_id].malloc_count);
  }
}

void hz11_class_diag_hit(uint8_t class_id) {
  if (class_id < HZ11_CLASS_COUNT) {
    hz11_class_diag_inc(&hz11_class_diag_counters[class_id].hit_count);
  }
}

void hz11_class_diag_refill(uint8_t class_id) {
  if (class_id < HZ11_CLASS_COUNT) {
    hz11_class_diag_inc(&hz11_class_diag_counters[class_id].refill_count);
  }
}

void hz11_class_diag_overflow(uint8_t class_id) {
  if (class_id < HZ11_CLASS_COUNT) {
    hz11_class_diag_inc(&hz11_class_diag_counters[class_id].overflow_count);
  }
}

void hz11_class_diag_returned_pop_hit(uint8_t class_id) {
  if (class_id < HZ11_CLASS_COUNT) {
    hz11_class_diag_inc(&hz11_class_diag_counters[class_id].returned_pop_hit);
  }
}

void hz11_class_diag_returned_pop_miss(uint8_t class_id) {
  if (class_id < HZ11_CLASS_COUNT) {
    hz11_class_diag_inc(&hz11_class_diag_counters[class_id].returned_pop_miss);
  }
}

void hz11_class_diag_dump_stats(void) {
  for (uint32_t class_id = 0u; class_id < HZ11_CLASS_COUNT; ++class_id) {
    uint64_t malloc_count = atomic_load_explicit(
        &hz11_class_diag_counters[class_id].malloc_count, memory_order_relaxed);
    uint64_t hit_count = atomic_load_explicit(
        &hz11_class_diag_counters[class_id].hit_count, memory_order_relaxed);
    uint64_t refill_count = atomic_load_explicit(
        &hz11_class_diag_counters[class_id].refill_count, memory_order_relaxed);
    uint64_t overflow_count = atomic_load_explicit(
        &hz11_class_diag_counters[class_id].overflow_count, memory_order_relaxed);
    uint64_t returned_pop_hit = atomic_load_explicit(
        &hz11_class_diag_counters[class_id].returned_pop_hit,
        memory_order_relaxed);
    uint64_t returned_pop_miss = atomic_load_explicit(
        &hz11_class_diag_counters[class_id].returned_pop_miss,
        memory_order_relaxed);
    if (malloc_count == 0u && hit_count == 0u && refill_count == 0u &&
        overflow_count == 0u && returned_pop_hit == 0u &&
        returned_pop_miss == 0u) {
      continue;
    }
    fprintf(stdout,
            "[HZ11_CLASS_DIAG] class=%u slot=%zu malloc=%llu hit=%llu "
            "refill=%llu overflow=%llu returned_pop_hit=%llu "
            "returned_pop_miss=%llu\n",
            (unsigned)class_id, hz11_class_slot_size((uint8_t)class_id),
            (unsigned long long)malloc_count,
            (unsigned long long)hit_count,
            (unsigned long long)refill_count,
            (unsigned long long)overflow_count,
            (unsigned long long)returned_pop_hit,
            (unsigned long long)returned_pop_miss);
  }
}
#endif

#if HZ11_CURRENT_SPAN_THREAD_EXIT && HZ11_TRANSFER_CENTRAL_SPAN && \
    HZ11_CLASSIFY_SPAN
#ifndef HZ11_CURRENT_SPAN_POOL_CAP
#define HZ11_CURRENT_SPAN_POOL_CAP 4096u
#endif

typedef struct H11CurrentSpanPoolEntry {
  char* base;
  uint32_t bump_index;
  uint32_t slot_count;
} H11CurrentSpanPoolEntry;

typedef struct H11CurrentSpanPool {
  pthread_mutex_t lock;
  H11CurrentSpanPoolEntry entries[HZ11_CURRENT_SPAN_POOL_CAP];
  uint32_t count;
} H11CurrentSpanPool;

static H11CurrentSpanPool hz11_current_span_pool[HZ11_CLASS_COUNT];
static pthread_once_t hz11_current_span_pool_once = PTHREAD_ONCE_INIT;
static pthread_once_t hz11_thread_cache_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t hz11_thread_cache_key;
static _Atomic uint64_t hz11_current_span_pool_push_count;
static _Atomic uint64_t hz11_current_span_pool_pop_count;
static _Atomic uint64_t hz11_current_span_pool_drop_count;

static void hz11_thread_cache_destroy(void* value);

static void hz11_current_span_pool_init_once(void) {
  for (uint32_t i = 0u; i < HZ11_CLASS_COUNT; ++i) {
    (void)pthread_mutex_init(&hz11_current_span_pool[i].lock, NULL);
  }
}

static void hz11_thread_cache_key_init_once(void) {
  (void)pthread_key_create(&hz11_thread_cache_key, hz11_thread_cache_destroy);
}

static void hz11_current_span_pool_push(uint8_t class_id,
                                        H11SpanCurrent* current) {
  if (class_id >= HZ11_CLASS_COUNT || !current->base ||
      current->bump_index >= current->slot_count) {
    return;
  }
  (void)pthread_once(&hz11_current_span_pool_once,
                     hz11_current_span_pool_init_once);
  H11CurrentSpanPool* pool = &hz11_current_span_pool[class_id];
  pthread_mutex_lock(&pool->lock);
  if (pool->count < HZ11_CURRENT_SPAN_POOL_CAP) {
    H11CurrentSpanPoolEntry* entry = &pool->entries[pool->count++];
    entry->base = current->base;
    entry->bump_index = current->bump_index;
    entry->slot_count = current->slot_count;
    atomic_fetch_add_explicit(&hz11_current_span_pool_push_count, 1u,
                              memory_order_relaxed);
  } else {
    atomic_fetch_add_explicit(&hz11_current_span_pool_drop_count, 1u,
                              memory_order_relaxed);
  }
  pthread_mutex_unlock(&pool->lock);
  current->base = NULL;
  current->bump_index = 0u;
  current->slot_count = 0u;
}

static int hz11_current_span_pool_pop(uint8_t class_id,
                                      H11SpanCurrent* current) {
  if (class_id >= HZ11_CLASS_COUNT || !current) {
    return 0;
  }
  (void)pthread_once(&hz11_current_span_pool_once,
                     hz11_current_span_pool_init_once);
  H11CurrentSpanPool* pool = &hz11_current_span_pool[class_id];
  pthread_mutex_lock(&pool->lock);
  if (pool->count == 0u) {
    pthread_mutex_unlock(&pool->lock);
    return 0;
  }
  H11CurrentSpanPoolEntry entry = pool->entries[--pool->count];
  pthread_mutex_unlock(&pool->lock);
  current->base = entry.base;
  current->bump_index = entry.bump_index;
  current->slot_count = entry.slot_count;
  atomic_fetch_add_explicit(&hz11_current_span_pool_pop_count, 1u,
                            memory_order_relaxed);
  return 1;
}

static void hz11_thread_cache_destroy(void* value) {
  H11ThreadCache* tc = (H11ThreadCache*)value;
  if (!tc) {
    return;
  }
  if (hz11_tls == tc) {
    hz11_tls = NULL;
  }
  for (uint32_t class_id = 0u; class_id < HZ11_CLASS_COUNT; ++class_id) {
    hz11_thread_cache_flush_class(tc, (uint8_t)class_id);
    hz11_current_span_pool_push((uint8_t)class_id, &tc->current[class_id]);
  }
  hz11_sys_free(tc);
}

void hz11_current_span_pool_dump_stats(void) {
  uint64_t push = atomic_load_explicit(&hz11_current_span_pool_push_count,
                                       memory_order_relaxed);
  uint64_t pop = atomic_load_explicit(&hz11_current_span_pool_pop_count,
                                      memory_order_relaxed);
  uint64_t drop = atomic_load_explicit(&hz11_current_span_pool_drop_count,
                                       memory_order_relaxed);
  fprintf(stderr,
          "hz11_current_span_pool push=%llu pop=%llu drop=%llu cap=%u\n",
          (unsigned long long)push, (unsigned long long)pop,
          (unsigned long long)drop, (unsigned)HZ11_CURRENT_SPAN_POOL_CAP);
}
#else
void hz11_current_span_pool_dump_stats(void) {}
#endif

/* ---------- TLS cache init + slow paths ---------- */

H11ThreadCache* hz11_thread_cache_init_slow(void) {
  void* raw = hz11_sys_malloc(sizeof(H11ThreadCache));
  if (!raw) {
    return NULL;
  }
  H11ThreadCache* tc = (H11ThreadCache*)raw;
  memset(tc, 0, sizeof(*tc));
#if HZ11_CLASSIFY_SPAN
  hz11_span_init(); /* ensure the arena is mapped before any span carve */
#endif
  hz11_tls = tc;
#if HZ11_CURRENT_SPAN_THREAD_EXIT && HZ11_TRANSFER_CENTRAL_SPAN && \
    HZ11_CLASSIFY_SPAN
  (void)pthread_once(&hz11_thread_cache_key_once,
                     hz11_thread_cache_key_init_once);
  (void)pthread_setspecific(hz11_thread_cache_key, tc);
#endif
#if HZ11_CACHE_TOPPTR
  for (uint32_t c = 0u; c < HZ11_CLASS_COUNT; ++c) {
    tc->class_cache[c].top = tc->class_cache[c].items;
  }
#endif
  return tc;
}

static void hz11_thread_cache_flush_class(H11ThreadCache* tc, uint8_t class_id) {
#if HZ11_CACHE_SOA
#if HZ11_TRANSFER_CENTRAL_SPAN && HZ11_CLASSIFY_SPAN
  /* Transfer lane: batch-move all cached objects into the transfer cache,
   * spilling any excess to the central stack. Never sys_free arena pointers. */
  uint32_t n = tc->class_counts[class_id];
  HZ11_COUNT_ADD(tc->flush_items, n);
  if (n > 0u) {
    uint32_t inserted = hz11_transfer_insert_range(class_id,
        tc->class_items[class_id], n);
    if (inserted < n) {
      hz11_central_stack_insert_range(class_id,
          tc->class_items[class_id] + inserted, n - inserted);
    }
  }
  tc->class_counts[class_id] = 0u;
#if HZ11_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz11_class_slot_size(class_id) * n;
#endif
#else
  uint32_t n = tc->class_counts[class_id];
  HZ11_COUNT_ADD(tc->flush_items, n);
  for (uint32_t i = 0u; i < n; ++i) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(tc->class_items[class_id][i])) {
      hz11_returned_push(class_id, tc->class_items[class_id][i]);
    } else {
      hz11_sys_free(tc->class_items[class_id][i]);
    }
#else
    hz11_sys_free(tc->class_items[class_id][i]);
#endif
    tc->class_items[class_id][i] = NULL;
  }
  tc->class_counts[class_id] = 0u;
#if HZ11_CACHE_BYTE_ACCOUNTING
  tc->cached_bytes -= hz11_class_slot_size(class_id) * n;
#endif
#endif /* HZ11_TRANSFER_CENTRAL_SPAN */
#else
  H11ClassCache* cc = &tc->class_cache[class_id];
  size_t slot = hz11_class_slot_size(class_id);
#if HZ11_CACHE_TOPPTR
  void** p = cc->items;
  uint32_t n = (uint32_t)(cc->top - cc->items);
  HZ11_COUNT_ADD(tc->flush_items, n);
  while (p < cc->top) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(*p)) {
      hz11_returned_push(class_id, *p);
    } else {
      hz11_sys_free(*p);
    }
#else
    hz11_sys_free(*p);
#endif
    ++p;
  }
  cc->top = cc->items;
  tc->cached_bytes -= slot * n;
#else
  HZ11_COUNT_ADD(tc->flush_items, cc->count);
  for (uint32_t i = 0; i < cc->count; ++i) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(cc->items[i])) {
      hz11_returned_push(class_id, cc->items[i]);
    } else {
      hz11_sys_free(cc->items[i]);
    }
#else
    hz11_sys_free(cc->items[i]);
#endif
    cc->items[i] = NULL;
  }
  tc->cached_bytes -= slot * cc->count;
  cc->count = 0;
#endif
#endif /* HZ11_CACHE_SOA */
  HZ11_COUNT_INC(tc->flush_count);
}

void hz11_thread_cache_push_overflow_slow(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  HZ11_COUNT_INC(tc->overflow_count);
  HZ11_CLASS_DIAG_OVERFLOW(class_id);
  hz11_thread_cache_flush_class(tc, class_id);
  if (class_id < HZ11_CLASS_COUNT) {
#if HZ11_CACHE_SOA
    tc->class_items[class_id][0] = ptr;
    tc->class_counts[class_id] = 1u;
#if HZ11_CACHE_BYTE_ACCOUNTING
    tc->cached_bytes += hz11_class_slot_size(class_id);
#endif
#else
    H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
    *cc->top++ = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
#else
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
#endif
#endif /* HZ11_CACHE_SOA */
  } else {
    hz11_sys_free(ptr);
  }
}

void* hz11_thread_cache_refill(H11ThreadCache* tc, uint8_t class_id) {
#if !HZ11_CLASSIFY_SPAN && !HZ11_ENABLE_HOT_COUNTERS
  (void)tc; /* token lane: tc only used for the now-compiled-out refill_count */
#endif
  HZ11_COUNT_INC(tc->refill_count);
  HZ11_CLASS_DIAG_REFILL(class_id);
#if HZ11_TRANSFER_CENTRAL_SPAN && HZ11_CLASSIFY_SPAN
  /* Transfer lane: batch refill from transfer cache -> central stack -> span.
   * One mutex lock per batch (vs per-object returned_pop). */
  void* tmp[HZ11_TRANSFER_BATCH];
  uint32_t n = hz11_transfer_remove_range(class_id, tmp, HZ11_TRANSFER_BATCH);
  if (n > 0u) {
    hz11_span_source_diag_transfer_refill(class_id, 1u);
    HZ11_COUNT_INC(tc->refill_from_transfer);
  } else {
    hz11_span_source_diag_transfer_refill(class_id, 0u);
    n = hz11_central_stack_remove_range(class_id, tmp, HZ11_TRANSFER_BATCH);
    if (n > 0u) {
      hz11_span_source_diag_central_refill(class_id, 1u);
      HZ11_COUNT_INC(tc->refill_from_central);
    } else {
      hz11_span_source_diag_central_refill(class_id, 0u);
      size_t slot = hz11_class_slot_size(class_id);
      H11SpanCurrent* cs = &tc->current[class_id];
      while (n < HZ11_TRANSFER_BATCH) {
        if (!cs->base || cs->bump_index >= cs->slot_count) {
          if (cs->base && cs->bump_index >= cs->slot_count) {
            hz11_span_source_diag_current_exhaust(class_id);
          }
          char* base = NULL;
#if HZ11_CURRENT_SPAN_THREAD_EXIT
          if (hz11_current_span_pool_pop(class_id, cs)) {
            hz11_span_source_diag_span_reuse(class_id);
            hz11_span_source_diag_current_replace(class_id);
            continue;
          }
#endif
          base = (char*)hz11_span_return_pop_reusable_span(class_id);
          if (!base) {
            base = (char*)hz11_span_carve_for_class(class_id);
            if (base) {
              hz11_span_source_diag_arena_carve(class_id);
            }
          } else {
            hz11_span_source_diag_span_reuse(class_id);
          }
          if (!base) {
            break;
          }
          hz11_span_source_diag_current_replace(class_id);
          cs->base = base;
          cs->bump_index = 0u;
          cs->slot_count = (uint32_t)(HZ11_SPAN_BYTES / slot);
          hz11_span_return_register_active_span(class_id, base, cs->slot_count);
        }
        tmp[n++] = cs->base + (size_t)cs->bump_index++ * slot;
      }
      if (n > 0u) {
        HZ11_COUNT_INC(tc->refill_from_span);
      }
    }
  }
  if (n == 0u) {
    return hz11_sys_malloc(hz11_class_slot_size(class_id)); /* arena full fallback */
  }
  /* push tmp[1..n-1] into thread cache, return tmp[0] */
  for (uint32_t i = 1u; i < n; ++i) {
    hz11_thread_cache_push(tc, class_id, tmp[i]);
  }
  return tmp[0];
#elif HZ11_CLASSIFY_SPAN
  /* 1. per-class returned-object sink first (reuse before carving a fresh span) */
#if HZ11_RETURNED_REFILL_BATCH
  if (class_id >= HZ11_RETURNED_REFILL_BATCH_MIN_CLASS &&
      class_id <= HZ11_RETURNED_REFILL_BATCH_MAX_CLASS) {
#if HZ11_RETURNED_REFILL_BATCH_PRESSURE_GATE
    uint8_t pressure = tc->returned_refill_pressure[class_id];
    if (pressure >= HZ11_RETURNED_REFILL_BATCH_PRESSURE_THRESHOLD) {
#endif
    void* tmp[HZ11_RETURNED_REFILL_BATCH_COUNT];
    uint32_t n = hz11_returned_pop_range(class_id, tmp,
                                         HZ11_RETURNED_REFILL_BATCH_COUNT);
    if (n > 0u) {
      for (uint32_t i = 1u; i < n; ++i) {
        hz11_thread_cache_push(tc, class_id, tmp[i]);
      }
#if HZ11_RETURNED_REFILL_BATCH_PRESSURE_GATE
      tc->returned_refill_pressure[class_id] = 0u;
#endif
      return tmp[0];
    }
#if HZ11_RETURNED_REFILL_BATCH_PRESSURE_GATE
    if (pressure < 255u) {
      tc->returned_refill_pressure[class_id] = (uint8_t)(pressure + 1u);
    }
    } else {
      void* reused = hz11_returned_pop(class_id);
      if (reused) {
        if (pressure > 0u) {
          tc->returned_refill_pressure[class_id] = (uint8_t)(pressure - 1u);
        }
        return reused;
      }
      if (pressure < 255u) {
        tc->returned_refill_pressure[class_id] = (uint8_t)(pressure + 1u);
      }
    }
#endif
  } else
#endif
  {
    void* reused = hz11_returned_pop(class_id);
    if (reused) {
      return reused;
    }
  }
  /* 2. bump from the per-thread current span */
  size_t slot = hz11_class_slot_size(class_id);
  H11SpanCurrent* cs = &tc->current[class_id];
  if (cs->base && cs->bump_index < cs->slot_count) {
    return cs->base + (size_t)cs->bump_index++ * slot;
  }
  /* 3. current span exhausted (or none yet): carve a fresh 64 KiB span */
  char* base = (char*)hz11_span_carve_for_class(class_id);
  if (!base) {
    return hz11_sys_malloc(slot); /* arena full -- fall back (bench never hits) */
  }
  cs->base = base;
  cs->bump_index = 0;
  cs->slot_count = (uint32_t)(HZ11_SPAN_BYTES / slot);
  return cs->base + (size_t)cs->bump_index++ * slot;
#else
  return hz11_sys_malloc(hz11_class_slot_size(class_id));
#endif
}
