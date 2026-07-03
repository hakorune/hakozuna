#include "../src/hz10_freelist_page.h"
#include "../src/hz10_platform.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Honest public-shaped malloc/free microbench for
 * HZ10ThreadLocalFreelistPage-L0 (bench/README.md). Compares
 * hz10_freelist_page against plain libc malloc/free doing the identical
 * fixed-size alloc/free/touch loop -- system malloc needs no external
 * checkout, so this comparison is always available.
 *
 * Two patterns, both required (bench/README.md): a LIFO-only bench cannot
 * distinguish a real freelist from a depth-1 last-pointer cache, which is
 * exactly the shape that limited HZ9 ProductEntry's free-cache (see
 * current_task.md box 2 notes):
 *   lifo:  alloc, touch, immediately free -- one live object at a time
 *   batch: alloc a batch, touch each, shuffle, free in that shuffled
 *          (non-reverse) order, repeat
 */

static uint64_t hz10_bench_rng_state = 88172645463325252ull;

static uint64_t hz10_bench_rng_next(void) {
  uint64_t x = hz10_bench_rng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  hz10_bench_rng_state = x;
  return x;
}

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

static void hz10_bench_touch(void* p, uint32_t size) {
  unsigned char* c = (unsigned char*)p;
  c[0] = 0xA5u;
  c[size - 1u] = 0x5Au;
}

static void hz10_bench_shuffle(void** items, uint32_t count) {
  for (uint32_t i = count; i > 1u; --i) {
    uint32_t j = (uint32_t)(hz10_bench_rng_next() % i);
    void* tmp = items[i - 1u];
    items[i - 1u] = items[j];
    items[j] = tmp;
  }
}

static int hz10_bench_lifo_hz10(Hz10FreelistPage* page, uint64_t iters,
                               uint32_t slot_size, uint64_t* checksum) {
  for (uint64_t i = 0; i < iters; ++i) {
    void* p = hz10_freelist_page_alloc(page);
    if (!p) {
      fprintf(stderr, "hz10/lifo: alloc failed at iter %llu\n",
              (unsigned long long)i);
      return 1;
    }
    hz10_bench_touch(p, slot_size);
    *checksum += (uint64_t)(uintptr_t)p & 0xFFu;
    hz10_freelist_page_free(page, p);
  }
  return 0;
}

static int hz10_bench_lifo_system(uint64_t iters, uint32_t slot_size,
                                 uint64_t* checksum) {
  for (uint64_t i = 0; i < iters; ++i) {
    void* p = malloc(slot_size);
    if (!p) {
      fprintf(stderr, "system/lifo: alloc failed at iter %llu\n",
              (unsigned long long)i);
      return 1;
    }
    hz10_bench_touch(p, slot_size);
    *checksum += (uint64_t)(uintptr_t)p & 0xFFu;
    free(p);
  }
  return 0;
}

static int hz10_bench_batch_hz10(Hz10FreelistPage* page, uint64_t iters,
                                uint32_t batch, uint32_t slot_size,
                                uint64_t* checksum) {
  void** items = calloc(batch, sizeof(*items));
  if (!items) {
    fprintf(stderr, "hz10/batch: allocation failure\n");
    return 1;
  }
  uint64_t done = 0;
  while (done < iters) {
    for (uint32_t i = 0; i < batch; ++i) {
      void* p = hz10_freelist_page_alloc(page);
      if (!p) {
        fprintf(stderr, "hz10/batch: alloc failed at batch item %u\n", i);
        free(items);
        return 1;
      }
      hz10_bench_touch(p, slot_size);
      items[i] = p;
    }
    hz10_bench_shuffle(items, batch);
    for (uint32_t i = 0; i < batch; ++i) {
      *checksum += (uint64_t)(uintptr_t)items[i] & 0xFFu;
      hz10_freelist_page_free(page, items[i]);
    }
    done += batch;
  }
  free(items);
  return 0;
}

static int hz10_bench_batch_system(uint64_t iters, uint32_t batch,
                                  uint32_t slot_size, uint64_t* checksum) {
  void** items = calloc(batch, sizeof(*items));
  if (!items) {
    fprintf(stderr, "system/batch: allocation failure\n");
    return 1;
  }
  uint64_t done = 0;
  while (done < iters) {
    for (uint32_t i = 0; i < batch; ++i) {
      void* p = malloc(slot_size);
      if (!p) {
        fprintf(stderr, "system/batch: alloc failed at batch item %u\n", i);
        free(items);
        return 1;
      }
      hz10_bench_touch(p, slot_size);
      items[i] = p;
    }
    hz10_bench_shuffle(items, batch);
    for (uint32_t i = 0; i < batch; ++i) {
      *checksum += (uint64_t)(uintptr_t)items[i] & 0xFFu;
      free(items[i]);
    }
    done += batch;
  }
  free(items);
  return 0;
}

static void hz10_bench_report(const char* mech, const char* pattern,
                             uint64_t iters, uint32_t run, uint32_t runs,
                             uint32_t slot_size, uint32_t slot_count,
                             uint64_t elapsed_ns, uint64_t checksum) {
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf(
      "hz10_freelist_page mech=%s pattern=%s threads=1 iters=%llu run=%u/%u "
      "slot_size=%u slot_count=%u seconds=%.6f ops_per_s=%.2f "
      "checksum=%llu\n",
      mech, pattern, (unsigned long long)iters, run, runs, slot_size,
      slot_count, seconds, (double)iters / seconds,
      (unsigned long long)checksum);
}

int main(void) {
  uint64_t iters = hz10_bench_env_u64("ITERS", 20000000ull);
  uint32_t slot_size = (uint32_t)hz10_bench_env_u64("SLOT_SIZE", 64ull);
  uint32_t slot_count = (uint32_t)hz10_bench_env_u64("SLOT_COUNT", 1024ull);
  uint32_t batch = (uint32_t)hz10_bench_env_u64("BATCH", 64ull);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 1ull);
  uint32_t mode = (uint32_t)hz10_bench_env_u64("MODE", 2ull);    /* 2 = both */
  uint32_t pattern = (uint32_t)hz10_bench_env_u64("PATTERN", 2ull); /* 2 = both */

  if (batch == 0u || batch > slot_count) {
    fprintf(stderr, "BATCH must be >= 1 and <= SLOT_COUNT\n");
    return 1;
  }

  Hz10FreelistPage* page = hz10_freelist_page_create(slot_size, slot_count);
  if (!page) {
    fprintf(stderr, "setup: hz10_freelist_page_create failed\n");
    return 1;
  }

  int failed = 0;
  for (uint32_t run = 1; run <= runs && !failed; ++run) {
    if (mode == 0u || mode == 2u) {
      if (pattern == 0u || pattern == 2u) {
        uint64_t checksum = 0;
        uint64_t start = hz10_platform_now_ns();
        failed |= hz10_bench_lifo_hz10(page, iters, slot_size, &checksum);
        uint64_t elapsed = hz10_platform_now_ns() - start;
        if (!failed) {
          hz10_bench_report("hz10_freelist", "lifo", iters, run, runs,
                           slot_size, slot_count, elapsed, checksum);
        }
      }
      if (!failed && (pattern == 1u || pattern == 2u)) {
        uint64_t checksum = 0;
        uint64_t start = hz10_platform_now_ns();
        failed |= hz10_bench_batch_hz10(page, iters, batch, slot_size,
                                       &checksum);
        uint64_t elapsed = hz10_platform_now_ns() - start;
        if (!failed) {
          hz10_bench_report("hz10_freelist", "batch", iters, run, runs,
                           slot_size, slot_count, elapsed, checksum);
        }
      }
    }
    if (!failed && (mode == 1u || mode == 2u)) {
      if (pattern == 0u || pattern == 2u) {
        uint64_t checksum = 0;
        uint64_t start = hz10_platform_now_ns();
        failed |= hz10_bench_lifo_system(iters, slot_size, &checksum);
        uint64_t elapsed = hz10_platform_now_ns() - start;
        if (!failed) {
          hz10_bench_report("system_malloc", "lifo", iters, run, runs,
                           slot_size, slot_count, elapsed, checksum);
        }
      }
      if (!failed && (pattern == 1u || pattern == 2u)) {
        uint64_t checksum = 0;
        uint64_t start = hz10_platform_now_ns();
        failed |=
            hz10_bench_batch_system(iters, batch, slot_size, &checksum);
        uint64_t elapsed = hz10_platform_now_ns() - start;
        if (!failed) {
          hz10_bench_report("system_malloc", "batch", iters, run, runs,
                           slot_size, slot_count, elapsed, checksum);
        }
      }
    }
  }

  hz10_freelist_page_destroy(page);
  return failed ? 1 : 0;
}
