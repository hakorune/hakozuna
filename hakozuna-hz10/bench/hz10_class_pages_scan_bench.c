#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_public_entry.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Regression bench for src/hz10_class_pages.h's bounded scan. A workload
 * that allocates from one class and rarely or never frees is realistic
 * (a growing cache, a long-lived arena), not adversarial -- and before
 * HZ10_CLASS_PAGES_SCAN_LIMIT existed, this exact shape measured a real
 * O(n^2) degradation: ~26M ops/s in the first million hz10_malloc calls
 * dropping to ~8.5M ops/s by 30 million (see current_task.md). Single
 * threaded and never-free by design -- this isolates the scan cost
 * itself, not allocator throughput in general (that is
 * hz10_public_entry_bench's job).
 *
 * Reports ops/s per CHECKPOINT-sized segment so a reader can see the
 * trend directly (flat == bounded scan doing its job; declining ==
 * something reintroduced unbounded scan cost).
 */

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

int main(void) {
  uint64_t total = hz10_bench_env_u64("ITERS", 20000000ull);
  uint64_t checkpoint = hz10_bench_env_u64("CHECKPOINT", 1000000ull);
  size_t size = (size_t)hz10_bench_env_u64("SIZE", 64ull);

  if (total == 0u || checkpoint == 0u) {
    fprintf(stderr, "invalid ITERS/CHECKPOINT\n");
    return 1;
  }

  hz10_pagemap_reset_for_tests();

  uint64_t first_seg_ops_per_s = 0u;
  uint64_t last_seg_ops_per_s = 0u;
  uint64_t seg_start = hz10_platform_now_ns();
  uint64_t seg_index = 0u;

  for (uint64_t i = 1; i <= total; ++i) {
    void* p = hz10_malloc(size); /* intentionally never freed */
    if (!p) {
      fprintf(stderr, "hz10_malloc failed at iter %llu\n",
              (unsigned long long)i);
      return 1;
    }

    if (i % checkpoint == 0u) {
      uint64_t now = hz10_platform_now_ns();
      double seconds = (double)(now - seg_start) / 1000000000.0;
      double ops_per_s = seconds > 0.0 ? (double)checkpoint / seconds : 0.0;
      printf("hz10_class_pages_scan seg=%llu iters_done=%llu "
             "seg_ops_per_s=%.2f\n",
             (unsigned long long)seg_index, (unsigned long long)i, ops_per_s);
      if (seg_index == 0u) {
        first_seg_ops_per_s = (uint64_t)ops_per_s;
      }
      last_seg_ops_per_s = (uint64_t)ops_per_s;
      seg_start = now;
      seg_index += 1u;
    }
  }

  if (first_seg_ops_per_s > 0u) {
    double ratio = (double)last_seg_ops_per_s / (double)first_seg_ops_per_s;
    printf("hz10_class_pages_scan summary first_seg_ops_per_s=%llu "
           "last_seg_ops_per_s=%llu last_over_first=%.3f\n",
           (unsigned long long)first_seg_ops_per_s,
           (unsigned long long)last_seg_ops_per_s, ratio);
  }
  return 0;
}
