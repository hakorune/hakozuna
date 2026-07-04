#ifndef HZ10_PUBLIC_ENTRY_H
#define HZ10_PUBLIC_ENTRY_H

#include <stddef.h>
#include <stdint.h>

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
 *     yet idle moves to a separate, also-bounded `retired` list instead
 *     of being dropped outright, and hz10_malloc's own slow path (right
 *     after a genuine miss, before paying for a fresh page) sweeps a
 *     small budget of it looking for pages that have since become idle
 *     -- see src/hz10_class_pages.h for the full two-list design and why
 *     an earlier one-shot-at-eviction version measured as an almost-total
 *     miss under real remote-free churn
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
  uint64_t retired_reclaimed_by_overflow_count;
  uint64_t retired_dropped_count;
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

#endif
