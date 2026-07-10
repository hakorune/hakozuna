/* HZ11ThreadCacheFastPath-L0 smoke: valid-C correctness for the front cache.
 * Exercises alloc across the size range, write/readback, LIFO reuse, refill,
 * realloc token bookkeeping (grow/shrink), large passthrough, and token
 * hit/miss. Asserts basic counter sanity. Does NOT test double-free / interior /
 * stale (speed-mode non-goals). */
#include "hz11.h"
#include "hz11_size_class.h"

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
  /* HZ11SizeTableStaticInit-L1: the smoke links core directly (no shim
   * constructor), so initialize the table here before any use. */
  hz11_size_class_init();

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

  /* 5b. exhaustive 1..65536 size-class check.
   * HZ11SizeTableStaticInit-L1: verify the initialized table matches the
   * formula for EVERY size, catching table corruption or future class-map
   * edits. Computes the expected class the same way the init loop does and
   * compares against hz11_size_class(). */
  for (size_t s = 1u; s <= HZ11_MAX_CACHED_SIZE; ++s) {
    size_t size_max = s;  /* the max size for this query is s itself */
    uint8_t expected = 0;
    size_t slot = hz11_class_slot_size(expected);
    while (slot < size_max && (expected + 1u) < HZ11_CLASS_COUNT) {
      expected += 1u;
      slot = hz11_class_slot_size(expected);
    }
    uint8_t got = hz11_size_class(s);
    if (got != expected) {
      fprintf(stderr, "FAIL: size-class mismatch at size %zu: expected %u got %u\n",
              s, expected, got);
      failures++;
    }
  }
  /* size 0 and >64KiB edge cases */
  CHECK(hz11_size_class(0) == 0u, "size_class(0) -> class 0");
  CHECK(hz11_size_class(HZ11_MAX_CACHED_SIZE + 1u) == HZ11_LARGE_CLASS,
        "size_class(>64K) -> LARGE");

#if HZ11_TRANSFER_CENTRAL_SPAN
  /* 5c. Transfer lane stress: force thread-cache overflow, transfer-cache
   * fill, central-stack spill, then allocate enough to reuse both transfer and
   * central objects before carving more spans. */
  enum { TRANSFER_STRESS_N = 1100 };
  void* many[TRANSFER_STRESS_N];
  for (int i = 0; i < TRANSFER_STRESS_N; ++i) {
    many[i] = hz11_malloc(64);
    CHECK(many[i] != NULL, "transfer stress alloc");
  }
  for (int i = 0; i < TRANSFER_STRESS_N; ++i) {
    hz11_free(many[i]);
  }
  for (int i = 0; i < TRANSFER_STRESS_N; ++i) {
    many[i] = hz11_malloc(64);
    CHECK(many[i] != NULL, "transfer stress realloc");
  }
  for (int i = 0; i < TRANSFER_STRESS_N; ++i) {
    hz11_free(many[i]);
  }
#endif

  /* 6. counter sanity (lane-aware + counters-flag-aware).
   *   When HZ11_ENABLE_HOT_COUNTERS is off (speed lane), all hot counters are
   *   compile-time eliminated and read as zero. Only the correctness checks
   *   above (alloc/free/reuse/realloc/large) must pass in BOTH lanes. */
  H11Stats s;
  hz11_stats(&s);
#if HZ11_ENABLE_HOT_COUNTERS
  CHECK(s.malloc_count > 0, "stats: malloc_count");
  CHECK(s.free_count > 0, "stats: free_count");
#if HZ11_CLASSIFY_SPAN
  CHECK(s.direct_hit_count > 0, "stats: direct_hit (LIFO exercised)");
#else
  CHECK(s.token_hit > 0, "stats: token_hit (LIFO exercised)");
#endif
#else
  /* speed lane: hot counters are zero by design -- verify they ARE zero */
  CHECK(s.malloc_count == 0, "stats: malloc_count zero (counters off)");
  CHECK(s.free_count == 0, "stats: free_count zero (counters off)");
#endif
#if HZ11_TRANSFER_CENTRAL_SPAN
#if HZ11_TRANSFER_STATS
  CHECK(s.transfer_insert > 0, "transfer stats: transfer_insert");
  CHECK(s.transfer_remove_hit > 0, "transfer stats: transfer_remove_hit");
  CHECK(s.transfer_insert_spill > 0, "transfer stats: transfer_insert_spill");
  CHECK(s.central_insert > 0, "transfer stats: central_insert");
  CHECK(s.central_remove_hit > 0, "transfer stats: central_remove_hit");
#else
  CHECK(s.transfer_insert == 0, "transfer stats: transfer_insert zero");
  CHECK(s.transfer_remove_hit == 0,
        "transfer stats: transfer_remove_hit zero");
  CHECK(s.transfer_insert_spill == 0,
        "transfer stats: transfer_insert_spill zero");
  CHECK(s.central_insert == 0, "transfer stats: central_insert zero");
  CHECK(s.central_remove_hit == 0,
        "transfer stats: central_remove_hit zero");
#endif
#endif
  fprintf(stderr,
          "hz11_thread_cache_smoke[%s] stats: malloc=%llu hit=%llu refill=%llu "
          "free=%llu token_hit=%llu token_miss=%llu direct_hit=%llu "
          "direct_miss=%llu span_create=%llu overflow=%llu flush=%llu "
          "flush_items=%llu cached_bytes=%zu xfer_hit=%llu xfer_insert=%llu "
          "xfer_spill=%llu central_hit=%llu central_insert=%llu\n",
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
          (unsigned long long)s.flush_items, s.cached_bytes,
          (unsigned long long)s.transfer_remove_hit,
          (unsigned long long)s.transfer_insert,
          (unsigned long long)s.transfer_insert_spill,
          (unsigned long long)s.central_remove_hit,
          (unsigned long long)s.central_insert);

  if (failures) {
    fprintf(stderr, "hz11_thread_cache_smoke: %d FAILURES\n", failures);
    return 1;
  }
  fprintf(stderr, "hz11_thread_cache_smoke ok\n");
  return 0;
}
