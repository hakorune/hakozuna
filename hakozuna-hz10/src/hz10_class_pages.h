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
 * Known, accepted L0 gap: the list only grows, it is never pruned or
 * capped. A thread that creates many pages for one class under sustained
 * churn keeps all of them findable (correct), but a future box should add
 * a cap and return genuinely-idle pages to Box 4's pool instead of
 * holding them here forever.
 */

typedef struct Hz10ClassPageList {
  Hz10FreelistPage* head;
} Hz10ClassPageList;

/* Prepends page to the list. O(1), does not check for duplicates -- the
 * caller (hz10_public_entry.c) only ever adds a page once, right after
 * creating it. */
void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page);

/*
 * Scans the list looking for a page with capacity, draining each
 * candidate's remote stack first (a page with no local capacity but a
 * pending remote free is exactly the case Box 5's old abandon-on-
 * exhaustion policy lost). Returns the first page found with capacity
 * (including the page just drained into having some), or NULL if every
 * page in the list is genuinely full. O(list length) -- see the module
 * comment's known gap about an unbounded list.
 */
Hz10FreelistPage* hz10_class_pages_find_with_capacity(Hz10ClassPageList* list);

#endif
