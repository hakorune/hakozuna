#include "../src/hz10_pagemap.h"
#include "../src/hz10_page_pool.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_pooled_page.h"

#include <stdint.h>
#include <stdio.h>

#define HZ10_SMOKE_POOL_CAP 4u
#define HZ10_SMOKE_POOL_RELEASES 10u
#define HZ10_SMOKE_CHURN_COUNT 10u

/* Case 1: hz10_page_pool primitive -- cap enforcement and reuse, tested
 * directly against raw quantum blocks (no Hz10FreelistPage involved). */
static int check_pool_cap_and_reuse(void) {
  hz10_page_pool_reset_for_tests();
  hz10_page_pool_set_cap(HZ10_SMOKE_POOL_CAP);
  int failed = 0;

  if (hz10_page_pool_try_acquire() != NULL) {
    fprintf(stderr, "pool: acquire from empty pool returned non-NULL\n");
    failed = 1;
  }

  void* blocks[HZ10_SMOKE_POOL_RELEASES];
  for (uint32_t i = 0; i < HZ10_SMOKE_POOL_RELEASES; ++i) {
    blocks[i] = hz10_platform_reserve_rw(HZ10_PAGE_QUANTUM);
    if (!blocks[i]) {
      fprintf(stderr, "pool: setup reserve failed\n");
      return 1;
    }
  }
  for (uint32_t i = 0; i < HZ10_SMOKE_POOL_RELEASES; ++i) {
    hz10_page_pool_release(blocks[i]);
  }

  if (hz10_page_pool_cached_count() != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "pool: cached_count=%u, expected cap %u\n",
            hz10_page_pool_cached_count(), HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }
  if (hz10_page_pool_release_count() !=
      HZ10_SMOKE_POOL_RELEASES - HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "pool: release_count=%llu, expected %u\n",
            (unsigned long long)hz10_page_pool_release_count(),
            HZ10_SMOKE_POOL_RELEASES - HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }

  uint32_t acquired = 0;
  void* reacquired[HZ10_SMOKE_POOL_CAP];
  void* extra;
  while (acquired < HZ10_SMOKE_POOL_CAP &&
         (extra = hz10_page_pool_try_acquire()) != NULL) {
    reacquired[acquired++] = extra;
  }
  if (acquired != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "pool: acquired %u cached blocks, expected %u\n",
            acquired, HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }
  if (hz10_page_pool_reuse_count() != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "pool: reuse_count=%llu, expected %u\n",
            (unsigned long long)hz10_page_pool_reuse_count(),
            HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }
  if (hz10_page_pool_try_acquire() != NULL) {
    fprintf(stderr, "pool: acquire succeeded after pool should be empty\n");
    failed = 1;
  }

  for (uint32_t i = 0; i < acquired; ++i) {
    hz10_platform_release(reacquired[i], HZ10_PAGE_QUANTUM);
  }
  return failed;
}

/* Case 2: hz10_pooled_page basic correctness -- functionally identical to
 * plain hz10_freelist_page, and destroy() offers its block back to the
 * pool. */
static int check_pooled_page_basic(void) {
  hz10_page_pool_reset_for_tests();
  Hz10FreelistPage* page = hz10_pooled_page_create(64u, 16u);
  if (!page) {
    fprintf(stderr, "pooled_page: create failed\n");
    return 1;
  }
  void* p = hz10_freelist_page_alloc(page);
  int failed = 0;
  if (!p) {
    fprintf(stderr, "pooled_page: alloc failed\n");
    failed = 1;
  } else {
    hz10_freelist_page_free(page, p);
  }
  hz10_pooled_page_destroy(page);
  if (hz10_page_pool_cached_count() != 1u) {
    fprintf(stderr, "pooled_page: expected 1 cached block after destroy, "
                    "got %u\n",
            hz10_page_pool_cached_count());
    failed = 1;
  }
  return failed;
}

/* Case 3: a recycled block is re-registered with a bumped generation, so a
 * route using the pre-recycle generation is rejected as stale -- ties
 * Box 1's generation contract to Box 4's reuse path. */
static int check_pooled_page_bumps_generation_on_reuse(void) {
  hz10_page_pool_reset_for_tests();
  Hz10FreelistPage* page1 = hz10_pooled_page_create(64u, 16u);
  if (!page1) {
    fprintf(stderr, "generation: first create failed\n");
    return 1;
  }
  void* base1 = page1->base;
  uint32_t gen1 = page1->generation;
  hz10_pooled_page_destroy(page1);

  Hz10FreelistPage* page2 = hz10_pooled_page_create(64u, 16u);
  if (!page2) {
    fprintf(stderr, "generation: second create failed\n");
    return 1;
  }
  int failed = 0;
  if (page2->base != base1) {
    fprintf(stderr,
            "generation: expected the single cached block to be reused\n");
    failed = 1;
  }
  if (page2->generation == gen1) {
    fprintf(stderr, "generation: reused block generation did not bump\n");
    failed = 1;
  }
  H10RouteResult stale = hz10_pagemap_route(base1, gen1);
  if (stale.kind == H10_ROUTE_VALID) {
    fprintf(stderr,
            "generation: stale pre-recycle generation still routes VALID\n");
    failed = 1;
  }

  hz10_pooled_page_destroy(page2);
  return failed;
}

/* Case 4: sustained caching pressure keeps the pool bounded at its cap --
 * the deterministic, counter-based proof of "release pressure" that a raw
 * OS RSS measurement cannot give reliably. count pages are all created
 * (kept alive) before any are destroyed, so the destroy phase genuinely
 * exercises cap overflow instead of trivially recycling one block. */
static int check_sustained_churn_bounds_cache(void) {
  hz10_page_pool_reset_for_tests();
  hz10_page_pool_set_cap(HZ10_SMOKE_POOL_CAP);

  Hz10FreelistPage* pages[HZ10_SMOKE_CHURN_COUNT];
  for (uint32_t i = 0; i < HZ10_SMOKE_CHURN_COUNT; ++i) {
    pages[i] = hz10_pooled_page_create(64u, 16u);
    if (!pages[i]) {
      fprintf(stderr, "churn: create %u failed\n", i);
      return 1;
    }
  }
  for (uint32_t i = 0; i < HZ10_SMOKE_CHURN_COUNT; ++i) {
    hz10_pooled_page_destroy(pages[i]);
  }

  int failed = 0;
  if (hz10_page_pool_cached_count() > HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "churn: cached_count=%u exceeds cap=%u\n",
            hz10_page_pool_cached_count(), HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }
  uint64_t expected_releases = HZ10_SMOKE_CHURN_COUNT - HZ10_SMOKE_POOL_CAP;
  if (hz10_page_pool_release_count() != expected_releases) {
    fprintf(stderr, "churn: release_count=%llu, expected %llu\n",
            (unsigned long long)hz10_page_pool_release_count(),
            (unsigned long long)expected_releases);
    failed = 1;
  }
  return failed;
}

/* Case 5: the decommit/aging sweep (hz10_page_pool_purge_idle). Avoids any
 * real-time sleep (which would make this test flaky/slow) by testing the
 * two unambiguous ends of the threshold instead: a huge max_idle_ns must
 * purge nothing this fresh, and a zero max_idle_ns must purge everything
 * currently cached (idle_ns >= 0 always holds, however recently a block
 * was released) -- together these prove the sweep's threshold comparison
 * itself is correct without depending on wall-clock timing in the test. */
static int check_purge_idle(void) {
  hz10_page_pool_reset_for_tests();
  hz10_page_pool_set_cap(HZ10_SMOKE_POOL_CAP);

  for (uint32_t i = 0; i < HZ10_SMOKE_POOL_CAP; ++i) {
    void* block = hz10_platform_reserve_rw(HZ10_PAGE_QUANTUM);
    if (!block || !hz10_page_pool_release(block)) {
      fprintf(stderr, "purge_idle: setup release %u failed\n", i);
      return 1;
    }
  }
  if (hz10_page_pool_cached_count() != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "purge_idle: setup did not fill the cache\n");
    return 1;
  }

  int failed = 0;
  uint32_t purged_none = hz10_page_pool_purge_idle(UINT64_MAX);
  if (purged_none != 0u ||
      hz10_page_pool_cached_count() != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr,
            "purge_idle: a huge idle threshold purged %u (should be 0)\n",
            purged_none);
    failed = 1;
  }

  uint32_t purged_all = hz10_page_pool_purge_idle(0u);
  if (purged_all != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "purge_idle: zero threshold purged %u, expected %u\n",
            purged_all, HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }
  if (hz10_page_pool_cached_count() != 0u) {
    fprintf(stderr,
            "purge_idle: cache should be empty after purging everything\n");
    failed = 1;
  }
  if (hz10_page_pool_purged_count() != HZ10_SMOKE_POOL_CAP) {
    fprintf(stderr, "purge_idle: purged_count=%llu, expected %u\n",
            (unsigned long long)hz10_page_pool_purged_count(),
            HZ10_SMOKE_POOL_CAP);
    failed = 1;
  }
  if (hz10_page_pool_purge_idle(0u) != 0u) {
    fprintf(stderr, "purge_idle: purging an already-empty cache should "
                    "purge 0\n");
    failed = 1;
  }
  return failed;
}

int main(void) {
  hz10_pagemap_reset_for_tests();

  if (check_pool_cap_and_reuse()) {
    return 2;
  }
  if (check_pooled_page_basic()) {
    return 3;
  }
  if (check_pooled_page_bumps_generation_on_reuse()) {
    return 4;
  }
  if (check_sustained_churn_bounds_cache()) {
    return 5;
  }
  if (check_purge_idle()) {
    return 6;
  }

  puts("hz10_bounded_page_pool_smoke ok");
  return 0;
}
