#ifndef HZ10_CLASS_PAGES_H
#define HZ10_CLASS_PAGES_H

#include "hz10_freelist_page.h"

/*
 * Fixes the box 5 remote-row regression: a thread's public entry used to
 * track exactly one page per class and abandon it forever the moment it
 * looked exhausted (see current_task.md). Under remote pressure that page
 * often still had a foreign free sitting in its remote stack, just never
 * revisited to drain it -- so its capacity was lost for good, and every
 * later allocation paid a full fresh-page cost instead of a freelist pop.
 * This module tracks every page a thread has ever created for one class,
 * so "exhausted" means "none of my own pages have capacity, even after
 * draining each one," not "the one page I happened to be looking at."
 *
 * Mirrors the shape of HZ8/HZ9's owner-scan-list-then-detached-list
 * cascade (h8_medium_alloc.inc's h8_medium_malloc_class_inner), scoped
 * down to what a single thread's own single-class page set needs.
 *
 * Measured, not assumed: an unbounded scan (the original Box 6 shape)
 * degrades badly on a realistic "allocate a lot, free rarely or never"
 * workload -- a single-threaded probe doing nothing but hz10_malloc(64)
 * in a loop, never freeing, went from ~26M ops/s in the first million
 * calls to ~8.5M ops/s by 30 million calls (list length growing roughly
 * linearly with call count, each failed scan walking the whole thing --
 * textbook O(n^2) over the run). HZ10_CLASS_PAGES_SCAN_LIMIT bounds the
 * `active` list below so per-call search cost stays O(1) regardless of
 * how much churn has happened, at a real, accepted cost: a page whose
 * capacity would only be discovered past the scan window is never found
 * by hz10_malloc again. This does NOT affect correctness of freeing that
 * page's pointers -- list membership here is purely an hz10_malloc-side
 * search structure, never consulted by hz10_free (which routes through
 * Box 1's pagemap and its owner tag directly, independent of either list
 * below).
 *
 * TWO-LIST DESIGN (active + retired), replacing an earlier one-list
 * design that measured as NOT working (see current_task.md's "class-list
 * diagnostic counters" and "active/retired two-list reclaim" entries for
 * the full measured history -- summarized here so the header itself
 * stays honest without needing that file open):
 *
 *   `active`: hz10_malloc's own bounded search structure, unchanged
 *   behavior from before -- hz10_class_pages_add() prepends here, and
 *   once length exceeds HZ10_CLASS_PAGES_SCAN_LIMIT the tail is evicted.
 *   An evicted page that is confirmed idle (free_count == slot_count) at
 *   that instant is reclaimed immediately (hz10_pooled_page_destroy(),
 *   returned to Box 4's pool) -- this was always correct and still
 *   catches the easy case. hz10_class_pages_find_with_capacity() only
 *   ever scans `active`, never `retired` -- this is what keeps the O(1)
 *   scan-cost guarantee SCAN_LIMIT was built for intact.
 *
 *   `retired`: where an evicted-but-NOT-yet-idle page goes instead of
 *   being dropped outright. The reason a one-shot idle check at the
 *   eviction instant measured as an almost total miss (see above): under
 *   real cross-thread churn, a page's last slot is freed by another
 *   thread on ITS OWN schedule, and one snapshot rarely lines up with
 *   that moment. `retired` gives a page more chances, spread out over
 *   time, via hz10_class_pages_sweep_retired() -- called only from
 *   hz10_public_entry.c's SLOW path (right after find_with_capacity()
 *   fails and before a fresh page would be created; a page reclaimed
 *   right here can immediately feed that very call's pool-acquire-first
 *   check), never from the hot per-op alloc path. Each sweep call
 *   re-drains and re-checks up to HZ10_CLASS_PAGES_SWEEP_BUDGET pages
 *   from `retired`'s tail (oldest first) -- bounded, so this cannot
 *   reintroduce the O(n^2) cost SCAN_LIMIT already closed on `active`.
 *   `retired` is itself bounded to HZ10_CLASS_PAGES_RETIRED_LIMIT: once
 *   full, the next non-idle eviction forces retired's own tail through
 *   one final drain+check (idle -> reclaimed; still not idle -> truly
 *   given up, dropped -- the same fate an evicted page always had before
 *   this design existed).
 *
 *   Deliberately NOT done in this version (documented up front, not
 *   discovered later): a `retired` page that a sweep drain finds
 *   PARTIALLY idle (some, not all, slots back) is left in `retired`
 *   rather than promoted back to `active` for reuse. Promoting it would
 *   also raise ops/s (avoids a fresh-page cost), not just RSS, but risks
 *   thrashing if `active` is already at its own SCAN_LIMIT (the newly
 *   promoted page could immediately force another eviction). Left as an
 *   explicit Phase 2 candidate once destroy-only reclaim's real RSS
 *   effect is measured on its own.
 *
 *   Counters below are split by WHICH mechanism reclaimed a page
 *   (eviction-instant vs. retired-overflow vs. budgeted sweep) so real
 *   workloads can show which lever is actually doing the work, not just
 *   whether reclaim happens at all.
 */

#define HZ10_CLASS_PAGES_SCAN_LIMIT 128u
#define HZ10_CLASS_PAGES_RETIRED_LIMIT 128u

/* Pages checked per hz10_class_pages_sweep_retired() call. Kept small on
 * purpose: for a slot_count=1 class under sustained churn, the slow path
 * this is called from can fire on nearly every hz10_malloc call (see
 * current_task.md), so this budget is paid that often -- a conservative
 * default, not yet tuned up against a measured ceiling. */
#define HZ10_CLASS_PAGES_SWEEP_BUDGET 4u

typedef struct Hz10PageSublist {
  Hz10FreelistPage* head;
  Hz10FreelistPage* tail;
  uint32_t length;
} Hz10PageSublist;

typedef struct Hz10ClassPageList {
  Hz10PageSublist active;
  Hz10PageSublist retired;

  /*
   * Plain counters, not atomic: list mutation only ever happens from this
   * class's owning thread (see the module comment's threading note).
   */
  uint64_t eviction_count;           /* active tail-evictions triggered */
  uint64_t eviction_reclaimed_count; /* ...of those, confirmed idle at that
                                      * instant and reclaimed immediately */
  uint64_t retired_count;            /* pages that ever entered retired
                                      * (an eviction that was NOT
                                      * immediately idle) */
  uint32_t max_retired_length;       /* high-water mark, for tuning
                                      * HZ10_CLASS_PAGES_RETIRED_LIMIT */
  uint64_t retired_reclaimed_by_sweep_count;    /* reclaimed later by a
                                                 * budgeted sweep call */
  uint64_t retired_reclaimed_by_overflow_count; /* reclaimed at the moment
                                                 * retired itself overflowed
                                                 * (its own tail's one
                                                 * last chance) */
  uint64_t retired_dropped_count;    /* truly given up: retired overflowed
                                      * and its tail was still not idle */
} Hz10ClassPageList;

/* Prepends page to `list->active`, O(1) amortized: at most one tail
 * eviction (drain attempt + free_count comparison) is performed per
 * call, never a walk of the whole list. An evicted, not-yet-idle page
 * moves to `list->retired` instead of being dropped (see the module
 * comment) -- itself at most one more bounded eviction check if
 * `retired` also overflows. Does not check for duplicates -- the caller
 * (hz10_public_entry.c) only ever adds a page once, right after creating
 * it. */
void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page);

/*
 * Scans up to HZ10_CLASS_PAGES_SCAN_LIMIT pages from the (most-recently-
 * created-first) head of `list->active` looking for one with capacity,
 * draining each candidate's remote stack first (a page with no local
 * capacity but a pending remote free is exactly the case Box 5's old
 * abandon-on-exhaustion policy lost). Returns the first page found with
 * capacity (including one just drained into having some), or NULL if
 * every page within the scan window is genuinely full. Never touches
 * `list->retired` -- see the module comment for why that split exists.
 */
Hz10FreelistPage* hz10_class_pages_find_with_capacity(Hz10ClassPageList* list);

/*
 * Checks up to HZ10_CLASS_PAGES_SWEEP_BUDGET pages from `list->retired`'s
 * tail (oldest first), draining each and reclaiming (destroying, via Box
 * 4's pool) any now confirmed idle. A page not (yet) idle is left in
 * place in `retired` for a future sweep call to re-check -- this
 * function does not give up on anything itself, only retired-overflow in
 * hz10_class_pages_add() does. Call only from a slow path (a
 * hz10_malloc cache-miss), never the hot per-op alloc path -- the fixed
 * budget keeps this O(1) per call regardless of how large `retired` is,
 * but it is still real, non-trivial work (drain_remote calls), unlike
 * the hot path's plain pointer-chasing.
 */
void hz10_class_pages_sweep_retired(Hz10ClassPageList* list);

#endif
