#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct WorkerArgs {
  uint64_t iters;
  size_t size;
  size_t align;
  int use_aligned_alloc;
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
    void* ptr = NULL;
    if (args->use_aligned_alloc) {
      ptr = aligned_alloc(args->align, args->size);
      if (!ptr) {
        fprintf(stderr, "aligned_alloc failed iter=%llu\n",
                (unsigned long long)i);
        abort();
      }
    } else {
      int rc = posix_memalign(&ptr, args->align, args->size);
      if (rc != 0 || !ptr) {
        fprintf(stderr, "posix_memalign failed rc=%d iter=%llu\n", rc,
                (unsigned long long)i);
        abort();
      }
    }
    if (((uintptr_t)ptr & (uintptr_t)(args->align - 1u)) != 0) {
      fprintf(stderr, "alignment failed ptr=%p align=%zu\n", ptr, args->align);
      abort();
    }
    ((volatile unsigned char*)ptr)[0] = (unsigned char)i;
    ((volatile unsigned char*)ptr)[args->size - 1u] = (unsigned char)(i >> 8);
    free(ptr);
    args->ops += 2u;
  }
  return NULL;
}

int main(int argc, char** argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 1;
  uint64_t iters = argc > 2 ? strtoull(argv[2], NULL, 10) : 100000;
  size_t size = argc > 3 ? strtoull(argv[3], NULL, 10) : 65536u;
  size_t align = argc > 4 ? strtoull(argv[4], NULL, 10) : 8192u;
  const char* mode = argc > 5 ? argv[5] : "posix";
  int use_aligned_alloc = 0;
  if (mode[0] == 'a') {
    use_aligned_alloc = 1;
  } else if (mode[0] != 'p') {
    fprintf(stderr,
            "usage: bench_aligned64k [threads] [iters] [size] [align] "
            "[posix|aligned]\n");
    return 2;
  }
  if (threads <= 0 || iters == 0 || size == 0 || align == 0 ||
      (align & (align - 1u)) != 0) {
    fprintf(stderr,
            "usage: bench_aligned64k [threads] [iters] [size] [align] "
            "[posix|aligned]\n");
    return 2;
  }
  if (use_aligned_alloc && (size % align) != 0) {
    fprintf(stderr, "aligned_alloc requires size to be a multiple of align\n");
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
    args[i].use_aligned_alloc = use_aligned_alloc;
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
  printf("threads=%d iters=%llu size=%zu align=%zu mode=%s time=%.6f "
         "ops/s=%.3f\n",
         threads, (unsigned long long)iters, size, align,
         use_aligned_alloc ? "aligned" : "posix", elapsed, (double)ops / elapsed);

  free(args);
  free(tids);
  return 0;
}
