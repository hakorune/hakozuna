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
 *     paying for a fresh one. The list is now bounded to
 *     HZ10_CLASS_PAGES_SCAN_LIMIT pages, not unbounded: once it would grow
 *     past that, the oldest page is evicted and, if confirmed idle,
 *     returned to Box 4's pool -- see hz10_class_pages.h for the measured
 *     before/after and the accepted tradeoff (a page that was NOT idle
 *     when evicted just stops being searchable, though it remains
 *     correctly freeable via the pagemap route either way)
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

/*
 * Diagnostic accessor, calling thread's own state only (per-class lists
 * are thread-local, see hz10_public_entry.c): reads out the current
 * length and lifetime eviction/reclaim counters (src/hz10_class_pages.h)
 * for class_id's page list, without disturbing them. class_id >=
 * HZ10_CLASS_COUNT is a no-op (all outputs left at 0) rather than
 * undefined behavior, so a caller sweeping every class_id doesn't need
 * its own bounds check. Added to make the main_r50/main_r90 RSS finding
 * in current_task.md checkable against a real workload instead of only
 * the isolated unit test in tests/hz10_class_pages_smoke.c.
 */
void hz10_public_entry_class_list_stats(uint32_t class_id,
                                        uint32_t* length_out,
                                        uint64_t* eviction_count_out,
                                        uint64_t* eviction_reclaimed_count_out);

#endif
