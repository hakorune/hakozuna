/* HZ11ThreadCacheFastPath-L0 smoke: valid-C correctness for the front cache.
 * Exercises alloc across the size range, write/readback, LIFO reuse, refill,
 * realloc token bookkeeping (grow/shrink), large passthrough, and token
 * hit/miss. Asserts basic counter sanity. Does NOT test double-free / interior /
 * stale (speed-mode non-goals). */
#include "hz11.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, msg)                       \
  do {                                         \
    if (!(cond)) {                             \
      fprintf(stderr, "FAIL: %s\n", msg);      \
      failures++;                              \
    }                                          \
  } while (0)

static void fill(void* p, size_t n, uint8_t v) {
  memset(p, v, n);
}
static int ok_fill(void* p, size_t n, uint8_t v) {
  for (size_t i = 0; i < n; ++i) {
    if (((uint8_t*)p)[i] != v) {
      return 0;
    }
  }
  return 1;
}

int main(void) {
  /* 1. alloc across the cached size range + large */
  static const size_t sizes[] = {16, 64, 256, 4096, 70000};
  void* ptrs[5];
  for (int i = 0; i < 5; ++i) {
    ptrs[i] = hz11_malloc(sizes[i]);
    CHECK(ptrs[i] != NULL, "alloc returned NULL");
    fill(ptrs[i], sizes[i], (uint8_t)(0xA0 + i));
  }
  for (int i = 0; i < 5; ++i) {
    CHECK(ok_fill(ptrs[i], sizes[i], (uint8_t)(0xA0 + i)), "readback mismatch");
  }
  for (int i = 0; i < 5; ++i) {
    hz11_free(ptrs[i]);
  }

  /* 2. LIFO reuse: alloc/free the same class repeatedly -> cache should serve */
  for (int i = 0; i < 1000; ++i) {
    void* p = hz11_malloc(64);
    CHECK(p != NULL, "lifo alloc NULL");
    ((uint8_t*)p)[0] = (uint8_t)i;
    ((uint8_t*)p)[63] = (uint8_t)i;
    CHECK(((uint8_t*)p)[0] == (uint8_t)i && ((uint8_t*)p)[63] == (uint8_t)i,
          "lifo readback");
    hz11_free(p);
  }

  /* 3. realloc grow + shrink keeps data and stays usable */
  char* p = (char*)hz11_malloc(32);
  CHECK(p != NULL, "realloc seed alloc");
  memset(p, 0x5A, 32);
  char* q = (char*)hz11_realloc(p, 200);
  CHECK(q != NULL, "realloc grow");
  if (q) {
    int good = 1;
    for (int i = 0; i < 32; ++i) {
      if (q[i] != 0x5A) {
        good = 0;
      }
    }
    CHECK(good, "realloc grow preserved data");
    memset(q, 0x5A, 200);
    char* r = (char*)hz11_realloc(q, 48);
    CHECK(r != NULL, "realloc shrink");
    if (r) {
      int good2 = 1;
      for (int i = 0; i < 48; ++i) {
        if (r[i] != 0x5A) {
          good2 = 0;
        }
      }
      CHECK(good2, "realloc shrink preserved data");
      hz11_free(r);
    }
  }

  /* 4. large alloc passthrough */
  void* big = hz11_malloc(1u << 20); /* 1 MiB, well past the 64KiB cache ceiling */
  CHECK(big != NULL, "large alloc");
  if (big) {
    fill(big, 4096, 0xC3);
    CHECK(ok_fill(big, 4096, 0xC3), "large readback");
    hz11_free(big);
  }

  /* 5. realloc(NULL, n) == malloc, realloc(p, 0) frees */
  void* pn = hz11_realloc(NULL, 128);
  CHECK(pn != NULL, "realloc(NULL,n)");
  void* pz = hz11_realloc(pn, 0);
  CHECK(pz == NULL, "realloc(p,0) returns NULL");

  /* 6. counter sanity (lane-aware) */
  H11Stats s;
  hz11_stats(&s);
  CHECK(s.malloc_count > 0, "stats: malloc_count");
  CHECK(s.free_count > 0, "stats: free_count");
#if HZ11_CLASSIFY_SPAN
  CHECK(s.direct_hit_count > 0, "stats: direct_hit (LIFO exercised)");
#else
  CHECK(s.token_hit > 0, "stats: token_hit (LIFO exercised)");
#endif
  fprintf(stderr,
          "hz11_thread_cache_smoke[%s] stats: malloc=%llu hit=%llu refill=%llu "
          "free=%llu token_hit=%llu token_miss=%llu direct_hit=%llu "
          "direct_miss=%llu span_create=%llu overflow=%llu flush=%llu "
          "flush_items=%llu cached_bytes=%zu\n",
#if HZ11_CLASSIFY_SPAN
          "span",
#else
          "token",
#endif
          (unsigned long long)s.malloc_count, (unsigned long long)s.malloc_hit,
          (unsigned long long)s.refill_count, (unsigned long long)s.free_count,
          (unsigned long long)s.token_hit, (unsigned long long)s.token_miss,
          (unsigned long long)s.direct_hit_count,
          (unsigned long long)s.direct_miss_count,
          (unsigned long long)s.span_create_count,
          (unsigned long long)s.overflow_count,
          (unsigned long long)s.flush_count,
          (unsigned long long)s.flush_items, s.cached_bytes);

  if (failures) {
    fprintf(stderr, "hz11_thread_cache_smoke: %d FAILURES\n", failures);
    return 1;
  }
  fprintf(stderr, "hz11_thread_cache_smoke ok\n");
  return 0;
}
