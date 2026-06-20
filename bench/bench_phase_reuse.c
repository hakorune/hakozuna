#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct FreeArgs {
  void** ptrs;
  uint64_t begin;
  uint64_t end;
} FreeArgs;

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static int compare_ptrs(const void* lhs, const void* rhs) {
  const uintptr_t a = (uintptr_t)*(void* const*)lhs;
  const uintptr_t b = (uintptr_t)*(void* const*)rhs;
  return (a > b) - (a < b);
}

static uint64_t count_reused_ptrs(void** first, void** second, uint64_t n) {
  uint64_t i = 0;
  uint64_t j = 0;
  uint64_t reused = 0;
  qsort(first, (size_t)n, sizeof(first[0]), compare_ptrs);
  qsort(second, (size_t)n, sizeof(second[0]), compare_ptrs);
  while (i < n && j < n) {
    uintptr_t a = (uintptr_t)first[i];
    uintptr_t b = (uintptr_t)second[j];
    if (a == b) {
      ++reused;
      ++i;
      ++j;
    } else if (a < b) {
      ++i;
    } else {
      ++j;
    }
  }
  return reused;
}

static void touch_payload(void* ptr, size_t size, uint64_t i) {
  if (!ptr || size == 0) {
    return;
  }
  unsigned char* bytes = (unsigned char*)ptr;
  bytes[0] = (unsigned char)i;
  bytes[size / 2u] ^= (unsigned char)(i >> 8);
  bytes[size - 1u] = (unsigned char)(i >> 16);
}

static void* free_worker(void* arg) {
  FreeArgs* args = (FreeArgs*)arg;
  for (uint64_t i = args->begin; i < args->end; ++i) {
    free(args->ptrs[i]);
  }
  return NULL;
}

int main(int argc, char** argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 4;
  uint64_t count = argc > 2 ? strtoull(argv[2], NULL, 10) : 4096u;
  size_t size = argc > 3 ? strtoull(argv[3], NULL, 10) : 128u;
  if (threads <= 0 || count == 0 || size == 0) {
    fprintf(stderr, "usage: bench_phase_reuse [threads] [count] [size]\n");
    return 2;
  }

  void** first = (void**)calloc((size_t)count, sizeof(first[0]));
  void** second = (void**)calloc((size_t)count, sizeof(second[0]));
  void** first_for_count = (void**)calloc((size_t)count, sizeof(first[0]));
  pthread_t* tids = (pthread_t*)calloc((size_t)threads, sizeof(tids[0]));
  FreeArgs* args = (FreeArgs*)calloc((size_t)threads, sizeof(args[0]));
  if (!first || !second || !first_for_count || !tids || !args) {
    free(args);
    free(tids);
    free(first_for_count);
    free(second);
    free(first);
    return 3;
  }

  double start = now_sec();
  uint64_t phase_a = 0;
  uint64_t phase_c = 0;
  int created_threads = 0;
  int phase_b_done = 0;
  int rc = 0;

  for (uint64_t i = 0; i < count; ++i) {
    first[i] = malloc(size);
    if (!first[i]) {
      fprintf(stderr, "phase-a malloc failed i=%llu size=%zu\n",
              (unsigned long long)i, size);
      rc = 4;
      goto done;
    }
    first_for_count[i] = first[i];
    touch_payload(first[i], size, i);
    ++phase_a;
  }

  for (int t = 0; t < threads; ++t) {
    uint64_t begin = (count * (uint64_t)t) / (uint64_t)threads;
    uint64_t end = (count * (uint64_t)(t + 1)) / (uint64_t)threads;
    args[t].ptrs = first;
    args[t].begin = begin;
    args[t].end = end;
    if (pthread_create(&tids[t], NULL, free_worker, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed t=%d\n", t);
      rc = 5;
      goto done;
    }
    ++created_threads;
  }
  for (int t = 0; t < created_threads; ++t) {
    pthread_join(tids[t], NULL);
  }
  phase_b_done = 1;

  for (uint64_t i = 0; i < count; ++i) {
    second[i] = malloc(size);
    if (!second[i]) {
      fprintf(stderr, "phase-c malloc failed i=%llu size=%zu\n",
              (unsigned long long)i, size);
      rc = 6;
      goto done;
    }
    touch_payload(second[i], size, i + count);
    ++phase_c;
  }

  double elapsed = now_sec() - start;
  uint64_t reuse_hits = count_reused_ptrs(first_for_count, second, count);
  printf("phase_reuse threads=%d count=%llu size=%zu time=%.6f "
         "ops=%llu ops/s=%.3f reuse_hits=%llu\n",
         threads, (unsigned long long)count, size, elapsed,
         (unsigned long long)(count * 3u), (double)(count * 3u) / elapsed,
         (unsigned long long)reuse_hits);

done:
  if (!phase_b_done) {
    for (int t = 0; t < created_threads; ++t) {
      pthread_join(tids[t], NULL);
    }
    phase_b_done = created_threads == threads;
  }
  for (uint64_t i = 0; i < phase_c; ++i) {
    free(second[i]);
  }
  if (rc != 0 && !phase_b_done && created_threads == 0) {
    for (uint64_t i = 0; i < phase_a; ++i) {
      if (first[i]) {
        free(first[i]);
      }
    }
  }
  free(args);
  free(tids);
  free(first_for_count);
  free(second);
  free(first);
  return rc;
}
