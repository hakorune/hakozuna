#ifndef HZ10_POOLED_PAGE_H
#define HZ10_POOLED_PAGE_H

#include "hz10_freelist_page.h"

#include <stdint.h>

/*
 * HZ10BoundedPagePool-L0 (Box 4), part 2 of 2: the glue between the
 * generic block cache (src/hz10_page_pool.{h,c}) and Box 2's freelist
 * page. This is the only module that knows about both -- Box 2 stays
 * pool-agnostic (hz10_freelist_page_create_with_base/
 * destroy_reclaim_base don't know where a base comes from or where it
 * goes), and hz10_page_pool stays page-shape-agnostic (it only ever sees
 * raw HZ10_PAGE_QUANTUM blocks).
 */

/*
 * Tries hz10_page_pool_try_acquire() first (skipping a fresh mmap +
 * re-registering the recycled block with the pagemap for slot_size/
 * slot_count -- a bump in generation, same as any HZ10PageMapRoute-L0
 * re-registration); falls back to a fresh hz10_freelist_page_create() if
 * the pool was empty. Functionally identical to
 * hz10_freelist_page_create() either way -- only the cost differs.
 */
Hz10FreelistPage* hz10_pooled_page_create(uint32_t slot_size,
                                          uint32_t slot_count);

/* Same as hz10_pooled_page_create(), but registers the page with itself as
 * Box 1's owner tag in the same registration call
 * (hz10_freelist_page_create_with_base_and_owner()), instead of a caller
 * doing a second register() afterward -- see that function's comment for
 * why the second call was a real, measured cost. */
Hz10FreelistPage* hz10_pooled_page_create_with_owner(uint32_t slot_size,
                                                     uint32_t slot_count);

/*
 * Reclaims page's base via hz10_freelist_page_destroy_reclaim_base() and
 * offers it to hz10_page_pool_release() -- cached if the pool is under
 * cap, actually unmapped if not. Does not drain page's remote stack; same
 * caller responsibility as hz10_freelist_page_destroy() (see
 * tests/README.md).
 */
void hz10_pooled_page_destroy(Hz10FreelistPage* page);

#endif
