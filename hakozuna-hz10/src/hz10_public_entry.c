#include "hz10_public_entry.h"
#include "hz10_pagemap.h"
#include "hz10_pooled_page.h"
#include "hz10_remote_stack.h"
#include "hz10_size_class.h"

static _Thread_local Hz10FreelistPage* hz10_active_pages[HZ10_CLASS_COUNT];
static _Thread_local char hz10_thread_token_storage;
/* &hz10_thread_token_storage is a genuinely distinct address per thread
 * (each thread gets its own TLS instance) -- a standard, portable way to
 * get a per-thread identity token without inventing a thread-id concept. */
#define HZ10_THREAD_TOKEN ((void*)&hz10_thread_token_storage)

void* hz10_malloc(size_t size) {
  uint32_t class_id = hz10_size_class_for(size);
  if (class_id >= HZ10_CLASS_COUNT) {
    return NULL;
  }

  Hz10FreelistPage* page = hz10_active_pages[class_id];
  if (page) {
    void* ptr = hz10_freelist_page_alloc(page);
    if (ptr) {
      return ptr;
    }
    /* Exhausted: absorb any remote frees that arrived before giving up on
     * this page and paying for a fresh one. */
    if (hz10_page_drain_remote(page) > 0u) {
      ptr = hz10_freelist_page_alloc(page);
      if (ptr) {
        return ptr;
      }
    }
    /* Still empty: abandon it. Known L0 gap, see header comment -- this
     * page stays correctly freeable (local or remote) forever, it is just
     * never revisited for new allocations by this module. */
  }

  uint32_t slot_size = hz10_size_class_slot_size(class_id);
  uint32_t slot_count = hz10_size_class_slot_count(class_id);
  Hz10FreelistPage* fresh = hz10_pooled_page_create(slot_size, slot_count);
  if (!fresh) {
    return NULL;
  }
  hz10_freelist_page_set_owner_thread(fresh, HZ10_THREAD_TOKEN);
  /* hz10_pooled_page_create() only knows Box 2's plain
   * hz10_freelist_page_create_with_base(), which registers via Box 1's
   * owner-less hz10_pagemap_register(). Re-registering here with the
   * owner tag is what lets a later route() on one of this page's
   * pointers hand back `fresh` itself. This bumps generation again (any
   * re-registration does), so fresh->generation is updated to match --
   * otherwise Box 3's remote_free() would compare a foreign caller's
   * freshly-routed generation against a stale cached value and reject
   * every legitimate remote free with GENERATION_STALE. */
  uint32_t generation =
      hz10_pagemap_register_with_owner(fresh->base, slot_size, slot_count,
                                       fresh);
  if (generation == 0u) {
    hz10_pooled_page_destroy(fresh);
    return NULL;
  }
  fresh->generation = generation;

  hz10_active_pages[class_id] = fresh;
  return hz10_freelist_page_alloc(fresh); /* fresh page: never NULL */
}

int hz10_free(void* ptr) {
  if (!ptr) {
    return 1;
  }
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  if (route.kind != H10_ROUTE_VALID) {
    return 0;
  }
  Hz10FreelistPage* page = (Hz10FreelistPage*)route.owner;
  if (!page) {
    return 0;
  }
  if (page->owner_thread_token == HZ10_THREAD_TOKEN) {
    hz10_freelist_page_free(page, ptr);
    return 1;
  }
  return hz10_page_remote_free(page, ptr, route.generation) ? 1 : 0;
}
