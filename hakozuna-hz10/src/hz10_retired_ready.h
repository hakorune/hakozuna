#ifndef HZ10_RETIRED_READY_H
#define HZ10_RETIRED_READY_H

#include "hz10_freelist_page.h"

/*
 * HZ10RetiredReadyQueue-L0 (prototype, not yet wired into hz10_malloc/
 * hz10_free): an event-driven hint layer meant to sit alongside Box 6's
 * harvest sweep (src/hz10_class_pages.{h,c}), not replace it -- see
 * current_task.md's "polling-vs-event-driven" entry for the full context
 * this responds to.
 *
 * The problem this targets: harvest's outflow is rate-limited by how
 * often hz10_malloc's OWN slow path happens to re-examine a given retired
 * page (a budgeted cursor walk, triggered only when the owner's active
 * list runs fully dry) -- measured (current_task.md) to be far slower
 * than inflow for main_r50/main_r90-shaped classes. This module instead
 * lets whichever thread frees what it BELIEVES is a retired page's last
 * outstanding slot push that page directly onto a lock-free ready stack,
 * so the owner can find out in O(1) next time it looks, instead of
 * waiting for a rotating scan to happen to land on it.
 *
 * Why this is a HINT, not a replacement for harvest's authoritative check
 * (this is the load-bearing design decision here, not an afterthought):
 * making the outstanding count itself provably race-free would require
 * synchronizing it against the exact instant a page transitions from
 * "actively tracked via Box 3's push+drain" to "tracked via this
 * counter" -- a free that is concurrently in flight right at that
 * boundary could, depending on scheduling, be accounted for by neither
 * side, or (worse) by a stale read that double-counts an event the
 * owner's own retirement-time drain already folded into the initial
 * count. Preventing the double-count case rigorously (the only one that
 * would be unsafe -- see below) is possible but needs machinery
 * (something like a quiescence/epoch scheme) well beyond what a hint
 * layer justifies. Instead: this module never lets its own counter
 * directly authorize a destroy or a reuse. A page it reports via
 * hz10_retired_ready_pop() is only ever a CANDIDATE -- the caller is
 * required to re-run the exact same free_count == slot_count /
 * local_free_head check harvest already uses before trusting it. A
 * false positive (popped too early, or spuriously) just means the
 * caller's re-check fails and the page is left exactly where harvest
 * would still find it later -- a wasted queue entry, not a correctness
 * bug. A false negative (a page that resolved but was never pushed,
 * because the race went the other way) just falls back to being found
 * by harvest's own cursor sweep eventually, same as it does today. Since
 * nothing downstream ever trusts this module's count on its own, only
 * its performance (does it actually reduce discovery latency enough to
 * matter) is worth measuring -- not its exactness.
 *
 * Threading model: hz10_retired_ready_mark() is owner-thread-only (same
 * rule as everything else that touches a page's retired-list membership).
 * hz10_retired_ready_note_remote_free() is safe from any thread and is
 * meant to be called immediately after a hz10_page_remote_free() call
 * that returned 1 (see its own comment for why that pairing matters).
 * hz10_retired_ready_pop() is owner-thread-only (only the owner may ever
 * destroy/reuse/promote a page, same rule Box 6 already documents).
 */

typedef struct Hz10RetiredReadyStack {
  _Atomic(struct Hz10FreelistPage*) head;
} Hz10RetiredReadyStack;

/*
 * Owner-only. Marks `page` as tracked by `stack` with `outstanding` slots
 * still unaccounted for -- normally slot_count - free_count as observed
 * by the owner's own drain_remote() + free_count read at the moment it
 * decides to retire this page (mirrors exactly what
 * hz10_class_pages_harvest_retired_capacity() already computes when
 * checking idle/partial). Caller must not pass outstanding == 0 (a page
 * with no outstanding slots is already reclaimable directly -- there is
 * nothing for this module to track).
 */
void hz10_retired_ready_mark(Hz10FreelistPage* page,
                             Hz10RetiredReadyStack* stack,
                             uint32_t outstanding);

/*
 * Call from any thread, immediately after a hz10_page_remote_free(page,
 * ...) call on the same page returns 1 (a genuinely-accepted, non-
 * duplicate remote free) -- this pairing matters: hz10_page_remote_free()
 * already guarantees at most one success per slot ever (its own pending-
 * bit dedup), so calling this once per success never double-counts a
 * single slot against itself. Cheap no-op (one relaxed load) if `page`
 * was never marked or already reported ready. If this call's decrement
 * is the one that brings the count to what it believes is zero, pushes
 * `page` onto its tracked stack for the owner to later pop -- see the
 * module comment for why this is only ever a candidate, not a guarantee.
 */
void hz10_retired_ready_note_remote_free(Hz10FreelistPage* page);

/*
 * Owner-only. Pops and returns one candidate page from `stack` (LIFO), or
 * NULL if empty. The caller MUST re-verify with drain_remote() +
 * free_count/local_free_head before treating the result as reclaimable
 * -- see the module comment.
 */
Hz10FreelistPage* hz10_retired_ready_pop(Hz10RetiredReadyStack* stack);

#endif
