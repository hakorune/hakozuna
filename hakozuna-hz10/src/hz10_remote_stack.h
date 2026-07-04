#ifndef HZ10_REMOTE_STACK_H
#define HZ10_REMOTE_STACK_H

#include "hz10_freelist_page.h"

#include <stdint.h>

/*
 * HZ10RemoteStackDrain-L0: Layer 2 of the HZ10 design -- the foreign-free
 * path. Operates on the same Hz10FreelistPage a caller already resolved
 * (e.g. via HZ10PageMapRoute-L0), but is a separate module from Box 2's
 * local mechanics on purpose: Box 2 is "owner thread only, plain
 * load/store"; everything in this module is "any thread, atomics only."
 *
 * Design rule (docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md): remote state is
 * isolated to remote stack / pending authority. A foreign thread never
 * touches page->local_free_head; the owner never merges anything into
 * local_free_head except through hz10_page_drain_remote().
 */

/*
 * Split into claim() + publish() (found necessary, not assumed, while
 * wiring src/hz10_retired_ready.h -- see current_task.md's "polling-vs-
 * event-driven" follow-up entries for the full incident): a caller that
 * wants to do its OWN bookkeeping on `page` in response to an accepted
 * remote free (e.g. HZ10RetiredReadyQueue-L0's outstanding-count
 * decrement) needs a point to do that BEFORE this slot becomes visible
 * to the owner's drain -- otherwise that bookkeeping can race a
 * concurrent destroy the owner performs the instant it observes the
 * slot merged.
 *
 * The invariant that makes this split safe (state plainly, since it is
 * not obvious from either function alone): a claimed-but-not-yet-
 * published slot is not reachable from any stripe of remote_free_head,
 * so hz10_page_drain_remote() cannot merge it, so page->free_count
 * cannot reach page->slot_count while a claim is outstanding for one of
 * its slots -- so the owner CANNOT conclude this page is idle and
 * destroy/recycle it during that window. Do NOT introduce any code path
 * that treats a page as idle by any means other than free_count ==
 * slot_count (e.g. do not let something outside this pair mark a page
 * reclaimable based on the claim alone) -- that would break the
 * invariant this split's safety depends on.
 *
 * hz10_page_remote_free() below remains a thin, complete wrapper
 * (claim then publish back to back) for ordinary callers (tests,
 * benches, anything not doing its own inter-step bookkeeping) --
 * existing callers are unaffected.
 */

/*
 * Validates ptr against this page's own bounds (tail-slack / misaligned
 * / interior) and expected_generation exactly like HZ10PageMapRoute-L0's
 * pipeline, then claims the slot's pending bit -- a second remote free
 * of the same pointer before it drains is rejected as a duplicate rather
 * than corrupting the freelist. Does NOT touch remote_free_head -- the
 * slot is not yet visible to hz10_page_drain_remote() after this call
 * returns 1 (see the module comment above). Returns 1 if ptr was
 * accepted (caller must now call hz10_page_remote_free_publish() to
 * actually free it -- an accepted-but-never-published claim leaves the
 * slot permanently unreachable, the same as silently dropping a free),
 * 0 if rejected (out of range, misaligned, interior, stale generation,
 * or already-pending duplicate -- caller does nothing further).
 */
int hz10_page_remote_free_claim(Hz10FreelistPage* page, void* ptr,
                                uint32_t expected_generation,
                                uint32_t* slot_index_out);

/*
 * Publishes ptr onto remote_free_head (the actual Treiber push) --
 * call ONLY after hz10_page_remote_free_claim() returned 1 for this same
 * (page, ptr) pair. This is the point the slot becomes visible to
 * hz10_page_drain_remote() and, transitively, to the owner's idle
 * check -- nothing may touch `page` from the calling thread after this
 * call returns (see the module comment above for why).
 */
void hz10_page_remote_free_publish(Hz10FreelistPage* page, void* ptr);

/*
 * Foreign-thread free, complete in one call (claim then publish with no
 * gap for the caller to act in between). Safe to call from any thread,
 * including the page's own owner (though the owner should prefer
 * hz10_freelist_page_free() -- this path is strictly more expensive).
 *
 * Returns 1 if ptr was accepted and pushed, 0 if rejected (out of range,
 * misaligned, interior, stale generation, or already-pending duplicate).
 */
int hz10_page_remote_free(Hz10FreelistPage* page, void* ptr,
                          uint32_t expected_generation);

/*
 * Owner-thread only. Atomically claims the entire remote stack in one
 * exchange, then merges every reclaimed slot into local_free_head (plain
 * loads/stores, since only the owner calls this) and clears each slot's
 * pending bit. Returns the number of slots merged.
 */
uint32_t hz10_page_drain_remote(Hz10FreelistPage* page);

#endif
