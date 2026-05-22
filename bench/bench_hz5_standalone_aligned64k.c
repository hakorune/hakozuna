#include "hz5.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct WorkerArgs {
  uint64_t iters;
  size_t size;
  size_t align;
  uint64_t ops;
} WorkerArgs;

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void* worker_main(void* arg) {
  WorkerArgs* args = (WorkerArgs*)arg;
  for (uint64_t i = 0; i < args->iters; ++i) {
    void* ptr = hz5_aligned_alloc(args->size, args->align);
    if (!ptr) {
      fprintf(stderr, "hz5_aligned_alloc failed iter=%llu size=%zu align=%zu\n",
              (unsigned long long)i, args->size, args->align);
      abort();
    }
    if (((uintptr_t)ptr & (uintptr_t)(args->align - 1u)) != 0) {
      fprintf(stderr, "alignment failed ptr=%p align=%zu\n", ptr, args->align);
      abort();
    }
    ((volatile unsigned char*)ptr)[0] = (unsigned char)i;
    ((volatile unsigned char*)ptr)[args->size - 1u] = (unsigned char)(i >> 8);
    if (hz5_free(ptr) == HZ5_FREE_INVALID) {
      fprintf(stderr, "hz5_free rejected ptr=%p\n", ptr);
      abort();
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
  for (int i = 0; i < threads; ++i) {
    pthread_join(tids[i], NULL);
    ops += args[i].ops;
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
