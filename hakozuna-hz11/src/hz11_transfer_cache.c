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
  }
}

static inline void hz11_transfer_init(void) {
  (void)pthread_once(&hz11_transfer_once, hz11_transfer_init_once);
}

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
    cs->slots[cs->count++] = items[i];
#if HZ11_CENTRAL_CLASS_DIAG
    cs->insert_count += 1u;
    if (cs->count > cs->high_water) {
      cs->high_water = cs->count;
    }
#endif
    atomic_fetch_add_explicit(&hz11_central_insert_count, 1u,
                              memory_order_relaxed);
  }
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

#endif /* HZ11_TRANSFER_CENTRAL_SPAN */
