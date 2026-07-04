#ifndef HZ10_RETIRED_READY_H
#define HZ10_RETIRED_READY_H

#include "hz10_freelist_page.h"

/*
 * HZ10RetiredReadyQueue-L0: an event-driven hint layer wired alongside
 * Box 6's harvest sweep (src/hz10_class_pages.{h,c}), not a replacement
 * for it -- see current_task.md's "polling-vs-event-driven" and its
 * wiring follow-up entries for the full context this responds to and the
 * measured effect (a real, large win for slot_count=1-shaped classes;
 * no effect on main_r50/r90, which turned out not to need it at all).
 *
 * The problem this targets: harvest's outflow is rate-limited by how
 * often hz10_malloc's OWN slow path happens to re-examine a given retired
 * page (a budgeted cursor walk, triggered only when the owner's active
 * list runs fully dry) -- measured (current_task.md) to be far slower
 * than inflow for some class shapes. This module instead lets whichever
 * thread frees what it BELIEVES is a retired page's last outstanding
 * slot push that page directly onto a lock-free ready stack, so the
 * owner can find out in O(1) next time it looks, instead of waiting for
 * a rotating scan to happen to land on it.
 *
 * Why this is still a HINT, not a replacement for harvest's authoritative
 * check, even after the two real races below were found and closed (this
 * is the load-bearing design decision here, not an afterthought): a
 * "ready" signal can legitimately arrive BEFORE the slot it is for is
 * actually drainable, by construction -- hz10_retired_ready_note_remote_
 * free() is called between hz10_page_remote_free_claim() and
 * hz10_page_remote_free_publish() (src/hz10_remote_stack.h), i.e. before
 * the slot becomes visible to hz10_page_drain_remote(), so outstanding
 * can reach what it believes is zero (and push the page) slightly ahead
 * of free_count actually catching up. A page popped from `ready` is
 * therefore only ever a CANDIDATE -- the caller is required to re-run
 * the exact same free_count == slot_count / local_free_head check
 * harvest already uses before trusting it. A false positive (popped
 * before free_count catches up, or before the owner's own local free of
 * one of this page's slots -- see below) just means the caller's
 * re-check fails and the page is left exactly where harvest would still
 * find it later -- a wasted queue entry, not a correctness bug. A false
 * negative (a page that resolved but was never pushed) just falls back
 * to being found by harvest's own cursor sweep eventually, same as it
 * does today.
 *
 * Two real bugs were found (via ASan/TSan under sustained, continuously-
 * running workloads that this project's shorter-lived benches never
 * exercised long enough to hit -- see current_task.md) and had to be
 * closed before any of the above tolerance was actually safe rather than
 * assumed:
 *
 * (1) a genuine data race, not just a logical hint-vs-authoritative gap:
 *     this module's own bookkeeping (the outstanding decrement, the
 *     ready-stack push) touches `page` in response to an accepted remote
 *     free -- and the OLD, unsplit hz10_page_remote_free() made that
 *     slot visible to the owner's drain (via its Treiber-stack CAS)
 *     BEFORE returning, meaning the owner could observe the page as
 *     idle and destroy it WHILE this module was still reading/writing
 *     it moments later. Closed by splitting hz10_page_remote_free() into
 *     claim() + publish() (src/hz10_remote_stack.h) and running this
 *     module's note_remote_free() strictly between them: a claimed-but-
 *     not-yet-published slot is not reachable from any remote_free_head
 *     stripe, so it cannot be merged, so free_count cannot reach
 *     slot_count, so the owner cannot conclude the page is idle and
 *     destroy/recycle it, for as long as a claim is outstanding. This
 *     also proves the double-count question this design originally
 *     flagged as merely "probably fine" is actually IMPOSSIBLE: a slot's
 *     note_remote_free() call can only ever run while retired_ready_flag
 *     is already 1 (set by mark(), after that slot's OWN prior
 *     resolution -- if any -- was already folded into the initial
 *     outstanding snapshot via drain), so a slot counted in the initial
 *     snapshot and a slot later decremented by note() are provably
 *     disjoint sets, never the same slot twice.
 * (2) an ABA hazard the data race above does NOT cover: even with (1)
 *     fixed, a `ready` stack ENTRY can still go stale between being
 *     pushed and being popped if the underlying page is destroyed and
 *     its address recycled for an unrelated brand-new page in between
 *     (harvest's budgeted cursor walk never has this problem -- it only
 *     ever walks its own thread-local `retired` list, never a reference
 *     a foreign thread handed it). Closed with retired_ready_generation
 *     (src/hz10_freelist_page.h): the owner must compare a popped
 *     candidate's CURRENT page->generation against the value captured at
 *     mark() time BEFORE touching any other field -- see hz10_class_
 *     pages.c's ready-queue drain loop.
 * (3) the mirror image of (2): the budgeted cursor walk can ALSO take a
 *     page out of `retired` (destroy or promote it) using its own
 *     free_count/local_free_head check, entirely independent of whether
 *     this module still considers that page tracked (flag == 1). Unlike
 *     (2), no address recycling is needed to trigger it -- the SAME live
 *     page is the problem, just no longer where either flag or
 *     `retired`'s own membership says it is. Closed with
 *     hz10_retired_ready_cancel() below: the walk must win a CAS on
 *     flag's 1->0 transition (the same transition note_remote_free()
 *     itself performs) before it is allowed to actually remove the page,
 *     so exactly one of {the walk, a concurrent note_remote_free() that
 *     reaches zero} ever wins, and the loser safely backs off instead of
 *     acting on a page it no longer has any claim to.
 *
 * What is still, honestly, just a missed optimization rather than
 * something this module tracks: if the OWNER thread itself performs the
 * local free (src/hz10_freelist_page.h's hz10_freelist_page_free(), not
 * a remote free) that resolves a retired page's last outstanding slot,
 * note_remote_free() is never called for it (that path does not call
 * into this module), so `outstanding` never reaches zero and this page
 * is never pushed -- it simply falls back to harvest's cursor sweep
 * finding it eventually via the ordinary free_count check, same as
 * before this module existed. Not wired up: doing so would need the
 * owner to know, at local-free time, whether the page it is freeing into
 * is currently retired-ready-tracked, which is a real but small
 * additional plumbing question, not a correctness gap.
 *
 * Threading model: hz10_retired_ready_mark() is owner-thread-only (same
 * rule as everything else that touches a page's retired-list membership).
 * hz10_retired_ready_note_remote_free() is safe from any thread and is
 * meant to be called between hz10_page_remote_free_claim() and
 * hz10_page_remote_free_publish() (src/hz10_remote_stack.h) -- see (1)
 * above for why that placement, not "any time after a successful free,"
 * is load-bearing. hz10_retired_ready_pop() is owner-thread-only (only
 * the owner may ever destroy/reuse/promote a page, same rule Box 6
 * already documents) -- its result must be generation-checked (2) before
 * anything else touches it.
 */

typedef struct Hz10RetiredReadyStack {
  _Atomic(struct Hz10FreelistPage*) head;

  /* Diagnostic only: lifetime count of successful pushes (see
   * hz10_retired_ready_note_remote_free()) -- lets a caller compute
   * "backlog currently sitting in `head`, not yet popped" as
   * push_count minus however many it has itself popped and processed,
   * without needing to walk the stack. Not read or written by pop(). */
  _Atomic(uint64_t) push_count;
} Hz10RetiredReadyStack;

/*
 * Owner-only. Marks `page` as tracked by `stack` with `outstanding` slots
 * still unaccounted for -- normally slot_count - free_count as observed
 * by the owner's own drain_remote() + free_count read at the moment it
 * decides to retire this page (mirrors exactly what
 * hz10_class_pages_harvest_retired_capacity() already computes when
 * checking idle/partial). Caller must not pass outstanding == 0 (a page
 * with no outstanding slots is already reclaimable directly -- there is
 * nothing for this module to track). Also captures page->generation into
 * retired_ready_generation (src/hz10_freelist_page.h) -- see the module
 * comment's bug (2) for why the pop path must compare against this
 * before trusting a popped candidate.
 */
void hz10_retired_ready_mark(Hz10FreelistPage* page,
                             Hz10RetiredReadyStack* stack,
                             uint32_t outstanding);

/*
 * Call from any thread, strictly BETWEEN hz10_page_remote_free_claim()
 * and hz10_page_remote_free_publish() (src/hz10_remote_stack.h) for the
 * same (page, ptr) pair, once claim() has returned 1 (a genuinely-
 * accepted, non-duplicate remote free) -- this placement matters, not
 * just the pairing: see the module comment's bug (1) for why calling
 * this after publish() instead would reopen a real data race. The claim/
 * publish split already guarantees at most one accepted claim per slot
 * ever, so calling this once per accepted claim never double-counts a
 * single slot against itself, and (per bug (1)'s proof) never double-
 * counts against the initial outstanding snapshot either. Cheap no-op
 * (one relaxed load) if `page` was never marked or already reported
 * ready. If this call's decrement is the one that brings the count to
 * what it believes is zero, attempts to claim flag's 1->0 transition via
 * CAS before pushing `page` onto its tracked stack for the owner to
 * later pop -- see the module comment's bug (3) and hz10_retired_ready_
 * cancel() for why this must be a CAS, not a plain store: the owner's
 * budgeted cursor walk may be racing to claim the same transition for
 * itself, and losing that race here means silently not pushing (the
 * page is being taken out of `retired` some other way; see the module
 * comment for why this is only ever a candidate, not a guarantee, even
 * ignoring this particular race).
 */
void hz10_retired_ready_note_remote_free(Hz10FreelistPage* page);

/*
 * Owner-only. Pops and returns one candidate page from `stack` (LIFO), or
 * NULL if empty. The caller MUST, in this order: (1) compare the
 * result's CURRENT page->generation against the value it had at mark()
 * time (retired_ready_generation, src/hz10_freelist_page.h) before
 * touching any other field -- a mismatch means this address has been
 * recycled for an unrelated page since it was pushed (the module
 * comment's bug (2)), and the popped reference must simply be dropped;
 * (2) only once generation matches, re-verify with drain_remote() +
 * free_count/local_free_head before treating the result as reclaimable
 * -- see the module comment.
 */
Hz10FreelistPage* hz10_retired_ready_pop(Hz10RetiredReadyStack* stack);

/*
 * Owner-only. Attempts to claim `page`'s retired_ready_flag 1->0
 * transition via CAS, returning 1 on success (the caller now has
 * exclusive rights: no note_remote_free() call, concurrent or future,
 * will act on this page again) or 0 on failure (flag was already 0 --
 * a concurrent or prior note_remote_free() call already won that same
 * transition instead, and this page either already is, or is about to
 * be, linked onto `stack` for the ready path to find).
 *
 * MUST be called, and its return value obeyed, immediately before the
 * owner takes a retired page out of `retired`-tracking via any path
 * OTHER than popping it from `stack` -- i.e. Box 6's budgeted cursor
 * walk's destroy/promote branches (src/hz10_class_pages.c). Found
 * necessary, not assumed, by a real bug: that walk decides to
 * destroy/promote a page using its own independent free_count/
 * local_free_head check, with no knowledge of whether a mark() cycle is
 * still accumulating for it (flag == 1, outstanding > 0, not yet
 * on_stack) -- and if it took the page anyway (e.g. promoting it into
 * `active`), a LATER note_remote_free() call for one of that page's
 * still-genuinely-outstanding slots would still see stale flag == 1 and
 * decrement/push a page that is no longer in `retired` at all,
 * corrupting `list->retired`'s own bookkeeping (the page cannot be
 * unlinked from a list it no longer belongs to).
 *
 * On success (1), the caller may proceed with the destroy/promote: this
 * is a real CAS race against hz10_retired_ready_note_remote_free()'s own
 * flag-clearing CAS below, not a check-then-act race, so exactly one
 * side wins and the loser correctly backs off (this function's failure
 * case; that function's own symmetric failure case, which makes it
 * return without pushing).
 *
 * On failure (0), the caller MUST leave the page exactly where it is in
 * `retired` and move on -- same tolerance already established for an
 * on_stack node or a ready-loop false positive (see the module comment):
 * a wasted walk step, not a correctness issue, since the page is already
 * headed for (or already sitting in) `stack`.
 */
int hz10_retired_ready_cancel(Hz10FreelistPage* page);

#endif
