#ifndef HZ10_PUBLIC_ENTRY_H
#define HZ10_PUBLIC_ENTRY_H

#include <stddef.h>
#include <stdint.h>

#include "hz10_class_pages.h"

/*
 * The box that finally wires Box 1 (route) + Box 2 (local) + Box 3
 * (remote) + Box 4 (pool) + the size-class table together into something
 * shaped like real malloc()/free(). This is the first HZ10 module that can
 * produce a row comparable to HZ8/HZ9/tcmalloc/mimalloc's medium_local0/
 * main_local0/medium_r50/main_r90 benches.
 *
 * L0 scope, honestly stated:
 *   - size > HZ10_PAGE_QUANTUM (64KiB) now routes to
 *     src/hz10_large_alloc.h instead of returning NULL: a dedicated
 *     direct-mmap path, one allocation per request, reusing Box 1's exact
 *     classification pipeline via a single-slot registration (see
 *     hz10_large_alloc.h for why only the allocation's own base pointer
 *     needs to be registered). No pooling/reuse and no cross-thread
 *     remote-free batching for large objects yet -- both are accepted,
 *     documented gaps there, not silently missing
 *   - hz10_malloc(0) returns NULL (no class covers size 0), unlike
 *     standard malloc(0)'s implementation-defined "valid unique pointer"
 *     behavior -- not attempting libc-compatible edge-case parity here
 *   - a page that becomes fully exhausted (even after an exhaustion-time
 *     drain) is no longer abandoned: src/hz10_class_pages.h tracks every
 *     page this thread has ever created for a class and scans (draining
 *     each candidate, bounded to HZ10_CLASS_PAGES_SCAN_LIMIT pages) before
 *     paying for a fresh one. The active list is bounded to
 *     HZ10_CLASS_PAGES_SCAN_LIMIT pages; a page evicted from it but not
 *     yet idle moves to a separate, unbounded `retired` list instead
 *     of being dropped outright, and hz10_malloc's own slow path (right
 *     after a genuine miss, before paying for a fresh page) harvests a
 *     small budget of it: a page found fully idle is destroyed, and one
 *     found with only PARTIAL capacity back (not idle, but usable) is
 *     promoted straight back into `active` for immediate reuse instead of
 *     waiting on the rest of its slots to free too. Before that budgeted
 *     scan even runs, a lock-free ready queue (src/hz10_retired_ready.h)
 *     is drained first: any thread's accepted remote free that resolves
 *     a retired page's last known-outstanding slot pushes it there
 *     directly, so the owner can find it in O(1) instead of waiting on
 *     a rotating scan -- measured (current_task.md) to need a single
 *     check-in regardless of population size, against a budgeted scan's
 *     population/budget floor. A candidate from that queue is always
 *     re-verified with the same idle/partial check before being trusted
 *     (it is a hint, not authoritative -- see hz10_retired_ready.h for
 *     why), so this can never itself cause an incorrect reclaim -- see
 *     src/hz10_class_pages.h for the full design, why an earlier
 *     one-shot-at-eviction version measured as an almost-total miss under
 *     real remote-free churn, and why waiting for full idle alone measured
 *     as a dead end for multi-slot classes
 *   - free() takes no generation from the caller (real free() can't carry
 *     one), so it always routes with HZ10_GENERATION_ANY; the
 *     generation-mismatch contract that Box 1-4's own smokes exercise
 *     directly is not reachable through this public API by design, not
 *     an oversight -- see tests/README.md
 */

/* Returns NULL if size is 0 or on allocation failure. size >
 * HZ10_PAGE_QUANTUM is now supported via src/hz10_large_alloc.h. */
void* hz10_malloc(size_t size);

/* ptr == NULL is a no-op success (matches free(NULL)). Returns 1 if freed
 * (locally or via Box 3's remote path), 0 if ptr did not route to a page
 * this module created (fail-closed rejection, matching Box 1's pipeline). */
int hz10_free(void* ptr);

/* Diagnostic snapshot of one class's page-list state (src/
 * hz10_class_pages.h) -- see hz10_public_entry_class_list_stats() below. */
typedef struct Hz10ClassPageListStats {
  uint32_t active_length;
  uint32_t retired_length;
  uint32_t max_retired_length;
  uint64_t eviction_count;
  uint64_t eviction_reclaimed_count;
  uint64_t retired_count;
  uint64_t retired_reclaimed_by_sweep_count;
  uint64_t retired_promoted_by_sweep_count;
  uint64_t harvest_call_count;
  uint64_t retired_reclaimed_by_ready_count;
  uint64_t retired_promoted_by_ready_count;
  uint64_t ready_false_positive_count;
  uint64_t ready_push_count;
  uint64_t ready_stale_generation_count;
  uint64_t sweep_cancel_lost_race_count;
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
  uint64_t active_switch_count;
  uint64_t active_ops_served_sum;
  uint64_t active_ops_served_immediate_count;
  uint64_t second_active_check_count;
  uint64_t second_active_hit_count;
} Hz10ClassPageListStats;

/*
 * Diagnostic accessor, calling thread's own state only (per-class lists
 * are thread-local, see hz10_public_entry.c): fills *stats_out with the
 * current length and lifetime eviction/reclaim counters (src/
 * hz10_class_pages.h) for class_id's page list, without disturbing them.
 * class_id >= HZ10_CLASS_COUNT zero-fills *stats_out rather than
 * undefined behavior, so a caller sweeping every class_id doesn't need
 * its own bounds check. stats_out == NULL is a no-op. Added to make the
 * main_r50/main_r90 RSS finding in current_task.md checkable against a
 * real workload instead of only the isolated unit tests in
 * tests/hz10_class_pages_smoke.c.
 */
void hz10_public_entry_class_list_stats(uint32_t class_id,
                                        Hz10ClassPageListStats* stats_out);

typedef struct Hz10PublicEntryThreadReclaimStats {
  uint64_t pages_seen;
  uint64_t pages_reclaimed;
  uint64_t pages_busy;
  uint64_t pages_deferred_ready;
  uint64_t pages_deferred_cancel;
  uint64_t slots_merged;
} Hz10PublicEntryThreadReclaimStats;

/*
 * HZ10LifecycleFlushContract-L1: an explicit, non-destructor lifecycle
 * flush for the CALLING thread's own TLS state (hz10_class_state in
 * src/hz10_public_entry.c). This is a contract, not a convenience call --
 * calling it outside the precondition below corrupts live allocator state
 * instead of merely wasting work.
 *
 * PRECONDITION (the caller's responsibility; not checked or enforceable
 * from inside this function): the calling thread must already be at a
 * genuine quiescent boundary for every page it has ever created. Nothing,
 * anywhere, may still (a) allocate from, (b) locally free into, or (c)
 * remote-free into one of this thread's pages -- including a remote free
 * that is mid claim()/publish() (src/hz10_remote_stack.h) or an inbox entry
 * some other thread has accepted but not yet handed to hz10_free(). A
 * benchmark's own "barrier, drain every worker inbox, barrier, drain again"
 * protocol satisfies this (see the call site in bench/
 * hz10_public_entry_bench.c). Ordinary pthread exit does NOT satisfy it --
 * a foreign thread can still be actively remote-freeing into a dying
 * thread's page -- which is exactly why this is a manual call, never an
 * automatic pthread destructor, and must not become one without a real
 * ownership/handoff redesign.
 *
 * POSTCONDITION this function guarantees in return: every page the calling
 * thread owns is inspected exactly once. A page is destroyed (returned to
 * Box 4's pool via hz10_pooled_page_destroy()) only after
 * hz10_page_drain_remote() confirms free_count == slot_count for it right
 * then. Any page that is not fully idle, or that this call cannot safely
 * take ownership of (see the load-bearing rule below), is left fully
 * registered in this thread's active/retired lists -- never partially
 * unlinked, never corrupted -- and counted in *stats_out
 * (pages_busy / pages_deferred_ready / pages_deferred_cancel) instead of
 * being silently dropped or destroyed anyway.
 *
 * LOAD-BEARING RULE (do not simplify this away when touching this
 * function): a retired page can be concurrently linked onto its class's
 * `list->ready` stack (src/hz10_retired_ready.h) by a FOREIGN thread's
 * remote free even after this thread's own traffic has gone quiet -- the
 * ready stack's push side is multi-producer by design, not owner-only.
 * Destroying such a page without first popping it via the ready-stack
 * owner-only drain, or without winning hz10_retired_ready_cancel()'s CAS,
 * is exactly the data-race/ABA hazard hz10_retired_ready.h's bugs (1) and
 * (2) already document for the ordinary harvest path
 * (hz10_class_pages_harvest_retired_capacity(), src/hz10_class_pages.c).
 * This function mirrors that same two-step guard (retired_ready_on_stack
 * skip, then hz10_retired_ready_cancel()) for the identical reason -- it is
 * a correctness requirement, not an incidental implementation detail, and
 * removing it to "simplify" the retired-page walk would reopen a real
 * use-after-free/ABA window. pages_deferred_ready/pages_deferred_cancel
 * exist so a caller can observe this happening instead of it being
 * invisible. See tests/hz10_public_entry_smoke.c's
 * check_thread_reclaim_drains_ready_retired_page() for the regression
 * test that exercises exactly this path.
 */
void hz10_public_entry_flush_thread_cache_quiescent(
    Hz10PublicEntryThreadReclaimStats* stats_out);

#endif
