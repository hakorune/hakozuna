#ifndef HZ10_CLASS_PAGES_H
#define HZ10_CLASS_PAGES_H

#include "hz10_freelist_page.h"
#include "hz10_retired_ready.h"

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
 *   check), never from the hot per-op alloc path.
 *
 *   `retired` is deliberately UNBOUNDED -- an earlier version capped it
 *   (HZ10_CLASS_PAGES_RETIRED_LIMIT) and dropped its own tail on
 *   overflow, but measurement (current_task.md) showed the cap was not
 *   the limiting factor: removing it entirely (retired growing to tens
 *   of thousands of pages within a single run) barely changed the
 *   reclaim rate. Unlike `active`, `retired` is never linearly scanned
 *   by hz10_malloc (hz10_class_pages_find_with_capacity() never touches
 *   it) -- only hz10_class_pages_sweep_retired()'s fixed-budget walk
 *   does, so an unbounded `retired` does not reintroduce the O(n^2) scan
 *   cost SCAN_LIMIT exists to prevent on `active`. Its only cost is the
 *   memory itself, which in the worst case (a page's remaining slots
 *   never actually get freed) is no worse than today's pre-existing
 *   status quo of a page just sitting there unreclaimed.
 *
 *   The real fix, found by testing (not guessing) why the earlier
 *   bounded+dropped version measured near-zero reclaims even after
 *   removing the drop: hz10_class_pages_sweep_retired() always restarted
 *   its budgeted walk from `retired`'s CURRENT tail. If the few oldest
 *   entries take a while to resolve (the common case here -- see the
 *   remote-free chain note below), the tail barely advances, so newer
 *   entries arriving at head are never reached at all: classic
 *   head-of-line blocking. Fixed with a persistent, resumable cursor
 *   (retired_sweep_cursor below): each call picks up where the last one
 *   left off (wrapping back to the tail once it reaches the head), so
 *   every retired page gets visited on a roughly even rotation instead
 *   of the same few oldest ones being re-examined every call.
 *
 *   Why a page's last slot can take a while to actually register as
 *   freed, even in a workload where every object IS eventually freed
 *   (worth stating plainly, since it is not obvious from this module
 *   alone): a remote free only pushes onto the OWNING page's remote
 *   stack; nothing updates that page's free_count until the OWNING
 *   thread itself later calls hz10_page_drain_remote() on that specific
 *   page (during its own local alloc, find_with_capacity scan, or this
 *   module's sweep). A caller-side "free" completing is therefore not
 *   the same event as this module observing it -- there is a real,
 *   sometimes long delay between the two, which is exactly what the
 *   persistent cursor is designed to outlast.
 *
 *   RetiredPartialReuse-L1: the destroy-only version above was measured
 *   (current_task.md) to fully solve nothing for multi-slot classes --
 *   waiting for free_count == slot_count is fine for a slot_count=1
 *   page (one free IS the whole page), but a page with hundreds or
 *   thousands of slots may realistically never see every one of them
 *   freed within any bounded window under sustained churn, no matter how
 *   well `retired` is traversed. The fix does not require waiting for
 *   that: hz10_class_pages_harvest_retired_capacity() (replacing the
 *   destroy-only sweep) also checks for PARTIAL capacity
 *   (local_free_head != NULL after a drain -- some, not necessarily all,
 *   slots are back) and, if found, promotes that page back to `active`
 *   for immediate reuse instead of leaving it in `retired` waiting for
 *   100%. This directly avoids a fresh-page-creation cost (RSS AND
 *   ops/s), not just eventually destroying pages that happen to fully
 *   drain. Idle takes priority over partial when both are true (an idle
 *   page is destroyed, not promoted -- no point keeping a genuinely
 *   empty page resident). Promotion reuses the same active-tail-eviction
 *   path hz10_class_pages_add() already has (a promoted page can itself
 *   force another eviction if `active` is already full) -- accepted,
 *   not assumed: even a promote-then-quickly-re-evict round trip is
 *   still cheaper than a fresh mmap-adjacent page creation, and this
 *   only ever happens on the slow path already paying that cost anyway.
 *
 *   Counters below are split by WHICH mechanism reclaimed OR reused a
 *   page (eviction-instant destroy / budgeted-sweep destroy / budgeted-
 *   sweep promote) so real workloads can show which lever is actually
 *   doing the work, not just whether reclaim happens at all.
 *
 *   HZ10RetiredReadyQueue-L0 (src/hz10_retired_ready.{h,c}): measured in
 *   isolation (current_task.md's "polling-vs-event-driven" entry) to
 *   need O(1) owner check-ins to reclaim a population that budgeted
 *   polling alone needs O(population / SWEEP_BUDGET) check-ins for --
 *   harvest's own outflow is capped at (frequency of a genuine miss) x
 *   SWEEP_BUDGET, entirely decoupled from how fast pages actually
 *   resolve, which is exactly what left retired_count == retired_length
 *   (100% retention, 0% outflow) in every main_r50/main_r90 run measured
 *   under RetiredPartialReuse-L1 above. `ready` is drained FIRST, before
 *   the budgeted cursor walk below, on every harvest call: each
 *   candidate popped from it is a HINT ONLY (see hz10_retired_ready.h for
 *   why this cannot be authoritative without much heavier synchronization
 *   machinery) and is re-verified with the exact same idle/partial check
 *   the cursor walk uses before being trusted -- a false positive just
 *   falls through to the cursor walk finding it later (still correctly
 *   linked in `retired` either way), never a correctness risk. A page
 *   evicted-but-not-idle is marked into `ready` (hz10_retired_ready_mark())
 *   at the same point it is prepended to `retired`, and every accepted
 *   remote free (hz10_public_entry.c's hz10_free()) reports itself via
 *   hz10_retired_ready_note_remote_free() -- a single flag check, cheap
 *   and unconditional, for every remote free regardless of whether its
 *   target page is tracked this way.
 */

#define HZ10_CLASS_PAGES_SCAN_LIMIT 128u

#ifndef HZ10_ENABLE_ACTIVE_SCAN_STATS
#define HZ10_ENABLE_ACTIVE_SCAN_STATS 0
#endif

#ifndef HZ10_ENABLE_ACTIVE_MTF
#define HZ10_ENABLE_ACTIVE_MTF 0
#endif

#ifndef HZ10_ENABLE_TWO_SLOT_STATS
#define HZ10_ENABLE_TWO_SLOT_STATS 0
#endif

#define HZ10_CLASS_PAGES_SCAN_DEPTH_HIST_BUCKETS 6u

/* Pages checked per hz10_class_pages_harvest_retired_capacity() call.
 * Kept small on purpose: for a slot_count=1 class under sustained churn,
 * the slow path this is called from can fire on nearly every hz10_malloc
 * call (see current_task.md), so this budget is paid that often -- a
 * conservative default, not yet tuned up against a measured ceiling. */
#define HZ10_CLASS_PAGES_SWEEP_BUDGET 4u

typedef struct Hz10PageSublist {
  Hz10FreelistPage* head;
  Hz10FreelistPage* tail;
  uint32_t length;
} Hz10PageSublist;

typedef struct Hz10ClassPageList {
  Hz10PageSublist active;
  Hz10PageSublist retired;

  /* Where hz10_class_pages_harvest_retired_capacity() resumes next --
   * persists across calls so a fixed-budget walk eventually rotates
   * through all of `retired` instead of always restarting from the tail
   * (see the module comment's head-of-line-blocking note). NULL means
   * "start fresh from retired.tail" (also true once a walk reaches
   * head). */
  Hz10FreelistPage* retired_sweep_cursor;

  /* HZ10RetiredReadyQueue-L0's ready stack for this class list -- pages
   * evicted-but-not-idle are marked against this (hz10_retired_ready_
   * mark()) and any thread's accepted remote free may push one back onto
   * it (hz10_retired_ready_note_remote_free()); only this class's owning
   * thread ever pops from it (hz10_class_pages_harvest_retired_capacity()
   * drains it first, before the budgeted cursor walk). */
  Hz10RetiredReadyStack ready;

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
  uint32_t max_retired_length;       /* high-water mark -- retired is
                                      * unbounded, so this is purely
                                      * informational */
  uint64_t retired_reclaimed_by_sweep_count;  /* destroyed by a harvest
                                              * call (found fully idle) */
  uint64_t retired_promoted_by_sweep_count;   /* moved back to `active` by
                                              * a harvest call (found
                                              * partial, not full, capacity) */
  uint64_t harvest_call_count;        /* diagnostic only: how many times
                                      * hz10_class_pages_harvest_retired_
                                      * capacity() was even called (i.e.
                                      * how often hz10_malloc's slow path
                                      * -- a genuine miss across all of
                                      * `active` -- was reached at all).
                                      * Separates "harvest runs constantly
                                      * but finds nothing in its budget"
                                      * from "harvest is essentially never
                                      * reached for this workload shape",
                                      * which look identical in the two
                                      * counters above alone. */
  uint64_t retired_reclaimed_by_ready_count;  /* destroyed via a `ready`
                                              * candidate (found fully
                                              * idle on re-verify) */
  uint64_t retired_promoted_by_ready_count;   /* moved back to `active`
                                              * via a `ready` candidate
                                              * (found partial capacity
                                              * on re-verify) */
  uint64_t ready_false_positive_count;  /* a `ready` candidate that,
                                        * on re-verify, was neither idle
                                        * nor partial -- left in `retired`
                                        * for the cursor walk to find
                                        * later; see hz10_retired_ready.h */
  uint64_t ready_stale_generation_count;  /* a `ready` candidate whose
                                          * address was recycled for an
                                          * unrelated page between push
                                          * and pop -- dropped without
                                          * being touched further; see
                                          * hz10_retired_ready.h's bug (2) */
  uint64_t sweep_cancel_lost_race_count;  /* the budgeted walk decided to
                                          * destroy/promote a node but lost
                                          * hz10_retired_ready_cancel()'s
                                          * CAS to a concurrent
                                          * note_remote_free() call -- left
                                          * in `retired` for the ready path
                                          * to finish instead; see
                                          * hz10_retired_ready.h's bug (3).
                                          * Expected to be rare (a narrow
                                          * race window); a nonzero count
                                          * confirms the window is real,
                                          * not just theoretical. */

  /*
   * HZ10ActiveScanCost-L0 counters. Updated only when
   * HZ10_ENABLE_ACTIVE_SCAN_STATS=1; otherwise they stay zero and the
   * release path keeps its previous counter cost. The counters live here
   * so public-entry active-cache hits and find_with_capacity() scan/drain
   * work share one diagnostic boundary.
   */
  uint64_t active_cache_hit_count;
  uint64_t active_cache_alloc_fail_count;
  uint64_t active_cache_drain_call_count;
  uint64_t active_cache_drain_fail_count;
  uint64_t active_cache_nonempty_drain_count;
  uint64_t active_cache_slots_merged_count;
  uint64_t active_cache_drain_hit_count;
  uint64_t find_call_count;
  uint64_t find_miss_count;
  uint64_t find_pages_visited_count;
  uint64_t find_drain_call_count;
  uint64_t find_nonempty_drain_count;
  uint64_t find_slots_merged_count;
  uint64_t find_local_hit_count;
  uint64_t find_drain_hit_count;
  uint64_t find_hit_depth_sum;
  uint64_t find_miss_pages_visited_count;
  uint32_t find_hit_depth_max;
  uint64_t find_depth_hist[HZ10_CLASS_PAGES_SCAN_DEPTH_HIST_BUCKETS];

  /*
   * HZ10TwoSlotActivePattern-L0 counters. Updated only when
   * HZ10_ENABLE_TWO_SLOT_STATS=1 (src/hz10_public_entry.c owns the
   * increments, since this is about the fast-path active pointer, not
   * find_with_capacity() itself). Purpose: for the classes where
   * find_with_capacity()'s scan cost concentrates (see current_task.md,
   * class 20/21 at ~93% of scan pages), test whether the active-page
   * cache is ping-ponging between exactly two physical pages, whether a
   * freshly activated page tends to exhaust again almost immediately,
   * and whether remembering a single "prior active" pointer would catch
   * the next miss without paying find_with_capacity()'s full list scan.
   */
  uint64_t active_switch_count;             /* state->active reassigned */
  uint64_t active_ops_served_sum;           /* sum, over every switch, of
                                             * how many allocs the page
                                             * being replaced had served
                                             * since it became active */
  uint64_t active_ops_served_immediate_count; /* ...of those, how many
                                              * served 0 or 1 alloc before
                                              * exhausting again */
  uint64_t second_active_check_count;   /* find_with_capacity() was about
                                        * to be called and a prior-active
                                        * pointer existed to compare
                                        * against */
  uint64_t second_active_hit_count;     /* ...of those, find_with_capacity()
                                        * returned exactly that prior-active
                                        * page -- a hit a single remembered
                                        * pointer would have caught for free */
} Hz10ClassPageList;

#if HZ10_ENABLE_ACTIVE_SCAN_STATS
void hz10_class_pages_note_active_cache_hit(Hz10ClassPageList* list);
void hz10_class_pages_note_active_cache_alloc_fail(Hz10ClassPageList* list);
void hz10_class_pages_note_active_cache_drain(Hz10ClassPageList* list,
                                             uint32_t slots_merged,
                                             int produced_capacity);
#else
static inline void hz10_class_pages_note_active_cache_hit(
    Hz10ClassPageList* list) {
  (void)list;
}

static inline void hz10_class_pages_note_active_cache_alloc_fail(
    Hz10ClassPageList* list) {
  (void)list;
}

static inline void hz10_class_pages_note_active_cache_drain(
    Hz10ClassPageList* list, uint32_t slots_merged, int produced_capacity) {
  (void)list;
  (void)slots_merged;
  (void)produced_capacity;
}
#endif

/* Prepends page to `list->active`, O(1) amortized: at most one tail
 * eviction (drain attempt + free_count comparison) is performed per
 * call, never a walk of the whole list. An evicted, not-yet-idle page
 * moves to `list->retired` instead of being dropped (see the module
 * comment) -- retired is unbounded, so this never itself triggers a
 * second eviction. That same page is also marked against list->ready
 * (hz10_retired_ready_mark()) so an accepted remote free targeting one
 * of its still-outstanding slots can push it back for O(1) discovery
 * instead of waiting on the budgeted cursor walk alone. Does not check
 * for duplicates -- the caller (hz10_public_entry.c) only ever adds a
 * page once, right after creating it. */
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
 * First drains list->ready to empty (see the module comment on
 * HZ10RetiredReadyQueue-L0): each candidate popped is FIRST generation-
 * checked (candidate->generation == candidate->retired_ready_generation)
 * before anything else touches it -- a mismatch means this address was
 * recycled for an unrelated page since it was pushed (hz10_retired_
 * ready.h's bug (2)), so the reference is simply dropped, counted, and
 * draining continues. Only once generation matches is a candidate
 * re-verified with exactly the same idle/partial check the budgeted walk
 * below uses before being trusted (a candidate is only ever a hint --
 * see hz10_retired_ready.h). A candidate found fully idle is unlinked
 * from `retired` and destroyed, and draining continues (more candidates
 * may be waiting, and this is O(1) per candidate, no scan cost). A
 * candidate found with PARTIAL capacity is unlinked from `retired`,
 * promoted back into `list->active`, and returned immediately -- this is
 * the common case this mechanism exists for, so it short-circuits here
 * rather than also running the budgeted walk below on the same call. A
 * candidate that is neither (a race -- see hz10_retired_ready.h) is
 * simply left in `retired`, counted, and draining continues.
 *
 * If `ready` never yields a promotable candidate, checks up to
 * HZ10_CLASS_PAGES_SWEEP_BUDGET pages from `list->retired` via a
 * budgeted cursor walk (the original RetiredPartialReuse-L1 mechanism,
 * unchanged): draining each, destroying on full idle, promoting and
 * returning on partial capacity. Returns NULL if both the ready queue
 * and the budgeted walk are exhausted (or empty) without finding
 * anything promotable -- destroys along either path still happen and
 * are still counted even when NULL is returned.
 *
 * The budgeted walk resumes from list->retired_sweep_cursor (or
 * `retired`'s tail if NULL) and saves where it left off, so a fixed
 * budget still eventually rotates through all of `retired` instead of
 * re-examining the same few oldest entries every call (see the module
 * comment) -- this remains the correctness backstop for anything the
 * ready queue's race window ever misses. Before the walk actually acts on
 * a destroy/promote decision (not before deciding -- only before
 * committing), it must win hz10_retired_ready_cancel()'s CAS first: this
 * walk's own free_count/local_free_head check has no idea whether this
 * module still considers the page tracked, and taking it out from under
 * a still-live mark() cycle without this handshake is exactly
 * hz10_retired_ready.h's bug (3). Losing that CAS just leaves the node in
 * `retired` for the ready path to finish (counted in
 * sweep_cancel_lost_race_count) -- expected to be rare. Call only from a
 * slow path (a hz10_malloc cache-miss, right before it would otherwise
 * pay for a fresh page), never the hot per-op alloc path.
 */
Hz10FreelistPage* hz10_class_pages_harvest_retired_capacity(
    Hz10ClassPageList* list);

#endif
