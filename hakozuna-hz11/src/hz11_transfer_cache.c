#include "hz11_transfer_cache.h"

#if HZ11_TRANSFER_CENTRAL_SPAN

#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static _Atomic uint64_t hz11_transfer_remove_hit_count;
static _Atomic uint64_t hz11_transfer_remove_miss_count;
static _Atomic uint64_t hz11_transfer_insert_count;
static _Atomic uint64_t hz11_transfer_insert_spill_count;
static _Atomic uint64_t hz11_central_remove_hit_count;
static _Atomic uint64_t hz11_central_remove_miss_count;
static _Atomic uint64_t hz11_central_insert_count;
static _Atomic uint64_t hz11_span_return_count;
static _Atomic uint64_t hz11_span_reuse_count;
static _Atomic uint64_t hz11_central_full_span_count;
static _Atomic uint64_t hz11_central_partial_span_count;
static _Atomic uint64_t hz11_central_object_count;
static _Atomic uint64_t hz11_span_return_by_class[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_central_high_water_by_class[HZ11_CLASS_COUNT];

#if HZ11_SPAN_SOURCE_DIAG
static _Atomic uint64_t hz11_diag_span_create[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_span_reuse[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_current_exhaust[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_current_replace[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_transfer_hit[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_transfer_miss[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_central_hit[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_central_miss[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_arena_carve[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_sweep_count[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_sweep_scanned[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_meta_lock[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_created_high_water[HZ11_CLASS_COUNT];

static void hz11_diag_inc(_Atomic uint64_t* slot) {
  atomic_fetch_add_explicit(slot, 1u, memory_order_relaxed);
}

static void hz11_diag_add(_Atomic uint64_t* slot, uint64_t value) {
  atomic_fetch_add_explicit(slot, value, memory_order_relaxed);
}
#endif

#if HZ11_CENTRAL_SPAN_RETURN
enum {
  HZ11_SPAN_RETURN_ACTIVE = 0,
  HZ11_SPAN_RETURN_CENTRAL_PARTIAL = 1,
  HZ11_SPAN_RETURN_REUSABLE = 2
};

typedef struct H11SpanReturnMeta {
  uint8_t class_id;
  uint32_t slot_count;
  uint32_t central_free_count;
  uint32_t transfer_count;
  uint32_t state;
} H11SpanReturnMeta;

typedef struct H11ReusableSpanStack {
  pthread_mutex_t lock;
  uint32_t span_ids[HZ11_REUSABLE_SPAN_CAP];
  uint32_t count;
} H11ReusableSpanStack;

static H11SpanReturnMeta hz11_span_return_meta[HZ11_SPAN_COUNT];
static H11ReusableSpanStack hz11_reusable_spans[HZ11_CLASS_COUNT];
static pthread_mutex_t hz11_span_return_meta_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ---------- per-class transfer cache ---------- */

typedef struct H11TransferCache {
  pthread_mutex_t lock;
  void* slots[HZ11_TRANSFER_CAP];
  uint32_t count;
} H11TransferCache;

static H11TransferCache hz11_tc[HZ11_CLASS_COUNT];

typedef struct H11CentralStack {
  pthread_mutex_t lock;
  void* slots[HZ11_CENTRAL_CAP];
  uint32_t count;
#if HZ11_CENTRAL_CLASS_DIAG
  uint32_t high_water;
  uint32_t max_overflow_need;
  uint64_t insert_count;
  uint64_t remove_count;
  uint64_t overflow_count;
#endif
} H11CentralStack;

static H11CentralStack hz11_cs[HZ11_CLASS_COUNT];
static pthread_once_t hz11_transfer_once = PTHREAD_ONCE_INIT;

static void hz11_transfer_init_once(void) {
  for (uint32_t i = 0u; i < HZ11_CLASS_COUNT; ++i) {
    (void)pthread_mutex_init(&hz11_tc[i].lock, NULL);
    (void)pthread_mutex_init(&hz11_cs[i].lock, NULL);
#if HZ11_CENTRAL_SPAN_RETURN
    (void)pthread_mutex_init(&hz11_reusable_spans[i].lock, NULL);
#endif
  }
}

static inline void hz11_transfer_init(void) {
  (void)pthread_once(&hz11_transfer_once, hz11_transfer_init_once);
}

#if HZ11_CENTRAL_SPAN_RETURN || HZ11_SPAN_SOURCE_DIAG
static void hz11_atomic_max_u64(_Atomic uint64_t* dst, uint64_t value) {
  uint64_t cur = atomic_load_explicit(dst, memory_order_relaxed);
  while (cur < value &&
         !atomic_compare_exchange_weak_explicit(dst, &cur, value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}
#endif

void hz11_span_source_diag_transfer_refill(uint8_t class_id, uint32_t hit) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  hz11_diag_inc(hit ? &hz11_diag_transfer_hit[class_id]
                    : &hz11_diag_transfer_miss[class_id]);
#else
  (void)class_id; (void)hit;
#endif
}

void hz11_span_source_diag_central_refill(uint8_t class_id, uint32_t hit) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  hz11_diag_inc(hit ? &hz11_diag_central_hit[class_id]
                    : &hz11_diag_central_miss[class_id]);
#else
  (void)class_id; (void)hit;
#endif
}

void hz11_span_source_diag_current_exhaust(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_current_exhaust[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_current_replace(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_current_replace[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_arena_carve(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  uint64_t created =
      atomic_fetch_add_explicit(&hz11_diag_span_create[class_id], 1u,
                                memory_order_relaxed) + 1u;
  hz11_diag_inc(&hz11_diag_arena_carve[class_id]);
  hz11_atomic_max_u64(&hz11_diag_created_high_water[class_id], created);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_span_reuse(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_span_reuse[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_sweep(uint8_t class_id, uint32_t scanned) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  hz11_diag_inc(&hz11_diag_sweep_count[class_id]);
  hz11_diag_add(&hz11_diag_sweep_scanned[class_id], scanned);
#else
  (void)class_id; (void)scanned;
#endif
}

void hz11_span_source_diag_meta_lock(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_meta_lock[class_id]);
#else
  (void)class_id;
#endif
}

#if HZ11_CENTRAL_SPAN_RETURN
static int hz11_span_id_for_ptr(const void* ptr, uint32_t* span_id) {
  if (!hz11_arena_base || !ptr || !span_id) {
    return 0;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz11_arena_base;
  uintptr_t off = p - base;
  if (p < base || off >= (uintptr_t)HZ11_ARENA_BYTES) {
    return 0;
  }
  *span_id = (uint32_t)(off >> HZ11_SPAN_SHIFT);
  return 1;
}

static void hz11_span_return_ensure_meta_locked(uint32_t span_id) {
  H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
  if (meta->slot_count != 0u) {
    return;
  }
  uint8_t c1 = hz11_span_class[span_id];
  if (c1 == 0u) {
    abort();
  }
  uint8_t class_id = (uint8_t)(c1 - 1u);
  meta->class_id = class_id;
  meta->slot_count = (uint32_t)(HZ11_SPAN_BYTES / hz11_class_slot_size(class_id));
  meta->central_free_count = 0u;
  meta->transfer_count = 0u;
  meta->state = HZ11_SPAN_RETURN_ACTIVE;
}

static void hz11_span_return_account_transfer(void* ptr, int delta) {
  uint32_t span_id;
  if (!hz11_span_id_for_ptr(ptr, &span_id)) {
    abort();
  }
  uint8_t diag_class = 0u;
  if (hz11_span_class[span_id] != 0u) {
    diag_class = (uint8_t)(hz11_span_class[span_id] - 1u);
  }
  hz11_span_source_diag_meta_lock(diag_class);
  pthread_mutex_lock(&hz11_span_return_meta_lock);
  hz11_span_return_ensure_meta_locked(span_id);
  H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
  if (delta > 0) {
    meta->transfer_count += 1u;
  } else {
    if (meta->transfer_count == 0u) {
      pthread_mutex_unlock(&hz11_span_return_meta_lock);
      abort();
    }
    meta->transfer_count -= 1u;
  }
  pthread_mutex_unlock(&hz11_span_return_meta_lock);
}

static void hz11_reusable_span_push(uint8_t class_id, uint32_t span_id) {
  H11ReusableSpanStack* rs = &hz11_reusable_spans[class_id];
  pthread_mutex_lock(&rs->lock);
  if (rs->count >= HZ11_REUSABLE_SPAN_CAP) {
    pthread_mutex_unlock(&rs->lock);
    abort();
  }
  rs->span_ids[rs->count++] = span_id;
  pthread_mutex_unlock(&rs->lock);
}

static void hz11_central_try_return_full_span_locked(uint8_t class_id,
                                                     H11CentralStack* cs,
                                                     uint32_t span_id) {
  H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
  if (meta->central_free_count != meta->slot_count ||
      meta->transfer_count != 0u || meta->slot_count == 0u) {
    meta->state = HZ11_SPAN_RETURN_CENTRAL_PARTIAL;
    atomic_fetch_add_explicit(&hz11_central_partial_span_count, 1u,
                              memory_order_relaxed);
    return;
  }

  uint32_t write = 0u;
  uint32_t removed = 0u;
  for (uint32_t read = 0u; read < cs->count; ++read) {
    uint32_t item_span_id = UINT32_MAX;
    if (hz11_span_id_for_ptr(cs->slots[read], &item_span_id) &&
        item_span_id == span_id) {
      ++removed;
      continue;
    }
    cs->slots[write++] = cs->slots[read];
  }
  if (removed != meta->slot_count) {
    abort();
  }
  cs->count = write;
  meta->central_free_count = 0u;
  meta->state = HZ11_SPAN_RETURN_REUSABLE;
  atomic_fetch_sub_explicit(&hz11_central_object_count, removed,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&hz11_span_return_count, 1u, memory_order_relaxed);
  atomic_fetch_add_explicit(&hz11_central_full_span_count, 1u,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&hz11_span_return_by_class[class_id], 1u,
                            memory_order_relaxed);
  hz11_reusable_span_push(class_id, span_id);
}

static void hz11_central_sweep_full_spans_locked(uint8_t class_id,
                                                 H11CentralStack* cs) {
  uint32_t i = 0u;
  while (i < cs->count) {
    uint32_t before_scan_count = cs->count;
    uint32_t span_id;
    if (!hz11_span_id_for_ptr(cs->slots[i], &span_id)) {
      ++i;
      continue;
    }
    hz11_span_source_diag_meta_lock(class_id);
    pthread_mutex_lock(&hz11_span_return_meta_lock);
    H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
    uint32_t before = cs->count;
    if (meta->state != HZ11_SPAN_RETURN_REUSABLE) {
      hz11_central_try_return_full_span_locked(class_id, cs, span_id);
    }
    pthread_mutex_unlock(&hz11_span_return_meta_lock);
    if (cs->count < before) {
      i = 0u;
    } else {
      ++i;
    }
    hz11_span_source_diag_sweep(class_id, before_scan_count);
  }
}
#endif

uint32_t hz11_transfer_remove_range(uint8_t class_id, void** out, uint32_t max_n) {
  if (class_id >= HZ11_CLASS_COUNT || !out || !max_n) {
    return 0u;
  }
  hz11_transfer_init();
  H11TransferCache* tc = &hz11_tc[class_id];
  uint32_t n = 0u;
  pthread_mutex_lock(&tc->lock);
  uint32_t avail = tc->count;
  if (avail < max_n) {
    max_n = avail;
  }
  /* pop from the end (LIFO within the transfer cache) */
  for (n = 0u; n < max_n; ++n) {
    out[n] = tc->slots[--tc->count];
#if HZ11_CENTRAL_SPAN_RETURN
    hz11_span_return_account_transfer(out[n], -1);
#endif
  }
  pthread_mutex_unlock(&tc->lock);
  atomic_fetch_add_explicit(n > 0u ? &hz11_transfer_remove_hit_count
                                   : &hz11_transfer_remove_miss_count,
                            1u, memory_order_relaxed);
  return n;
}

uint32_t hz11_transfer_insert_range(uint8_t class_id, void** items, uint32_t n) {
  if (class_id >= HZ11_CLASS_COUNT || !items || !n) {
    return 0u;
  }
  hz11_transfer_init();
  H11TransferCache* tc = &hz11_tc[class_id];
  uint32_t inserted = 0u;
  pthread_mutex_lock(&tc->lock);
  uint32_t space = HZ11_TRANSFER_CAP - tc->count;
  if (space > n) {
    space = n;
  }
  for (inserted = 0u; inserted < space; ++inserted) {
#if HZ11_CENTRAL_SPAN_RETURN
    hz11_span_return_account_transfer(items[inserted], 1);
#endif
    tc->slots[tc->count++] = items[inserted];
  }
  pthread_mutex_unlock(&tc->lock);
  atomic_fetch_add_explicit(&hz11_transfer_insert_count, inserted,
                            memory_order_relaxed);
  if (inserted < n) {
    atomic_fetch_add_explicit(&hz11_transfer_insert_spill_count, n - inserted,
                              memory_order_relaxed);
  }
  return inserted;
}

/* ---------- per-class central object stack ---------- */

uint32_t hz11_central_stack_remove_range(uint8_t class_id, void** out, uint32_t max_n) {
  if (class_id >= HZ11_CLASS_COUNT || !out || !max_n) {
    return 0u;
  }
  hz11_transfer_init();
  H11CentralStack* cs = &hz11_cs[class_id];
  uint32_t n = 0u;
  pthread_mutex_lock(&cs->lock);
  uint32_t avail = cs->count;
  if (avail < max_n) {
    max_n = avail;
  }
  for (n = 0u; n < max_n; ++n) {
    out[n] = cs->slots[--cs->count];
#if HZ11_CENTRAL_SPAN_RETURN
    uint32_t span_id;
    if (!hz11_span_id_for_ptr(out[n], &span_id)) {
      pthread_mutex_unlock(&cs->lock);
      abort();
    }
    hz11_span_source_diag_meta_lock(class_id);
    pthread_mutex_lock(&hz11_span_return_meta_lock);
    hz11_span_return_ensure_meta_locked(span_id);
    H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
    if (meta->central_free_count == 0u ||
        meta->state == HZ11_SPAN_RETURN_REUSABLE ||
        meta->class_id != class_id) {
      pthread_mutex_unlock(&hz11_span_return_meta_lock);
      pthread_mutex_unlock(&cs->lock);
      abort();
    }
    meta->central_free_count -= 1u;
    meta->state = meta->central_free_count > 0u
        ? HZ11_SPAN_RETURN_CENTRAL_PARTIAL
        : HZ11_SPAN_RETURN_ACTIVE;
    pthread_mutex_unlock(&hz11_span_return_meta_lock);
    atomic_fetch_sub_explicit(&hz11_central_object_count, 1u,
                              memory_order_relaxed);
#endif
  }
#if HZ11_CENTRAL_CLASS_DIAG
  cs->remove_count += n;
#endif
  pthread_mutex_unlock(&cs->lock);
  atomic_fetch_add_explicit(n > 0u ? &hz11_central_remove_hit_count
                                   : &hz11_central_remove_miss_count,
                            1u, memory_order_relaxed);
  return n;
}

void hz11_central_stack_insert_range(uint8_t class_id, void** items, uint32_t n) {
  if (class_id >= HZ11_CLASS_COUNT || !items || !n) {
    return;
  }
  hz11_transfer_init();
  H11CentralStack* cs = &hz11_cs[class_id];
  pthread_mutex_lock(&cs->lock);
  for (uint32_t i = 0u; i < n; ++i) {
    if (cs->count >= HZ11_CENTRAL_CAP) {
#if HZ11_CENTRAL_SPAN_RETURN
      hz11_central_sweep_full_spans_locked(class_id, cs);
      if (cs->count < HZ11_CENTRAL_CAP) {
        /* The sweep returned at least one full span; retry this item. */
      } else
#endif
      {
#if HZ11_CENTRAL_CLASS_DIAG
        uint32_t needed = cs->count + (n - i);
        if (needed > cs->max_overflow_need) {
          cs->max_overflow_need = needed;
        }
        cs->overflow_count += 1u;
        fprintf(stderr,
                "hz11_central_overflow class=%u count=%u cap=%u incoming_left=%u "
                "needed=%u high_water=%u insert=%llu remove=%llu overflow=%llu\n",
                (unsigned)class_id, (unsigned)cs->count,
                (unsigned)HZ11_CENTRAL_CAP, (unsigned)(n - i),
                (unsigned)needed, (unsigned)cs->high_water,
                (unsigned long long)cs->insert_count,
                (unsigned long long)cs->remove_count,
                (unsigned long long)cs->overflow_count);
#endif
        pthread_mutex_unlock(&cs->lock);
#if HZ11_CENTRAL_CLASS_DIAG
        hz11_central_stack_dump_class_stats();
#endif
        /* L1 has no span reclaim. A full central stack means the cap policy is wrong;
         * fail fast instead of silently dropping arena objects. */
        abort();
      }
    }
#if HZ11_CENTRAL_SPAN_RETURN
    uint32_t span_id;
    if (!hz11_span_id_for_ptr(items[i], &span_id)) {
      pthread_mutex_unlock(&cs->lock);
      abort();
    }
    hz11_span_source_diag_meta_lock(class_id);
    pthread_mutex_lock(&hz11_span_return_meta_lock);
    hz11_span_return_ensure_meta_locked(span_id);
    H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
    if (meta->state == HZ11_SPAN_RETURN_REUSABLE) {
      meta->state = HZ11_SPAN_RETURN_ACTIVE;
    }
    if (meta->class_id != class_id || meta->slot_count == 0u) {
      pthread_mutex_unlock(&hz11_span_return_meta_lock);
      pthread_mutex_unlock(&cs->lock);
      abort();
    }
    meta->central_free_count += 1u;
    pthread_mutex_unlock(&hz11_span_return_meta_lock);
#endif
    cs->slots[cs->count++] = items[i];
#if HZ11_CENTRAL_SPAN_RETURN
    atomic_fetch_add_explicit(&hz11_central_object_count, 1u,
                              memory_order_relaxed);
    hz11_atomic_max_u64(&hz11_central_high_water_by_class[class_id], cs->count);
#endif
#if HZ11_CENTRAL_CLASS_DIAG
    cs->insert_count += 1u;
    if (cs->count > cs->high_water) {
      cs->high_water = cs->count;
    }
#endif
    atomic_fetch_add_explicit(&hz11_central_insert_count, 1u,
                              memory_order_relaxed);
  }
#if HZ11_CENTRAL_SPAN_RETURN
  for (uint32_t i = 0u; i < n; ++i) {
    uint32_t span_id;
    if (!hz11_span_id_for_ptr(items[i], &span_id)) {
      continue;
    }
    hz11_span_source_diag_meta_lock(class_id);
    pthread_mutex_lock(&hz11_span_return_meta_lock);
    H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
    if (meta->state != HZ11_SPAN_RETURN_REUSABLE) {
      hz11_central_try_return_full_span_locked(class_id, cs, span_id);
    }
    pthread_mutex_unlock(&hz11_span_return_meta_lock);
  }
#endif
  pthread_mutex_unlock(&cs->lock);
}

void hz11_central_stack_dump_class_stats(void) {
#if HZ11_CENTRAL_CLASS_DIAG
  hz11_transfer_init();
  fprintf(stderr, "hz11_central_class_stats_begin cap=%u\n",
          (unsigned)HZ11_CENTRAL_CAP);
  for (uint32_t class_id = 0u; class_id < HZ11_CLASS_COUNT; ++class_id) {
    H11CentralStack* cs = &hz11_cs[class_id];
    pthread_mutex_lock(&cs->lock);
    uint32_t count = cs->count;
    uint32_t high_water = cs->high_water;
    uint32_t max_overflow_need = cs->max_overflow_need;
    uint64_t insert_count = cs->insert_count;
    uint64_t remove_count = cs->remove_count;
    uint64_t overflow_count = cs->overflow_count;
    pthread_mutex_unlock(&cs->lock);
    if (count == 0u && high_water == 0u && max_overflow_need == 0u &&
        insert_count == 0u && remove_count == 0u && overflow_count == 0u) {
      continue;
    }
    fprintf(stderr,
            "hz11_central_class class=%u count=%u high_water=%u "
            "max_overflow_need=%u insert=%llu remove=%llu overflow=%llu\n",
            (unsigned)class_id, (unsigned)count, (unsigned)high_water,
            (unsigned)max_overflow_need, (unsigned long long)insert_count,
            (unsigned long long)remove_count, (unsigned long long)overflow_count);
  }
  fprintf(stderr, "hz11_central_class_stats_end\n");
#endif
}

void hz11_span_source_diag_dump(void) {
#if HZ11_SPAN_SOURCE_DIAG
  fprintf(stderr, "hz11_span_source_begin\n");
  for (uint32_t class_id = 0u; class_id < HZ11_CLASS_COUNT; ++class_id) {
    uint64_t span_create = atomic_load_explicit(&hz11_diag_span_create[class_id],
                                                memory_order_relaxed);
    uint64_t span_reuse = atomic_load_explicit(&hz11_diag_span_reuse[class_id],
                                               memory_order_relaxed);
    uint64_t current_exhaust =
        atomic_load_explicit(&hz11_diag_current_exhaust[class_id],
                             memory_order_relaxed);
    uint64_t current_replace =
        atomic_load_explicit(&hz11_diag_current_replace[class_id],
                             memory_order_relaxed);
    uint64_t transfer_hit =
        atomic_load_explicit(&hz11_diag_transfer_hit[class_id],
                             memory_order_relaxed);
    uint64_t transfer_miss =
        atomic_load_explicit(&hz11_diag_transfer_miss[class_id],
                             memory_order_relaxed);
    uint64_t central_hit = atomic_load_explicit(&hz11_diag_central_hit[class_id],
                                                memory_order_relaxed);
    uint64_t central_miss =
        atomic_load_explicit(&hz11_diag_central_miss[class_id],
                             memory_order_relaxed);
    uint64_t arena_carve =
        atomic_load_explicit(&hz11_diag_arena_carve[class_id],
                             memory_order_relaxed);
    uint64_t sweep_count =
        atomic_load_explicit(&hz11_diag_sweep_count[class_id],
                             memory_order_relaxed);
    uint64_t sweep_scanned =
        atomic_load_explicit(&hz11_diag_sweep_scanned[class_id],
                             memory_order_relaxed);
    uint64_t meta_lock = atomic_load_explicit(&hz11_diag_meta_lock[class_id],
                                              memory_order_relaxed);
    uint64_t created_high =
        atomic_load_explicit(&hz11_diag_created_high_water[class_id],
                             memory_order_relaxed);
    uint64_t live_current =
        current_replace > current_exhaust ? current_replace - current_exhaust : 0u;
    if (span_create == 0u && span_reuse == 0u && current_exhaust == 0u &&
        current_replace == 0u && transfer_hit == 0u && transfer_miss == 0u &&
        central_hit == 0u && central_miss == 0u && arena_carve == 0u &&
        sweep_count == 0u && sweep_scanned == 0u && meta_lock == 0u) {
      continue;
    }
    fprintf(stderr,
            "hz11_span_source_class class=%u span_create=%llu span_reuse=%llu "
            "current_span_exhaust=%llu current_span_replace=%llu "
            "transfer_refill_hit=%llu transfer_refill_miss=%llu "
            "central_refill_hit=%llu central_refill_miss=%llu "
            "arena_carve=%llu live_current_span=%llu created_high_water=%llu "
            "sweep_count=%llu sweep_scanned=%llu meta_lock=%llu\n",
            (unsigned)class_id, (unsigned long long)span_create,
            (unsigned long long)span_reuse,
            (unsigned long long)current_exhaust,
            (unsigned long long)current_replace,
            (unsigned long long)transfer_hit,
            (unsigned long long)transfer_miss,
            (unsigned long long)central_hit,
            (unsigned long long)central_miss,
            (unsigned long long)arena_carve,
            (unsigned long long)live_current,
            (unsigned long long)created_high,
            (unsigned long long)sweep_count,
            (unsigned long long)sweep_scanned,
            (unsigned long long)meta_lock);
  }
  fprintf(stderr, "hz11_span_source_end\n");
#endif
}

void* hz11_span_return_pop_reusable_span(uint8_t class_id) {
#if HZ11_CENTRAL_SPAN_RETURN
  if (class_id >= HZ11_CLASS_COUNT) {
    return NULL;
  }
  hz11_transfer_init();
  H11ReusableSpanStack* rs = &hz11_reusable_spans[class_id];
  for (;;) {
    pthread_mutex_lock(&rs->lock);
    if (rs->count == 0u) {
      pthread_mutex_unlock(&rs->lock);
      return NULL;
    }
    uint32_t span_id = rs->span_ids[--rs->count];
    pthread_mutex_unlock(&rs->lock);

    pthread_mutex_lock(&hz11_span_return_meta_lock);
    H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
    if (meta->state != HZ11_SPAN_RETURN_REUSABLE ||
        meta->central_free_count != 0u || meta->transfer_count != 0u ||
        meta->class_id != class_id) {
      pthread_mutex_unlock(&hz11_span_return_meta_lock);
      continue;
    }
    meta->state = HZ11_SPAN_RETURN_ACTIVE;
    pthread_mutex_unlock(&hz11_span_return_meta_lock);

    atomic_fetch_add_explicit(&hz11_span_reuse_count, 1u, memory_order_relaxed);
    return hz11_arena_base + ((size_t)span_id << HZ11_SPAN_SHIFT);
  }
#else
  (void)class_id;
  return NULL;
#endif
}

void hz11_span_return_register_active_span(uint8_t class_id, void* base,
                                           uint32_t slot_count) {
#if HZ11_CENTRAL_SPAN_RETURN
  uint32_t span_id;
  if (class_id >= HZ11_CLASS_COUNT || !base || slot_count == 0u ||
      !hz11_span_id_for_ptr(base, &span_id)) {
    abort();
  }
  hz11_transfer_init();
  pthread_mutex_lock(&hz11_span_return_meta_lock);
  H11SpanReturnMeta* meta = &hz11_span_return_meta[span_id];
  if (meta->state != HZ11_SPAN_RETURN_ACTIVE &&
      meta->state != HZ11_SPAN_RETURN_REUSABLE) {
    pthread_mutex_unlock(&hz11_span_return_meta_lock);
    abort();
  }
  meta->class_id = class_id;
  meta->slot_count = slot_count;
  meta->central_free_count = 0u;
  meta->transfer_count = 0u;
  meta->state = HZ11_SPAN_RETURN_ACTIVE;
  pthread_mutex_unlock(&hz11_span_return_meta_lock);
#else
  (void)class_id; (void)base; (void)slot_count;
#endif
}

uint64_t hz11_transfer_remove_hit_count_load(void) {
  return atomic_load_explicit(&hz11_transfer_remove_hit_count, memory_order_relaxed);
}

uint64_t hz11_transfer_remove_miss_count_load(void) {
  return atomic_load_explicit(&hz11_transfer_remove_miss_count, memory_order_relaxed);
}

uint64_t hz11_transfer_insert_count_load(void) {
  return atomic_load_explicit(&hz11_transfer_insert_count, memory_order_relaxed);
}

uint64_t hz11_transfer_insert_spill_count_load(void) {
  return atomic_load_explicit(&hz11_transfer_insert_spill_count, memory_order_relaxed);
}

uint64_t hz11_central_remove_hit_count_load(void) {
  return atomic_load_explicit(&hz11_central_remove_hit_count, memory_order_relaxed);
}

uint64_t hz11_central_remove_miss_count_load(void) {
  return atomic_load_explicit(&hz11_central_remove_miss_count, memory_order_relaxed);
}

uint64_t hz11_central_insert_count_load(void) {
  return atomic_load_explicit(&hz11_central_insert_count, memory_order_relaxed);
}

uint64_t hz11_span_return_count_load(void) {
  return atomic_load_explicit(&hz11_span_return_count, memory_order_relaxed);
}

uint64_t hz11_span_reuse_count_load(void) {
  return atomic_load_explicit(&hz11_span_reuse_count, memory_order_relaxed);
}

uint64_t hz11_central_full_span_count_load(void) {
  return atomic_load_explicit(&hz11_central_full_span_count,
                              memory_order_relaxed);
}

uint64_t hz11_central_partial_span_count_load(void) {
  return atomic_load_explicit(&hz11_central_partial_span_count,
                              memory_order_relaxed);
}

uint64_t hz11_central_object_count_load(void) {
  return atomic_load_explicit(&hz11_central_object_count, memory_order_relaxed);
}

uint64_t hz11_span_return_by_class_load(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return 0u;
  }
  return atomic_load_explicit(&hz11_span_return_by_class[class_id],
                              memory_order_relaxed);
}

uint64_t hz11_central_high_water_by_class_load(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return 0u;
  }
  return atomic_load_explicit(&hz11_central_high_water_by_class[class_id],
                              memory_order_relaxed);
}

#endif /* HZ11_TRANSFER_CENTRAL_SPAN */
