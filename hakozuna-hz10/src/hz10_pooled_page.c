#include "hz10_pooled_page.h"
#include "hz10_page_pool.h"

Hz10FreelistPage* hz10_pooled_page_create(uint32_t slot_size,
                                          uint32_t slot_count) {
  void* base = hz10_page_pool_try_acquire();
  Hz10FreelistPage* page =
      hz10_freelist_page_create_with_base(base, slot_size, slot_count);
  if (!page && base) {
    /* Registration failed on the recycled block (bad arguments, most
     * likely) -- offer it back to the pool rather than leaking it. */
    hz10_page_pool_release(base);
  }
  return page;
}

void hz10_pooled_page_destroy(Hz10FreelistPage* page) {
  void* base = hz10_freelist_page_destroy_reclaim_base(page);
  if (base) {
    hz10_page_pool_release(base);
  }
}
