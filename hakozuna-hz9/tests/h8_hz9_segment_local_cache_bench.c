#include "../src/h8_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if !defined(H9_SEGMENT_LOCAL_CACHE_L0)
#error "segment local cache bench requires H9_SEGMENT_LOCAL_CACHE_L0"
#endif

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !*value) {
    return fallback;
  }
  char* end = NULL;
  unsigned long long parsed = strtoull(value, &end, 10);
  return end && *end == '\0' ? (uint64_t)parsed : fallback;
}

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int main(void) {
  h8_init();
  uint32_t class_id = (uint32_t)env_u64("CLASS_ID", 5u);
  uint64_t iters = env_u64("ITERS", 10000000u);
  if (!h9_segment_local_cache_debug_put(class_id, 0u)) {
    fprintf(stderr, "segment bench setup failed for class %u\n", class_id);
    return 1;
  }

  uint64_t ok = 0u;
  uint32_t slot = 0u;
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    if (!h9_segment_local_cache_debug_take(class_id, &slot) ||
        !h9_segment_local_cache_debug_free_allocated(class_id, slot)) {
      fprintf(stderr, "segment bench transition failed at iter %llu\n",
              (unsigned long long)i);
      return 2;
    }
    ++ok;
  }
  double elapsed = now_seconds() - start;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  double cycles = (double)ok / elapsed;
  printf("hz9_segment_local_cache_api class=%u iters=%llu seconds=%.6f "
         "cycles_per_s=%.2f ops_per_s=%.2f\n",
         class_id, (unsigned long long)ok, elapsed, cycles, cycles * 2.0);
  return 0;
}
