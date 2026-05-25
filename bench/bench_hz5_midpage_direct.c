// Usage:
//   bench_hz5_midpage_direct [threads] [iters] [working_set] [min_size] [max_size]

#include "hz5_midpagefront.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct Hz5MidPageDirectArg {
  uint32_t seed;
  size_t iters;
  size_t working_set;
  size_t min_size;
  size_t max_size;
  uint64_t alloc_failures;
  uint64_t free_failures;
} Hz5MidPageDirectArg;

static inline uint32_t hz5_midpage_direct_lcg(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static uint64_t hz5_midpage_direct_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void* hz5_midpage_direct_thread(void* arg) {
  Hz5MidPageDirectArg* ta = (Hz5MidPageDirectArg*)arg;
  void** slots = (void**)calloc(ta->working_set, sizeof(void*));
  if (!slots) {
    ta->alloc_failures += ta->iters;
    return NULL;
  }

  uint32_t seed = ta->seed;
  size_t span = (ta->max_size > ta->min_size)
                    ? (ta->max_size - ta->min_size + 1u)
                    : 1u;

  for (size_t i = 0; i < ta->iters; ++i) {
    size_t idx = (size_t)(hz5_midpage_direct_lcg(&seed) % ta->working_set);
    if (slots[idx]) {
      Hz5MidPageFrontFreeResult free_result =
          hz5_midpagefront_free(slots[idx]);
      if (free_result != HZ5_MIDPAGEFRONT_FREE_OK) {
        ++ta->free_failures;
      }
      slots[idx] = NULL;
    }

    size_t size = ta->min_size + (hz5_midpage_direct_lcg(&seed) % span);
    void* ptr = NULL;
    Hz5MidPageFrontAllocResult alloc_result =
        hz5_midpagefront_try_alloc(size, 16u, &ptr);
    if (alloc_result != HZ5_MIDPAGEFRONT_ALLOC_OK || !ptr) {
      ++ta->alloc_failures;
      continue;
    }
    slots[idx] = ptr;
  }

  for (size_t i = 0; i < ta->working_set; ++i) {
    if (!slots[i]) {
      continue;
    }
    Hz5MidPageFrontFreeResult free_result = hz5_midpagefront_free(slots[i]);
    if (free_result != HZ5_MIDPAGEFRONT_FREE_OK) {
      ++ta->free_failures;
    }
  }
  free(slots);
  return NULL;
}

int main(int argc, char** argv) {
  size_t threads = 8;
  size_t iters = 300000;
  size_t working_set = 100;
  size_t min_size = 2049;
  size_t max_size = 32768;

  if (argc > 1) {
    threads = (size_t)strtoull(argv[1], NULL, 10);
  }
  if (argc > 2) {
    iters = (size_t)strtoull(argv[2], NULL, 10);
  }
  if (argc > 3) {
    working_set = (size_t)strtoull(argv[3], NULL, 10);
  }
  if (argc > 4) {
    min_size = (size_t)strtoull(argv[4], NULL, 10);
  }
  if (argc > 5) {
    max_size = (size_t)strtoull(argv[5], NULL, 10);
  }

  if (threads == 0) {
    threads = 1;
  }
  if (working_set == 0) {
    working_set = 1;
  }
  if (min_size == 0) {
    min_size = 1;
  }
  if (max_size < min_size) {
    max_size = min_size;
  }

  pthread_t* tids = (pthread_t*)calloc(threads, sizeof(pthread_t));
  Hz5MidPageDirectArg* args =
      (Hz5MidPageDirectArg*)calloc(threads, sizeof(Hz5MidPageDirectArg));
  if (!tids || !args) {
    fprintf(stderr, "bench_hz5_midpage_direct: allocation failure\n");
    free(tids);
    free(args);
    return 1;
  }

  uint64_t start = hz5_midpage_direct_now_ns();
  for (size_t i = 0; i < threads; ++i) {
    args[i].seed = (uint32_t)(0xC0FFEEu + (uint32_t)i * 17u);
    args[i].iters = iters;
    args[i].working_set = working_set;
    args[i].min_size = min_size;
    args[i].max_size = max_size;
    if (pthread_create(&tids[i], NULL, hz5_midpage_direct_thread, &args[i]) !=
        0) {
      fprintf(stderr, "bench_hz5_midpage_direct: pthread_create failed\n");
      free(tids);
      free(args);
      return 1;
    }
  }

  uint64_t alloc_failures = 0;
  uint64_t free_failures = 0;
  for (size_t i = 0; i < threads; ++i) {
    pthread_join(tids[i], NULL);
    alloc_failures += args[i].alloc_failures;
    free_failures += args[i].free_failures;
  }
  uint64_t end = hz5_midpage_direct_now_ns();

  double sec = (double)(end - start) / 1000000000.0;
  double total_ops = (double)threads * (double)iters;
  double ops_per_s = sec > 0.0 ? total_ops / sec : 0.0;

  printf("[BENCH_HZ5_MIDPAGE_DIRECT] threads=%zu iters=%zu ws=%zu "
         "min_size=%zu max_size=%zu\n",
         threads,
         iters,
         working_set,
         min_size,
         max_size);
  printf("bench_hz5_midpage_direct: threads=%zu ops=%.0f time=%.6f "
         "ops/s=%.2f\n",
         threads,
         total_ops,
         sec,
         ops_per_s);
  printf("[STATS] alloc_failures=%llu free_failures=%llu\n",
         (unsigned long long)alloc_failures,
         (unsigned long long)free_failures);

  free(tids);
  free(args);
  return (alloc_failures == 0 && free_failures == 0) ? 0 : 2;
}
