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
 * Foreign-thread free. Validates ptr against this page's own bounds
 * (tail-slack / misaligned / interior) and expected_generation exactly
 * like HZ10PageMapRoute-L0's pipeline, then claims the slot's pending bit
 * before pushing -- a second remote free of the same pointer before it
 * drains is rejected as a duplicate rather than corrupting the freelist.
 * Safe to call from any thread, including the page's own owner (though the
 * owner should prefer hz10_freelist_page_free() -- this path is strictly
 * more expensive).
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
