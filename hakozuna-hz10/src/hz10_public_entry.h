#ifndef HZ10_PUBLIC_ENTRY_H
#define HZ10_PUBLIC_ENTRY_H

#include <stddef.h>

/*
 * The box that finally wires Box 1 (route) + Box 2 (local) + Box 3
 * (remote) + Box 4 (pool) + the size-class table together into something
 * shaped like real malloc()/free(). This is the first HZ10 module that can
 * produce a row comparable to HZ8/HZ9/tcmalloc/mimalloc's medium_local0/
 * main_local0/medium_r50/main_r90 benches.
 *
 * L0 scope, honestly stated:
 *   - single quantum per page still (no multi-quantum spanning), so
 *     size > HZ10_PAGE_QUANTUM (64KiB) is not supported -- hz10_malloc
 *     returns NULL, matching Box 1-4's existing single-quantum limit
 *   - hz10_malloc(0) returns NULL (no class covers size 0), unlike
 *     standard malloc(0)'s implementation-defined "valid unique pointer"
 *     behavior -- not attempting libc-compatible edge-case parity here
 *   - a page that becomes fully exhausted (even after an exhaustion-time
 *     drain, see hz10_malloc) is abandoned: it stays registered and any
 *     future free -- local or remote -- into it is still correct, but the
 *     module no longer revisits it for new allocations, so its freed
 *     capacity is never reused until a future box adds a per-class
 *     multi-page free-page sweep
 *   - free() takes no generation from the caller (real free() can't carry
 *     one), so it always routes with HZ10_GENERATION_ANY; the
 *     generation-mismatch contract that Box 1-4's own smokes exercise
 *     directly is not reachable through this public API by design, not
 *     an oversight -- see tests/README.md
 */

/* Returns NULL if size is 0, or larger than one HZ10_PAGE_QUANTUM, or on
 * allocation failure. */
void* hz10_malloc(size_t size);

/* ptr == NULL is a no-op success (matches free(NULL)). Returns 1 if freed
 * (locally or via Box 3's remote path), 0 if ptr did not route to a page
 * this module created (fail-closed rejection, matching Box 1's pipeline). */
int hz10_free(void* ptr);

#endif
