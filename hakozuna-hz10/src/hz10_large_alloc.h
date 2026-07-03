#ifndef HZ10_LARGE_ALLOC_H
#define HZ10_LARGE_ALLOC_H

#include <stddef.h>
#include <stdint.h>

/*
 * HZ10LargeAllocDirect-L0: the path for requests bigger than one
 * HZ10_PAGE_QUANTUM (64KiB) -- src/hz10_public_entry.c rejected these
 * outright (NULL) up through Box 6; this box adds a dedicated direct-mmap
 * path instead of the slotted-page machinery every smaller class uses.
 *
 * Design: a large allocation is registered with Box 1 (src/hz10_pagemap.h)
 * as a single-slot "page" whose slot_size is the exact rounded reservation
 * size and slot_count == 1 -- the *same* classification pipeline every
 * small-class page already uses (tail-slack/misaligned/interior/generation),
 * reused as-is rather than inventing a second one. Only the allocation's
 * own base address is ever registered (one HZ10_PAGE_QUANTUM-aligned
 * quantum's worth of pagemap leaf entry, at the start of a reservation that
 * may span many quanta) -- any pointer landing in a later quantum of the
 * same reservation looks up a *different*, unregistered pagemap slot and
 * correctly comes back MISS, which hz10_free() already treats as reject.
 * Since a large allocation only ever hands out its own base pointer (never
 * interior offsets), this is sufficient: it is never asked to validate an
 * address inside a later quantum of a legitimate allocation.
 *
 * HZ10_PAGEMAP_FLAG_LARGE is registered via
 * hz10_pagemap_register_with_owner_and_flags() so hz10_free() can tell a
 * large allocation's route apart from a small-class Hz10FreelistPage*
 * before it ever casts route.owner -- owner itself is left NULL here (no
 * per-thread ownership concept for large allocations in this box: no
 * cross-thread remote-free path either, free() must happen with Box 1's
 * pagemap lock as the only synchronization, same as register/release
 * always have been).
 *
 * Known, accepted L0 gaps, honestly scoped like every prior box:
 *   - no pooling/reuse of large blocks (unlike Box 4's bounded page pool
 *     for quantum-sized blocks) -- every hz10_large_alloc() is a fresh
 *     mmap, every hz10_large_free() a real munmap. A future box could add
 *     a size-bucketed large-object cache if this matters in practice.
 *   - no cross-thread remote-free path (unlike Box 3's Treiber stack for
 *     small classes) -- hz10_large_free() is same-thread-or-not, it does
 *     not care, since there is no thread-local state to protect (the
 *     pagemap release call is its own synchronization). A large object
 *     freed from a different thread than it was allocated on works
 *     correctly today; it just is not measured as a "local vs remote" row
 *     the way small classes are.
 */

#define HZ10_PAGEMAP_FLAG_LARGE 1u

/*
 * Allocates a dedicated, HZ10_PAGE_QUANTUM-aligned block sized to fit
 * `size` bytes (rounded up to the next HZ10_PAGE_QUANTUM multiple),
 * registers it with Box 1, and returns the block's base address (NULL on
 * invalid size or any allocation/registration failure). Caller (Box 6's
 * hz10_public_entry.c) is expected to have already confirmed size >
 * HZ10_PAGE_QUANTUM -- this function itself only requires size > 0.
 */
void* hz10_large_alloc(size_t size);

/*
 * Frees a pointer this module allocated. Caller must have already routed
 * base through Box 1 and confirmed route.flags == HZ10_PAGEMAP_FLAG_LARGE;
 * rounded_size must be exactly route.slot_size from that same route call
 * (the exact reservation size hz10_large_alloc() registered, not the
 * original request). Releases the pagemap registration and unmaps the
 * block.
 */
void hz10_large_free(void* base, uint32_t rounded_size);

#endif
