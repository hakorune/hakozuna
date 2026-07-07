#include "hz11_transfer_cache.h"

#if HZ11_TRANSFER_CENTRAL_SPAN

#include <stdatomic.h>
#include <pthread.h>
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
      pthread_mutex_unlock(&cs->lock);
      /* L1 has no span reclaim. A full central stack means the cap policy is wrong;
       * fail fast instead of silently dropping arena objects. */
      abort();
    }
    cs->slots[cs->count++] = items[i];
    atomic_fetch_add_explicit(&hz11_central_insert_count, 1u,
                              memory_order_relaxed);
  }
  pthread_mutex_unlock(&cs->lock);
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
