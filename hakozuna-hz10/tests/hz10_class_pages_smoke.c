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
 * and hz10_class_pages_harvest_retired_capacity(), including
 * RetiredPartialReuse-L1's promote-on-partial-capacity path.
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

/* Same as above, but allocates (and never frees) EVERY slot from each
 * page first, so every one is genuinely NOT idle AND has zero remaining
 * capacity (local_free_head == NULL) -- forces an active eviction of one
 * of these to move it into `retired` instead of being reclaimed
 * immediately, and (unlike a page with only SOME of its slots allocated)
 * guarantees hz10_class_pages_harvest_retired_capacity() cannot mistake it
 * for a promotable partial-capacity page either: allocating only one of
 * many slots would still leave the rest sitting on local_free_head from
 * page creation, i.e. already "partial" by RetiredPartialReuse-L1's
 * definition -- not the fully-stuck filler these callers need. */
static int fill_list_with_busy_filler_pages(Hz10ClassPageList* list,
                                           uint32_t count,
                                           const char* case_name) {
  for (uint32_t i = 0; i < count; ++i) {
    Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
    if (!page) {
      fprintf(stderr, "%s: create %u failed\n", case_name, i);
      return 1;
    }
    for (uint32_t s = 0; s < 16u; ++s) {
      if (!hz10_freelist_page_alloc(page)) {
        fprintf(stderr, "%s: exhaust filler %u at slot %u failed\n",
                case_name, i, s);
        return 1;
      }
    }
    hz10_class_pages_add(list, page);
  }
  return 0;
}

/* Shared teardown: destroys every page still reachable from either
 * sublist (a page reclaimed by hz10_class_pages_add() or
 * hz10_class_pages_harvest_retired_capacity() is already destroyed and
 * not reachable here). */
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

/* Case 4: a retired page that becomes fully idle AFTER entering `retired`
 * (every slot freed by the "owner" here, standing in for remote frees
 * arriving late in a real workload) is reclaimed (destroyed) the next
 * time hz10_class_pages_harvest_retired_capacity() runs -- this is the
 * actual regression test for the original fix: the old one-shot
 * eviction-time check could never catch this, only a later re-check can.
 * target is fully exhausted (all 16 slots allocated) before eviction so
 * it starts with zero capacity, not partial -- freeing only SOME of its
 * 16 slots would exercise the promote path (case 6) instead of this one's
 * destroy path. */
static int check_sweep_reclaims_after_late_free(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  Hz10FreelistPage* target = hz10_freelist_page_create(64u, 16u);
  if (!target) {
    fprintf(stderr, "sweep_reclaim: create target failed\n");
    return 1;
  }
  void* outstanding[16];
  for (uint32_t i = 0; i < 16u; ++i) {
    outstanding[i] = hz10_freelist_page_alloc(target);
    if (!outstanding[i]) {
      fprintf(stderr, "sweep_reclaim: exhaust target at slot %u failed\n", i);
      return 1;
    }
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
   * `retired`, and only once every one of its 16 slots is back. */
  for (uint32_t i = 0; i < 16u; ++i) {
    hz10_freelist_page_free(target, outstanding[i]);
  }

  Hz10FreelistPage* harvested =
      hz10_class_pages_harvest_retired_capacity(&list);
  if (harvested) {
    fprintf(stderr,
            "sweep_reclaim: harvest returned %p, expected NULL -- target "
            "was idle and should have been destroyed, not promoted\n",
            (void*)harvested);
    failed = 1;
  }
  if (list.retired.length != 0u) {
    fprintf(stderr,
            "sweep_reclaim: retired.length=%u, expected 0 after harvest\n",
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
            "not actually returned to Box 4 by the harvest\n",
            hz10_page_pool_cached_count());
    failed = 1;
  }

  destroy_class_page_list(&list);
  return failed;
}

/* Case 5: `retired` is unbounded, but
 * hz10_class_pages_harvest_retired_capacity()'s budget is not -- if it
 * always restarted its walk from retired's tail, a chronically
 * zero-capacity page sitting there (`stuck` below, standing in for a page
 * whose real application data genuinely never comes back) would make
 * every harvest call re-examine the same few tail-most positions forever,
 * never reaching newer, already-resolvable entries further toward head.
 * This is the actual regression test for the persistent sweep cursor
 * (retired_sweep_cursor): repeated harvest calls must eventually rotate
 * past `stuck` and reclaim something (`target`) that became idle closer
 * to head, while leaving `stuck` in place (never falsely reclaimed or
 * promoted, since it is fully exhausted -- zero capacity, not idle and
 * not partial -- for this entire test). */
static int check_sweep_cursor_rotates_past_stuck_tail(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  /* 9 candidates, each fully exhausted (all 16 slots allocated, none
   * freed yet): `stuck` first (oldest -> evicted first -> ends up at
   * retired.tail), 7 generic fillers, then `target` last (evicted last ->
   * ends up at retired.head). None are freed yet -- freeing target before
   * it is evicted would let it get reclaimed immediately at eviction time
   * instead of testing the sweep cursor at all. Full exhaustion (not just
   * one of 16 slots) matters here specifically: a page with any
   * unallocated slot already has local_free_head != NULL from creation,
   * which RetiredPartialReuse-L1's harvest would treat as promotable
   * partial capacity -- `stuck` needs genuinely zero capacity to stay
   * stuck for this test to mean anything. */
  Hz10FreelistPage* stuck = hz10_freelist_page_create(64u, 16u);
  if (!stuck) {
    fprintf(stderr, "sweep_rotate: create stuck failed\n");
    return 1;
  }
  for (uint32_t s = 0; s < 16u; ++s) {
    if (!hz10_freelist_page_alloc(stuck)) {
      fprintf(stderr, "sweep_rotate: exhaust stuck at slot %u failed\n", s);
      return 1;
    }
  }
  hz10_class_pages_add(&list, stuck);

  Hz10FreelistPage* target = NULL;
  void* target_ptrs[16];
  for (uint32_t i = 0; i < 8u; ++i) {
    Hz10FreelistPage* page = hz10_freelist_page_create(64u, 16u);
    if (!page) {
      fprintf(stderr, "sweep_rotate: create candidate %u failed\n", i);
      return 1;
    }
    if (i == 7u) {
      for (uint32_t s = 0; s < 16u; ++s) {
        target_ptrs[s] = hz10_freelist_page_alloc(page);
        if (!target_ptrs[s]) {
          fprintf(stderr,
                  "sweep_rotate: exhaust target at slot %u failed\n", s);
          return 1;
        }
      }
      target = page;
    } else {
      for (uint32_t s = 0; s < 16u; ++s) {
        if (!hz10_freelist_page_alloc(page)) {
          fprintf(stderr,
                  "sweep_rotate: exhaust candidate %u at slot %u failed\n",
                  i, s);
          return 1;
        }
      }
    }
    hz10_class_pages_add(&list, page);
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

  /* Only now does target become fully idle (every one of its 16 slots
   * freed) -- the late free this design exists to catch, exactly like
   * check_sweep_reclaims_after_late_free, just with target buried behind
   * 8 other retired entries instead of being the only one. */
  for (uint32_t s = 0; s < 16u; ++s) {
    hz10_freelist_page_free(target, target_ptrs[s]);
  }

  /* HZ10_CLASS_PAGES_SWEEP_BUDGET=4: reaching target (position 8 from
   * tail) needs ceil(9/4)=3 calls if the cursor genuinely advances each
   * time instead of restarting from tail every call (the bug this
   * design fixes -- a tail-restarting sweep would check the same first
   * few tail-most positions on every call and never reach target no
   * matter how many times it is called). */
  hz10_class_pages_harvest_retired_capacity(&list);
  hz10_class_pages_harvest_retired_capacity(&list);
  hz10_class_pages_harvest_retired_capacity(&list);

  if (list.retired_reclaimed_by_sweep_count != 1u) {
    fprintf(stderr,
            "sweep_rotate: retired_reclaimed_by_sweep_count=%llu, expected "
            "1 -- the cursor did not reach target (still stuck near tail?)\n",
            (unsigned long long)list.retired_reclaimed_by_sweep_count);
    failed = 1;
  }
  if (list.retired_promoted_by_sweep_count != 0u) {
    fprintf(stderr,
            "sweep_rotate: retired_promoted_by_sweep_count=%llu, expected "
            "0 -- nothing in this test ever has partial-only capacity\n",
            (unsigned long long)list.retired_promoted_by_sweep_count);
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
            "sweep_rotate: stuck was reclaimed, promoted, or moved -- it "
            "must never be, since it never has any capacity in this test\n");
    failed = 1;
  }

  destroy_class_page_list(&list);
  return failed;
}

/* Case 6: a retired page that regains SOME (not all) of its capacity is
 * promoted straight back into `active` by
 * hz10_class_pages_harvest_retired_capacity(), instead of being left in
 * `retired` waiting for every last slot to free -- the actual regression
 * test for RetiredPartialReuse-L1 (see hz10_class_pages.h): a page with
 * hundreds or thousands of slots may realistically never see every one of
 * them freed within any bounded window under sustained churn, so waiting
 * for free_count == slot_count alone measured (current_task.md) as never
 * reclaiming a single main_r50/main_r90-shaped class's page. */
static int check_harvest_promotes_partial_capacity(void) {
  hz10_page_pool_reset_for_tests();
  Hz10ClassPageList list = {0};
  int failed = 0;

  Hz10FreelistPage* candidate = hz10_freelist_page_create(64u, 16u);
  if (!candidate) {
    fprintf(stderr, "harvest_promote: create candidate failed\n");
    return 1;
  }
  void* ptrs[16];
  for (uint32_t i = 0; i < 16u; ++i) {
    ptrs[i] = hz10_freelist_page_alloc(candidate);
    if (!ptrs[i]) {
      fprintf(stderr, "harvest_promote: exhaust candidate at slot %u failed\n",
              i);
      return 1;
    }
  }
  hz10_class_pages_add(&list, candidate);

  if (fill_list_with_filler_pages(&list, HZ10_CLASS_PAGES_SCAN_LIMIT,
                                 "harvest_promote")) {
    return 1;
  }
  if (list.retired.length != 1u || list.retired.head != candidate) {
    fprintf(stderr,
            "harvest_promote: expected candidate to be the sole retired "
            "entry (retired.length=%u)\n",
            list.retired.length);
    return 1;
  }

  /* Free 5 of 16 -- partial capacity, deliberately not all of them. */
  for (uint32_t i = 0; i < 5u; ++i) {
    hz10_freelist_page_free(candidate, ptrs[i]);
  }

  Hz10FreelistPage* harvested =
      hz10_class_pages_harvest_retired_capacity(&list);
  if (harvested != candidate) {
    fprintf(stderr,
            "harvest_promote: harvest returned %p, expected candidate (%p)\n",
            (void*)harvested, (void*)candidate);
    failed = 1;
  }
  if (list.retired.length != 0u) {
    fprintf(stderr,
            "harvest_promote: retired.length=%u, expected 0 after promote\n",
            list.retired.length);
    failed = 1;
  }
  if (list.active.head != candidate) {
    fprintf(stderr,
            "harvest_promote: candidate not found at active's head after "
            "promotion\n");
    failed = 1;
  }
  if (list.retired_promoted_by_sweep_count != 1u) {
    fprintf(stderr,
            "harvest_promote: retired_promoted_by_sweep_count=%llu, "
            "expected 1\n",
            (unsigned long long)list.retired_promoted_by_sweep_count);
    failed = 1;
  }
  if (list.retired_reclaimed_by_sweep_count != 0u) {
    fprintf(stderr,
            "harvest_promote: retired_reclaimed_by_sweep_count=%llu, "
            "expected 0 -- candidate had partial capacity and should have "
            "been promoted, not destroyed\n",
            (unsigned long long)list.retired_reclaimed_by_sweep_count);
    failed = 1;
  }

  /* And it must actually be allocatable now -- this is the entire point
   * of promoting instead of leaving it in retired. */
  if (!hz10_freelist_page_alloc(candidate)) {
    fprintf(stderr,
            "harvest_promote: alloc from promoted candidate failed\n");
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
  if (check_harvest_promotes_partial_capacity()) {
    return 6;
  }
  puts("hz10_class_pages_smoke ok");
  return 0;
}
