/* HZ11 fixed-size local alloc/free bench. Allocator-agnostic: uses plain
 * malloc/free so the SAME binary compares glibc / tcmalloc / mimalloc / hz10 /
 * hz11 via LD_PRELOAD (the gate script). LIFO pattern (alloc, touch, free) so
 * the front cache is exercised at steady state; reports ops/s and ns/op where
 * one op = one (alloc + free) pair.
 *
 * Env: SLOT_SIZE (default 64), ITERS (default 20,000,000), TOUCH (default 1). */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void) {
  size_t slot = (size_t)strtoull(getenv("SLOT_SIZE") ? getenv("SLOT_SIZE") : "64",
                                 NULL, 10);
  if (slot == 0u) {
    slot = 64u;
  }
  unsigned long long iters =
      strtoull(getenv("ITERS") ? getenv("ITERS") : "20000000", NULL, 10);
  if (iters == 0u) {
    iters = 20000000ull;
  }

  /* warmup so the cache is at steady state before timing */
  for (int i = 0; i < 1024; ++i) {
    void* p = malloc(slot);
    if (p) {
      ((char*)p)[0] = (char)i;
      free(p);
    }
  }

  struct timespec t0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  volatile char sink = 0;
  for (unsigned long long i = 0; i < iters; ++i) {
    char* p = (char*)malloc(slot);
    if (!p) {
      fprintf(stderr, "malloc failed at iter %llu\n", i);
      return 1;
    }
    *p = (char)i;       /* touch first byte */
    p[slot - 1u] = (char)i; /* touch last byte */
    sink ^= *p;
    free(p);
  }
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);

  double elapsed =
      (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  double ops = (double)iters / elapsed;
  double ns_per_op = elapsed * 1e9 / (double)iters;
  printf("hz11_fixed_local_bench slot=%zu iters=%llu seconds=%.6f ops_per_s=%.2f "
         "ns_per_op=%.3f\n",
         slot, iters, elapsed, ops, ns_per_op);
  fprintf(stderr, "volatile_sink=%d\n", (int)sink);
  return 0;
}
