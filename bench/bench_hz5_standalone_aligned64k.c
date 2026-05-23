#include "hz5.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef BENCHLAB_HZ5_EXACT_LOCAL2P_API
#define BENCHLAB_HZ5_EXACT_LOCAL2P_API 0
#endif

typedef struct WorkerArgs {
  uint64_t iters;
  size_t size;
  size_t align;
  uint64_t ops;
  int status;
} WorkerArgs;

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void* worker_main(void* arg) {
  WorkerArgs* args = (WorkerArgs*)arg;
  for (uint64_t i = 0; i < args->iters; ++i) {
#if BENCHLAB_HZ5_EXACT_LOCAL2P_API
    void* ptr = (args->size == 65536u && args->align == 8192u)
                    ? hz5_local2p_alloc_64k_a8192()
                    : hz5_aligned_alloc(args->size, args->align);
#else
    void* ptr = hz5_aligned_alloc(args->size, args->align);
#endif
    if (!ptr) {
      fprintf(stderr, "hz5_aligned_alloc failed iter=%llu size=%zu align=%zu\n",
              (unsigned long long)i, args->size, args->align);
      args->status = 5;
      return NULL;
    }
    if (((uintptr_t)ptr & (uintptr_t)(args->align - 1u)) != 0) {
      fprintf(stderr, "alignment failed ptr=%p align=%zu\n", ptr, args->align);
      hz5_free(ptr);
      args->status = 6;
      return NULL;
    }
    ((volatile unsigned char*)ptr)[0] = (unsigned char)i;
    ((volatile unsigned char*)ptr)[args->size - 1u] = (unsigned char)(i >> 8);
#if BENCHLAB_HZ5_EXACT_LOCAL2P_API
    Hz5FreeResult free_result =
        (args->size == 65536u && args->align == 8192u)
            ? hz5_local2p_free_64k_a8192(ptr)
            : hz5_free(ptr);
#else
    Hz5FreeResult free_result = hz5_free(ptr);
#endif
    if (free_result == HZ5_FREE_INVALID) {
      fprintf(stderr, "hz5_free rejected ptr=%p\n", ptr);
      args->status = 7;
      return NULL;
    }
    args->ops += 2u;
  }
  return NULL;
}

int main(int argc, char** argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 1;
  uint64_t iters = argc > 2 ? strtoull(argv[2], NULL, 10) : 100000;
  size_t size = argc > 3 ? strtoull(argv[3], NULL, 10) : 65536u;
  size_t align = argc > 4 ? strtoull(argv[4], NULL, 10) : 8192u;
  if (threads <= 0 || iters == 0 || size == 0 || align == 0 ||
      (align & (align - 1u)) != 0) {
    fprintf(stderr,
            "usage: bench_hz5_standalone_aligned64k [threads] [iters] "
            "[size] [align]\n");
    return 2;
  }

  pthread_t* tids = (pthread_t*)calloc((size_t)threads, sizeof(*tids));
  WorkerArgs* args = (WorkerArgs*)calloc((size_t)threads, sizeof(*args));
  if (!tids || !args) {
    return 3;
  }

  double start = now_sec();
  for (int i = 0; i < threads; ++i) {
    args[i].iters = iters;
    args[i].size = size;
    args[i].align = align;
    if (pthread_create(&tids[i], NULL, worker_main, &args[i]) != 0) {
      return 4;
    }
  }
  uint64_t ops = 0;
  int status = 0;
  for (int i = 0; i < threads; ++i) {
    pthread_join(tids[i], NULL);
    ops += args[i].ops;
    if (status == 0 && args[i].status != 0) {
      status = args[i].status;
    }
  }
  if (status != 0) {
    free(args);
    free(tids);
    return status;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz5-standalone threads=%d iters=%llu size=%zu align=%zu "
         "time=%.6f ops/s=%.3f\n",
         threads, (unsigned long long)iters, size, align, elapsed,
         (double)ops / elapsed);

  free(args);
  free(tids);
  return 0;
}
