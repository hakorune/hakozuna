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
 * scan so per-call cost stays O(1) regardless of how large the list has
 * grown, at a real, accepted cost: a page whose capacity would only be
 * discovered past the scan window (e.g. an old page that receives a
 * remote free long after newer pages pushed it deep into the list) is
 * never found by hz10_malloc again, and its slots' capacity is
 * permanently invisible to future allocations of that class. This does
 * NOT affect correctness of freeing that page's pointers -- list
 * membership here is purely an hz10_malloc-side search structure, never
 * consulted by hz10_free (which routes through Box 1's pagemap and its
 * owner tag directly, independent of this list).
 *
 * List length is now bounded to HZ10_CLASS_PAGES_SCAN_LIMIT permanently
 * (the "never pruned, grows without bound" gap from the note above is
 * closed): hz10_class_pages_add() evicts the tail (the oldest, least-
 * recently-created page) whenever the list would grow past the limit.
 * The evicted page gets one last hz10_page_drain_remote() attempt; if
 * that leaves it genuinely idle (free_count == slot_count -- no
 * application-held pointer can reference any of its slots, since idle
 * means every slot that was ever handed out has already come back), it is
 * returned to Box 4's pool via hz10_pooled_page_destroy() instead of
 * being silently kept forever unreachable. This directly targets the
 * slot_count=1 residual gap documented in current_task.md: a 1-slot page
 * is exhausted on every single allocation, so under sustained churn it
 * fell out of the (formerly unbounded but only-128-scanned) window almost
 * immediately, permanently losing its capacity to future allocations and
 * its RSS to the process for good. A page evicted while NOT yet idle is
 * just unlinked, same as today's out-of-window behavior -- no correctness
 * change, since list membership was never load-bearing for hz10_free.
 */

#define HZ10_CLASS_PAGES_SCAN_LIMIT 128u

typedef struct Hz10ClassPageList {
  Hz10FreelistPage* head;
  Hz10FreelistPage* tail;
  uint32_t length;

  /*
   * Plain counters, not atomic: list mutation only ever happens from this
   * class's owning thread (see the module comment's threading note), same
   * reason head/tail/length above are plain fields. Added specifically to
   * make the main_r50/main_r90 RSS finding in current_task.md checkable
   * instead of inferred -- without these there was no way to tell whether
   * a given workload's classes ever actually hit the eviction boundary at
   * all, only to guess from RSS growth after the fact.
   */
  uint64_t eviction_count;           /* tail-evictions triggered (length
                                      * exceeded HZ10_CLASS_PAGES_SCAN_LIMIT) */
  uint64_t eviction_reclaimed_count; /* of those, how many were confirmed
                                      * idle and returned to Box 4's pool
                                      * (the rest were just unlinked, still
                                      * outstanding, see below) */
} Hz10ClassPageList;

/* Prepends page to the list, O(1) amortized: at most one tail eviction
 * (itself O(1) -- a single drain attempt plus a free_count comparison,
 * see the module comment above) is performed per call, never a walk of
 * the whole list. Does not check for duplicates -- the caller
 * (hz10_public_entry.c) only ever adds a page once, right after creating
 * it. */
void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page);

/*
 * Scans up to HZ10_CLASS_PAGES_SCAN_LIMIT pages from the (most-recently-
 * created-first) head of the list looking for one with capacity,
 * draining each candidate's remote stack first (a page with no local
 * capacity but a pending remote free is exactly the case Box 5's old
 * abandon-on-exhaustion policy lost). Returns the first page found with
 * capacity (including one just drained into having some), or NULL if
 * every page within the scan window is genuinely full -- see the module
 * comment above for the bounded-scan tradeoff this implies.
 */
Hz10FreelistPage* hz10_class_pages_find_with_capacity(Hz10ClassPageList* list);

#endif
