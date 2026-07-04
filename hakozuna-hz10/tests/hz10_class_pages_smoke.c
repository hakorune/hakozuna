#include "../src/hz10_class_pages.h"
#include "../src/hz10_freelist_page.h"
#include "../src/hz10_page_pool.h"
#include "../src/hz10_pagemap.h"

#include <stdio.h>

/*
 * Box 6 (src/hz10_class_pages.{h,c}) had no dedicated smoke of its own --
 * only exercised indirectly through tests/hz10_public_entry_smoke.c. This
 * file targets the bounded active/retired eviction/reclaim behavior
 * directly (no hz10_malloc/hz10_free involved). Extended alongside the
 * active/retired two-list redesign (see hz10_class_pages.h): the earlier
 * one-list, eviction-time-only reclaim measured as an almost-total miss
 * on real workloads, so these cases now also cover the `retired` sublist
 * and hz10_class_pages_sweep_retired().
 */

/* Shared setup: adds `count` freshly created (64u, 16u), never-touched
 * (and therefore already idle) pages to `list`. */
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

/* Same as above, but allocates (and never frees) one slot from each page
 * first, so every one is genuinely NOT idle -- forces an active eviction
 * of one of these to move it into `retired` instead of being reclaimed
 * immediately. */
static int fill_list_with_busy_filler_pages(Hz10ClassPageList* list,
                                           uint32_t count,
                                           const char* case_name) {
  for (uint32_t i = 0; i < count; ++i) {
    Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
    if (!page) {
      fprintf(stderr, "%s: create %u failed\n", case_name, i);
      return 1;
    }
    if (!hz10_freelist_page_alloc(page)) {
      fprintf(stderr, "%s: alloc on filler %u failed\n", case_name, i);
      return 1;
    }
    hz10_class_pages_add(list, page);
  }
  return 0;
}

/* Shared teardown: destroys every page still reachable from either
 * sublist (a page reclaimed by hz10_class_pages_add()/sweep_retired() is
 * already destroyed and not reachable here). */
static void destroy_class_page_list(Hz10ClassPageList* list) {
  for (Hz10FreelistPage* page = list->active.head; page;) {
    Hz10FreelistPage* next = page->next_in_owner_list;
    hz10_freelist_page_destroy(page);
    page = next;
  }
  for (Hz10FreelistPage* page = list->retired.head; page;) {
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

  if (list.active.length != HZ10_CLASS_PAGES_SCAN_LIMIT) {
    fprintf(stderr, "no_eviction: active.length=%u, expected %u\n",
            list.active.length, HZ10_CLASS_PAGES_SCAN_LIMIT);
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

/* Case 2: one more page past the limit evicts the active tail. Every page
 * here is freshly created and never touched, so it is already idle
 * (free_count == slot_count) at creation -- the evicted tail must be
 * reclaimed immediately (destroyed, returned to Box 4's pool), never
 * routed through `retired` at all. */
static int check_eviction_reclaims_idle_tail(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT + 1u,
                                 "reclaim_idle")) {
    return 1;
  }

  if (list.active.length != HZ10_CLASS_PAGES_SCAN_LIMIT) {
    fprintf(stderr,
            "reclaim_idle: active.length=%u, expected %u (one eviction)\n",
            list.active.length, HZ10_CLASS_PAGES_SCAN_LIMIT);
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
  if (list.retired.length != 0u) {
    fprintf(stderr,
            "reclaim_idle: retired.length=%u, expected 0 (idle pages never "
            "route through retired)\n",
            list.retired.length);
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
 * gets evicted from active moves to `retired` -- it must NOT be
 * destroyed (that would be a use-after-free the moment the application
 * frees its still-outstanding pointer), and it must still be safely
 * freeable while sitting in `retired` (list membership was never
 * load-bearing for hz10_free's correctness). Placed first (oldest =
 * active's tail) so it is exactly the page the (SCAN_LIMIT+1)'th add()
 * evicts. */
static int check_eviction_retires_non_idle_tail(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  Hz10FreelistPage* oldest = hz10_freelist_page_create(64u, 16u);
  if (!oldest) {
    fprintf(stderr, "retire_non_idle: create oldest failed\n");
    return 1;
  }
  void* outstanding = hz10_freelist_page_alloc(oldest);
  if (!outstanding) {
    fprintf(stderr, "retire_non_idle: alloc from oldest failed\n");
    return 1;
  }
  hz10_class_pages_add(&list, oldest);

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT,
                                 "retire_non_idle")) {
    return 1;
  }

  if (list.eviction_count != 1u) {
    fprintf(stderr, "retire_non_idle: eviction_count=%llu, expected 1\n",
            (unsigned long long)list.eviction_count);
    failed = 1;
  }
  if (list.eviction_reclaimed_count != 0u) {
    fprintf(stderr,
            "retire_non_idle: eviction_reclaimed_count=%llu, expected 0 "
            "(oldest still has an outstanding slot, must not be reclaimed "
            "yet)\n",
            (unsigned long long)list.eviction_reclaimed_count);
    failed = 1;
  }
  if (list.retired.length != 1u || list.retired.head != oldest) {
    fprintf(stderr,
            "retire_non_idle: expected oldest to be the sole retired "
            "entry (retired.length=%u)\n",
            list.retired.length);
    failed = 1;
  }
  if (list.retired_count != 1u) {
    fprintf(stderr, "retire_non_idle: retired_count=%llu, expected 1\n",
            (unsigned long long)list.retired_count);
    failed = 1;
  }

  /* oldest is no longer in `active`, but must still be safely freeable
   * while it sits in `retired`. */
  hz10_freelist_page_free(oldest, outstanding);
  if (oldest->free_count != oldest->slot_count) {
    fprintf(stderr,
            "retire_non_idle: freeing the outstanding slot while retired "
            "did not register on the (still-alive) page\n");
    failed = 1;
  }

  destroy_class_page_list(&list);
  return failed;
}

/* Case 4: a retired page that becomes idle AFTER entering `retired` (its
 * last slot freed by the "owner" here, standing in for a remote free
 * arriving late in a real workload) is reclaimed the next time
 * hz10_class_pages_sweep_retired() runs -- this is the actual regression
 * test for the fix: the old one-shot eviction-time check could never
 * catch this, only a later re-check can. */
static int check_sweep_reclaims_after_late_free(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  Hz10FreelistPage* target = hz10_freelist_page_create(64u, 16u);
  if (!target) {
    fprintf(stderr, "sweep_reclaim: create target failed\n");
    return 1;
  }
  void* outstanding = hz10_freelist_page_alloc(target);
  if (!outstanding) {
    fprintf(stderr, "sweep_reclaim: alloc from target failed\n");
    return 1;
  }
  hz10_class_pages_add(&list, target);

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT,
                                 "sweep_reclaim")) {
    return 1;
  }
  if (list.retired.length != 1u || list.retired.head != target) {
    fprintf(stderr,
            "sweep_reclaim: expected target to be the sole retired entry "
            "before the late free (retired.length=%u)\n",
            list.retired.length);
    return 1;
  }

  /* The late free: target only becomes idle now, after it is already in
   * `retired`. */
  hz10_freelist_page_free(target, outstanding);

  hz10_class_pages_sweep_retired(&list);

  if (list.retired.length != 0u) {
    fprintf(stderr,
            "sweep_reclaim: retired.length=%u, expected 0 after sweep\n",
            list.retired.length);
    failed = 1;
  }
  if (list.retired_reclaimed_by_sweep_count != 1u) {
    fprintf(stderr,
            "sweep_reclaim: retired_reclaimed_by_sweep_count=%llu, "
            "expected 1\n",
            (unsigned long long)list.retired_reclaimed_by_sweep_count);
    failed = 1;
  }
  if (hz10_page_pool_cached_count() != 1u) {
    fprintf(stderr,
            "sweep_reclaim: pool cached_count=%u, expected 1 -- target was "
            "not actually returned to Box 4 by the sweep\n",
            hz10_page_pool_cached_count());
    failed = 1;
  }

  destroy_class_page_list(&list);
  return failed;
}

/* Case 5: `retired` is unbounded, but hz10_class_pages_sweep_retired()'s
 * budget is not -- if it always restarted its walk from retired's tail,
 * a chronically non-idle page sitting there (`stuck` below, standing in
 * for a page whose real application data genuinely never comes back)
 * would make every sweep call re-examine the same few tail-most
 * positions forever, never reaching newer, already-resolvable entries
 * further toward head. This is the actual regression test for the
 * persistent sweep cursor (retired_sweep_cursor): repeated
 * sweep_retired() calls must eventually rotate past `stuck` and reclaim
 * something (`target`) that became idle closer to head, while leaving
 * `stuck` in place (never falsely reclaimed, since it never becomes
 * idle in this test). */
static int check_sweep_cursor_rotates_past_stuck_tail(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  /* 9 candidates, each busy (allocated, not yet freed): `stuck` first
   * (oldest -> evicted first -> ends up at retired.tail), 7 generic
   * fillers, then `target` last (evicted last -> ends up at
   * retired.head). None are freed yet -- freeing target before it is
   * evicted would let it get reclaimed immediately at eviction time
   * instead of testing the sweep cursor at all. */
  Hz10FreelistPage* stuck = hz10_freelist_page_create(64u, 16u);
  if (!stuck || !hz10_freelist_page_alloc(stuck)) {
    fprintf(stderr, "sweep_rotate: create/alloc stuck failed\n");
    return 1;
  }
  hz10_class_pages_add(&list, stuck);

  Hz10FreelistPage* target = NULL;
  void* target_ptr = NULL;
  for (uint32_t i = 0; i < 8u; ++i) {
    Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
    void* p = page ? hz10_freelist_page_alloc(page) : NULL;
    if (!page || !p) {
      fprintf(stderr, "sweep_rotate: create/alloc candidate %u failed\n", i);
      return 1;
    }
    hz10_class_pages_add(&list, page);
    if (i == 7u) {
      target = page;
      target_ptr = p;
    }
  }

  /* Enough generic fillers to trigger exactly 9 evictions (stuck + the 8
   * candidates above), pushing all 9 into retired, oldest (stuck) at
   * tail through newest (target) at head -- none idle yet. */
  if (fill_list_with_busy_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT,
                                     "sweep_rotate")) {
    return 1;
  }
  if (list.retired.length != 9u || list.retired.tail != stuck ||
      list.retired.head != target) {
    fprintf(stderr,
            "sweep_rotate: expected 9 retired entries, stuck at tail, "
            "target at head (retired.length=%u)\n",
            list.retired.length);
    return 1;
  }

  /* Only now does target become idle -- the late free this design exists
   * to catch, exactly like check_sweep_reclaims_after_late_free, just
   * with target buried behind 8 other retired entries instead of being
   * the only one. */
  hz10_freelist_page_free(target, target_ptr);

  /* HZ10_CLASS_PAGES_SWEEP_BUDGET=4: reaching target (position 8 from
   * tail) needs ceil(9/4)=3 calls if the cursor genuinely advances each
   * time instead of restarting from tail every call (the bug this
   * design fixes -- a tail-restarting sweep would check the same first
   * few tail-most positions on every call and never reach target no
   * matter how many times it is called). */
  hz10_class_pages_sweep_retired(&list);
  hz10_class_pages_sweep_retired(&list);
  hz10_class_pages_sweep_retired(&list);

  if (list.retired_reclaimed_by_sweep_count != 1u) {
    fprintf(stderr,
            "sweep_rotate: retired_reclaimed_by_sweep_count=%llu, expected "
            "1 -- the cursor did not reach target (still stuck near tail?)\n",
            (unsigned long long)list.retired_reclaimed_by_sweep_count);
    failed = 1;
  }
  if (list.retired.length != 8u) {
    fprintf(stderr,
            "sweep_rotate: retired.length=%u, expected 8 after reclaiming "
            "target\n",
            list.retired.length);
    failed = 1;
  }
  if (list.retired.tail != stuck) {
    fprintf(stderr,
            "sweep_rotate: stuck was reclaimed or moved -- it must never "
            "be, since it is never idle\n");
    failed = 1;
  }

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
  if (check_eviction_retires_non_idle_tail()) {
    return 3;
  }
  if (check_sweep_reclaims_after_late_free()) {
    return 4;
  }
  if (check_sweep_cursor_rotates_past_stuck_tail()) {
    return 5;
  }
  puts("hz10_class_pages_smoke ok");
  return 0;
}
