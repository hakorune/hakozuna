#include "hz5.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct PlateauResult {
  uint64_t allocs;
  uint64_t nulls;
  uint64_t frees;
  uint64_t ops;
  size_t rss_after_alloc_kb;
  size_t rss_after_free_kb;
  size_t rss_peak_kb;
} PlateauResult;

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static size_t current_rss_kb(void) {
  FILE* fp = fopen("/proc/self/status", "r");
  if (!fp) {
    return 0;
  }
  char line[256];
  size_t rss = 0;
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      (void)sscanf(line + 6, "%zu", &rss);
      break;
    }
  }
  fclose(fp);
  return rss;
}

static void touch_edge_block(void* ptr, size_t size, uint64_t salt) {
  volatile unsigned char* bytes = (volatile unsigned char*)ptr;
  bytes[0] = (unsigned char)salt;
  bytes[size - 1u] = (unsigned char)(salt >> 8);
}

static void touch_resident_block(void* ptr, size_t size, uint64_t salt) {
  const size_t page = 4096u;
  volatile unsigned char* bytes = (volatile unsigned char*)ptr;
  for (size_t offset = 0; offset < size; offset += page) {
    bytes[offset] = (unsigned char)(salt + offset);
  }
  bytes[size - 1u] = (unsigned char)(salt >> 8);
}

static int run_plateau(size_t blocks,
                       int rounds,
                       size_t size,
                       size_t align,
                       int allow_null,
                       PlateauResult* out) {
  void** ptrs = (void**)calloc(blocks, sizeof(*ptrs));
  if (!ptrs) {
    return 3;
  }

  for (int round = 0; round < rounds; ++round) {
    for (size_t i = 0; i < blocks; ++i) {
      void* ptr = hz5_aligned_alloc(size, align);
      if (!ptr) {
        out->nulls++;
        if (!allow_null) {
          fprintf(stderr,
                  "hz5_aligned_alloc failed round=%d block=%zu size=%zu "
                  "align=%zu\n",
                  round, i, size, align);
          free(ptrs);
          return 5;
        }
        continue;
      }
      if (((uintptr_t)ptr & (uintptr_t)(align - 1u)) != 0) {
        fprintf(stderr, "alignment failed ptr=%p align=%zu\n", ptr, align);
        hz5_free(ptr);
        free(ptrs);
        return 6;
      }
      touch_resident_block(ptr, size, (uint64_t)round * blocks + i);
      ptrs[i] = ptr;
      out->allocs++;
      out->ops++;
    }

    out->rss_after_alloc_kb = current_rss_kb();
    if (out->rss_after_alloc_kb > out->rss_peak_kb) {
      out->rss_peak_kb = out->rss_after_alloc_kb;
    }

    for (size_t i = 0; i < blocks; ++i) {
      if (!ptrs[i]) {
        continue;
      }
      if (hz5_free(ptrs[i]) == HZ5_FREE_INVALID) {
        fprintf(stderr, "hz5_free rejected ptr=%p\n", ptrs[i]);
        free(ptrs);
        return 7;
      }
      ptrs[i] = NULL;
      out->frees++;
      out->ops++;
    }

    out->rss_after_free_kb = current_rss_kb();
    if (out->rss_after_free_kb > out->rss_peak_kb) {
      out->rss_peak_kb = out->rss_after_free_kb;
    }
  }

  free(ptrs);
  return 0;
}

int main(int argc, char** argv) {
  size_t prelude_blocks = argc > 1 ? strtoull(argv[1], NULL, 10) : 1024u;
  int prelude_rounds = argc > 2 ? atoi(argv[2]) : 3;
  uint64_t measure_iters = argc > 3 ? strtoull(argv[3], NULL, 10) : 1000000u;
  size_t size = argc > 4 ? strtoull(argv[4], NULL, 10) : 65536u;
  size_t align = argc > 5 ? strtoull(argv[5], NULL, 10) : 8192u;
  size_t probe_size = argc > 6 ? strtoull(argv[6], NULL, 10) : 262144u;
  size_t probe_align = argc > 7 ? strtoull(argv[7], NULL, 10) : 8192u;
  size_t probe_attempts = argc > 8 ? strtoull(argv[8], NULL, 10) : 256u;
  if (prelude_blocks == 0 || prelude_rounds <= 0 || measure_iters == 0 ||
      size == 0 || align == 0 || probe_size == 0 || probe_align == 0 ||
      probe_attempts == 0 || (align & (align - 1u)) != 0 ||
      (probe_align & (probe_align - 1u)) != 0) {
    fprintf(stderr,
            "usage: bench_hz5_standalone_mixed_prelude [prelude_blocks] "
            "[prelude_rounds] [measure_iters] [size] [align] [probe_size] "
            "[probe_align] [probe_attempts]\n");
    return 2;
  }

  PlateauResult prelude = {0};
  prelude.rss_peak_kb = current_rss_kb();
  int status =
      run_plateau(prelude_blocks, prelude_rounds, size, align, 0, &prelude);
  if (status != 0) {
    return status;
  }

  PlateauResult probe = {0};
  probe.rss_peak_kb = prelude.rss_peak_kb;
  status = run_plateau(probe_attempts, 1, probe_size, probe_align, 1, &probe);
  if (status != 0) {
    return status;
  }

  uint64_t measure_ops = 0;
  size_t measure_peak_rss = probe.rss_peak_kb;
  double start = now_sec();
  for (uint64_t i = 0; i < measure_iters; ++i) {
    void* ptr = hz5_aligned_alloc(size, align);
    if (!ptr) {
      fprintf(stderr,
              "measure hz5_aligned_alloc failed iter=%llu size=%zu "
              "align=%zu\n",
              (unsigned long long)i, size, align);
      return 8;
    }
    if (((uintptr_t)ptr & (uintptr_t)(align - 1u)) != 0) {
      fprintf(stderr, "measure alignment failed ptr=%p align=%zu\n", ptr,
              align);
      hz5_free(ptr);
      return 9;
    }
    touch_edge_block(ptr, size, i);
    if (hz5_free(ptr) == HZ5_FREE_INVALID) {
      fprintf(stderr, "measure hz5_free rejected ptr=%p\n", ptr);
      return 10;
    }
    measure_ops += 2u;
  }
  double elapsed = now_sec() - start;
  size_t final_rss = current_rss_kb();
  if (final_rss > measure_peak_rss) {
    measure_peak_rss = final_rss;
  }

  printf("allocator=hz5-standalone mode=mixed-prelude prelude_blocks=%zu "
         "prelude_rounds=%d measure_iters=%llu size=%zu align=%zu "
         "probe_size=%zu probe_align=%zu probe_attempts=%zu "
         "probe_successes=%llu probe_nulls=%llu time=%.6f ops/s=%.3f "
         "pairs/s=%.3f prelude_ops=%llu probe_ops=%llu "
         "rss_after_prelude_kb=%zu rss_after_probe_kb=%zu "
         "rss_final_kb=%zu rss_peak_kb=%zu\n",
         prelude_blocks, prelude_rounds, (unsigned long long)measure_iters,
         size, align, probe_size, probe_align, probe_attempts,
         (unsigned long long)probe.allocs, (unsigned long long)probe.nulls,
         elapsed, (double)measure_ops / elapsed,
         (double)measure_iters / elapsed, (unsigned long long)prelude.ops,
         (unsigned long long)probe.ops, prelude.rss_after_free_kb,
         probe.rss_after_free_kb, final_rss, measure_peak_rss);
  return 0;
}
