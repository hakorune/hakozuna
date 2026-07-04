#include "../src/hz10_class_pages.h"
#include "../src/hz10_freelist_page.h"
#include "../src/hz10_page_pool.h"
#include "../src/hz10_pagemap.h"

#include <stdio.h>

/*
 * Box 6 (src/hz10_class_pages.{h,c}) had no dedicated smoke of its own --
 * only exercised indirectly through tests/hz10_public_entry_smoke.c. This
 * file targets the bounded-list eviction/reclaim behavior directly (no
 * hz10_malloc/hz10_free involved), added alongside the eviction_count/
 * eviction_reclaimed_count counters: those counters exist specifically so
 * the main_r50/main_r90 RSS finding in current_task.md is checkable
 * instead of inferred, and this test is the first thing that actually
 * checks them.
 */

/* Shared setup: adds `count` freshly created (64u, 16u) pages to `list`,
 * used by all three cases below as filler to reach/exceed
 * HZ10_CLASS_PAGES_SCAN_LIMIT. Returns 0 on success. */
static int fill_list_with_filler_pages(Hz10ClassPageList* list, uint32_t count,
                                      const char* case_name) {
  for (uint32_t i = 0; i < count; ++i) {
    Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
    if (!page) {
      fprintf(stderr, "%s: create %u failed\n", case_name, i);
      return 1;
    }
    hz10_class_pages_add(list, page);
  }
  return 0;
}

/* Shared teardown: destroys every page still reachable from `list->head`
 * (a page already evicted/dropped by hz10_class_pages_add() is not
 * reachable here and must be torn down separately by its own case). */
static void destroy_class_page_list(Hz10ClassPageList* list) {
  for (Hz10FreelistPage* page = list->head; page;) {
    Hz10FreelistPage* next = page->next_in_owner_list;
    hz10_freelist_page_destroy(page);
    page = next;
  }
}

/* Case 1: adding exactly HZ10_CLASS_PAGES_SCAN_LIMIT pages never evicts. */
static int check_no_eviction_within_limit(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT,
                                 "no_eviction")) {
    return 1;
  }

  if (list.length != HZ10_CLASS_PAGES_SCAN_LIMIT) {
    fprintf(stderr, "no_eviction: length=%u, expected %u\n", list.length,
            HZ10_CLASS_PAGES_SCAN_LIMIT);
    failed = 1;
  }
  if (list.eviction_count != 0u) {
    fprintf(stderr, "no_eviction: eviction_count=%llu, expected 0\n",
            (unsigned long long)list.eviction_count);
    failed = 1;
  }

  destroy_class_page_list(&list);
  return failed;
}

/* Case 2: one more page past the limit evicts the tail. Every page here
 * is freshly created and never touched, so it is already idle
 * (free_count == slot_count) at creation -- the evicted tail must be
 * reclaimed (destroyed, returned to Box 4's pool), not just dropped. */
static int check_eviction_reclaims_idle_tail(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT + 1u,
                                 "reclaim_idle")) {
    return 1;
  }

  if (list.length != HZ10_CLASS_PAGES_SCAN_LIMIT) {
    fprintf(stderr, "reclaim_idle: length=%u, expected %u (one eviction)\n",
            list.length, HZ10_CLASS_PAGES_SCAN_LIMIT);
    failed = 1;
  }
  if (list.eviction_count != 1u) {
    fprintf(stderr, "reclaim_idle: eviction_count=%llu, expected 1\n",
            (unsigned long long)list.eviction_count);
    failed = 1;
  }
  if (list.eviction_reclaimed_count != 1u) {
    fprintf(stderr,
            "reclaim_idle: eviction_reclaimed_count=%llu, expected 1 "
            "(the evicted page was idle and should have been reclaimed)\n",
            (unsigned long long)list.eviction_reclaimed_count);
    failed = 1;
  }
  /* A fresh reset_for_tests() leaves the pool empty with the default cap
   * (64, well above 1), so a single destroy() must land as a cached
   * block here, not a real release -- cached_count is the right signal,
   * not reuse/release_count (those only move on a later acquire or an
   * over-cap release, neither of which applies to this one call). */
  if (hz10_page_pool_cached_count() != 1u) {
    fprintf(stderr,
            "reclaim_idle: pool cached_count=%u, expected 1 -- the "
            "evicted page was not actually returned to Box 4\n",
            hz10_page_pool_cached_count());
    failed = 1;
  }

  destroy_class_page_list(&list);
  return failed;
}

/* Case 3: a non-idle page (one slot still allocated, never freed) that
 * gets evicted must be dropped, not destroyed -- destroying it would be a
 * use-after-free the moment the application frees its still-outstanding
 * pointer. Placed first (oldest = tail) so it is exactly the page the
 * (SCAN_LIMIT+1)'th add() evicts. */
static int check_eviction_drops_non_idle_tail(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  Hz10FreelistPage* oldest = hz10_freelist_page_create(64u, 16u);
  if (!oldest) {
    fprintf(stderr, "drop_non_idle: create oldest failed\n");
    return 1;
  }
  void* outstanding = hz10_freelist_page_alloc(oldest);
  if (!outstanding) {
    fprintf(stderr, "drop_non_idle: alloc from oldest failed\n");
    return 1;
  }
  hz10_class_pages_add(&list, oldest);

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT,
                                 "drop_non_idle")) {
    return 1;
  }

  if (list.eviction_count != 1u) {
    fprintf(stderr, "drop_non_idle: eviction_count=%llu, expected 1\n",
            (unsigned long long)list.eviction_count);
    failed = 1;
  }
  if (list.eviction_reclaimed_count != 0u) {
    fprintf(stderr,
            "drop_non_idle: eviction_reclaimed_count=%llu, expected 0 "
            "(oldest still has an outstanding slot, must not be destroyed)\n",
            (unsigned long long)list.eviction_reclaimed_count);
    failed = 1;
  }

  /* oldest is no longer in `list` (evicted), but must still be safely
   * freeable -- list membership was never load-bearing for correctness. */
  hz10_freelist_page_free(oldest, outstanding);
  if (oldest->free_count != oldest->slot_count) {
    fprintf(stderr,
            "drop_non_idle: freeing the outstanding slot after eviction "
            "did not register on the (still-alive) page\n");
    failed = 1;
  }
  hz10_freelist_page_destroy(oldest);

  destroy_class_page_list(&list);
  return failed;
}

int main(void) {
  if (check_no_eviction_within_limit()) {
    return 1;
  }
  if (check_eviction_reclaims_idle_tail()) {
    return 2;
  }
  if (check_eviction_drops_non_idle_tail()) {
    return 3;
  }
  puts("hz10_class_pages_smoke ok");
  return 0;
}
