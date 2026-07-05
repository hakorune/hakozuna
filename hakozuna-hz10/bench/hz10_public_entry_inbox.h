#ifndef HZ10_PUBLIC_ENTRY_INBOX_H
#define HZ10_PUBLIC_ENTRY_INBOX_H

#include "../src/hz10_platform.h"

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct Hz10BenchInbox {
  hz10_platform_mutex_t lock;
  void** items;
  _Atomic(uintptr_t)* atomic_items;
  _Atomic(size_t)* atomic_seq;
  size_t count;
  size_t capacity;
  _Atomic(size_t) head;
  _Atomic(size_t) tail;
} Hz10BenchInbox;

static int hz10_bench_use_spin_inbox;
static size_t hz10_bench_spin_inbox_capacity;

static size_t hz10_bench_next_power_of_two(size_t value) {
  size_t result = 1u;
  while (result < value) {
    result <<= 1u;
  }
  return result;
}

static void hz10_bench_inbox_init(Hz10BenchInbox* box, size_t capacity) {
  hz10_platform_mutex_init(&box->lock);
  if (hz10_bench_use_spin_inbox) {
    size_t requested = hz10_bench_spin_inbox_capacity != 0u
                           ? hz10_bench_spin_inbox_capacity
                           : capacity;
    capacity = hz10_bench_next_power_of_two(requested);
  }
  box->items = hz10_bench_use_spin_inbox ? NULL
                                         : calloc(capacity, sizeof(*box->items));
  box->atomic_items = hz10_bench_use_spin_inbox
                          ? calloc(capacity, sizeof(*box->atomic_items))
                          : NULL;
  box->atomic_seq = hz10_bench_use_spin_inbox
                        ? calloc(capacity, sizeof(*box->atomic_seq))
                        : NULL;
  box->count = 0;
  box->capacity = capacity;
  atomic_store_explicit(&box->head, 0u, memory_order_relaxed);
  atomic_store_explicit(&box->tail, 0u, memory_order_relaxed);
  if (hz10_bench_use_spin_inbox) {
    for (size_t i = 0; i < capacity; ++i) {
      atomic_store_explicit(&box->atomic_seq[i], i, memory_order_relaxed);
    }
  }
}

static void hz10_bench_inbox_destroy(Hz10BenchInbox* box) {
  hz10_platform_mutex_destroy(&box->lock);
  free(box->items);
  free(box->atomic_items);
  free(box->atomic_seq);
}

static void hz10_bench_inbox_push(Hz10BenchInbox* box, void* ptr) {
  if (hz10_bench_use_spin_inbox) {
    size_t ticket = atomic_fetch_add_explicit(&box->tail, 1u,
                                              memory_order_relaxed);
    size_t idx = ticket & (box->capacity - 1u);
    while (atomic_load_explicit(&box->atomic_seq[idx],
                                memory_order_acquire) != ticket) {
      sched_yield();
    }
    atomic_store_explicit(&box->atomic_items[idx], (uintptr_t)ptr,
                          memory_order_relaxed);
    atomic_store_explicit(&box->atomic_seq[idx], ticket + 1u,
                          memory_order_release);
    return;
  }
  hz10_platform_mutex_lock(&box->lock);
  if (box->count < box->capacity) {
    box->items[box->count++] = ptr;
  }
  hz10_platform_mutex_unlock(&box->lock);
}

static void hz10_bench_inbox_drain(Hz10BenchInbox* box,
                                  void (*free_fn)(void*)) {
  if (hz10_bench_use_spin_inbox) {
    size_t head = atomic_load_explicit(&box->head, memory_order_relaxed);
    for (;;) {
      size_t idx = head & (box->capacity - 1u);
      if (atomic_load_explicit(&box->atomic_seq[idx],
                               memory_order_acquire) != head + 1u) {
        break;
      }
      uintptr_t raw = atomic_load_explicit(&box->atomic_items[idx],
                                           memory_order_relaxed);
      atomic_store_explicit(&box->atomic_items[idx], 0u,
                            memory_order_relaxed);
      free_fn((void*)raw);
      atomic_store_explicit(&box->atomic_seq[idx], head + box->capacity,
                            memory_order_release);
      head += 1u;
    }
    atomic_store_explicit(&box->head, head, memory_order_release);
    return;
  }
  hz10_platform_mutex_lock(&box->lock);
  for (size_t i = 0; i < box->count; ++i) {
    free_fn(box->items[i]);
  }
  box->count = 0;
  hz10_platform_mutex_unlock(&box->lock);
}

#endif
