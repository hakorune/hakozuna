#include "hz5.h"

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct SpscQueue {
  void** slots;
  uint64_t mask;
  _Atomic uint64_t head;
  _Atomic uint64_t tail;
  _Atomic int status;
  uint64_t iters;
  size_t size;
  size_t align;
  uint64_t ops;
} SpscQueue;

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static int is_power_of_two(uint64_t value) {
  return value != 0 && (value & (value - 1u)) == 0;
}

static void* producer_main(void* arg) {
  SpscQueue* q = (SpscQueue*)arg;
  for (uint64_t i = 0; i < q->iters; ++i) {
    void* ptr = hz5_aligned_alloc(q->size, q->align);
    if (!ptr) {
      fprintf(stderr, "producer alloc failed iter=%llu size=%zu align=%zu\n",
              (unsigned long long)i, q->size, q->align);
      atomic_store_explicit(&q->status, 5, memory_order_release);
      return NULL;
    }
    if (((uintptr_t)ptr & (uintptr_t)(q->align - 1u)) != 0) {
      fprintf(stderr, "producer alignment failed ptr=%p align=%zu\n", ptr,
              q->align);
      hz5_free(ptr);
      atomic_store_explicit(&q->status, 6, memory_order_release);
      return NULL;
    }

    ((volatile unsigned char*)ptr)[0] = (unsigned char)i;
    ((volatile unsigned char*)ptr)[q->size - 1u] = (unsigned char)(i >> 8);

    for (;;) {
      uint64_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
      uint64_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
      if (head - tail <= q->mask) {
        q->slots[head & q->mask] = ptr;
        atomic_store_explicit(&q->head, head + 1u, memory_order_release);
        q->ops++;
        break;
      }
      sched_yield();
    }
  }
  return NULL;
}

static void* consumer_main(void* arg) {
  SpscQueue* q = (SpscQueue*)arg;
  for (uint64_t i = 0; i < q->iters; ++i) {
    void* ptr = NULL;
    for (;;) {
      uint64_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
      uint64_t head = atomic_load_explicit(&q->head, memory_order_acquire);
      if (tail != head) {
        ptr = q->slots[tail & q->mask];
        atomic_store_explicit(&q->tail, tail + 1u, memory_order_release);
        break;
      }
      if (atomic_load_explicit(&q->status, memory_order_acquire) != 0) {
        return NULL;
      }
      sched_yield();
    }
    if (hz5_free(ptr) == HZ5_FREE_INVALID) {
      fprintf(stderr, "consumer free rejected ptr=%p\n", ptr);
      atomic_store_explicit(&q->status, 7, memory_order_release);
      return NULL;
    }
    q->ops++;
  }
  return NULL;
}

int main(int argc, char** argv) {
  uint64_t iters = argc > 1 ? strtoull(argv[1], NULL, 10) : 100000u;
  size_t size = argc > 2 ? strtoull(argv[2], NULL, 10) : 65536u;
  size_t align = argc > 3 ? strtoull(argv[3], NULL, 10) : 8192u;
  uint64_t queue_cap = argc > 4 ? strtoull(argv[4], NULL, 10) : 1024u;
  if (iters == 0 || size == 0 || align == 0 ||
      (align & (align - 1u)) != 0 || !is_power_of_two(queue_cap)) {
    fprintf(stderr,
            "usage: bench_hz5_standalone_remote64k [iters] [size] [align] "
            "[queue_cap_power_of_two]\n");
    return 2;
  }

  SpscQueue q = {0};
  q.slots = (void**)calloc((size_t)queue_cap, sizeof(*q.slots));
  if (!q.slots) {
    return 3;
  }
  q.mask = queue_cap - 1u;
  q.iters = iters;
  q.size = size;
  q.align = align;

  pthread_t producer;
  pthread_t consumer;
  double start = now_sec();
  if (pthread_create(&consumer, NULL, consumer_main, &q) != 0 ||
      pthread_create(&producer, NULL, producer_main, &q) != 0) {
    free(q.slots);
    return 4;
  }
  pthread_join(producer, NULL);
  pthread_join(consumer, NULL);
  double elapsed = now_sec() - start;

  int status = atomic_load_explicit(&q.status, memory_order_acquire);
  if (status != 0) {
    free(q.slots);
    return status;
  }

  printf("allocator=hz5-standalone mode=producer-consumer iters=%llu "
         "size=%zu align=%zu queue=%llu time=%.6f ops/s=%.3f pairs/s=%.3f\n",
         (unsigned long long)iters, size, align,
         (unsigned long long)queue_cap, elapsed, (double)q.ops / elapsed,
         (double)iters / elapsed);
  free(q.slots);
  return 0;
}
