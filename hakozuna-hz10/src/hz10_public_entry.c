#include "hz10_public_entry.h"
#include "hz10_class_pages.h"
#include "hz10_large_alloc.h"
#include "hz10_pagemap.h"
#include "hz10_pooled_page.h"
#include "hz10_remote_stack.h"
#include "hz10_size_class.h"

typedef struct Hz10ClassState {
  Hz10FreelistPage* active;  /* fast-path cache; always also in `list` */
  Hz10ClassPageList list;    /* every page this thread ever made for the
                              * class -- see src/hz10_class_pages.h */
} Hz10ClassState;

static _Thread_local Hz10ClassState hz10_class_state[HZ10_CLASS_COUNT];
static _Thread_local char hz10_thread_token_storage;
/* &hz10_thread_token_storage is a genuinely distinct address per thread
 * (each thread gets its own TLS instance) -- a standard, portable way to
 * get a per-thread identity token without inventing a thread-id concept. */
#define HZ10_THREAD_TOKEN ((void*)&hz10_thread_token_storage)

void* hz10_malloc(size_t size) {
  if (size == 0u) {
    return NULL;
  }
  if (size > (size_t)HZ10_PAGE_QUANTUM) {
    return hz10_large_alloc(size);
  }
  uint32_t class_id = hz10_size_class_for(size);
  if (class_id >= HZ10_CLASS_COUNT) {
    return NULL;
  }
  Hz10ClassState* state = &hz10_class_state[class_id];

  if (state->active) {
    void* ptr = hz10_freelist_page_alloc(state->active);
    if (ptr) {
      return ptr;
    }
    if (hz10_page_drain_remote(state->active) > 0u) {
      ptr = hz10_freelist_page_alloc(state->active);
      if (ptr) {
        return ptr;
      }
    }
  }

  /* The active page (if any) is exhausted even after draining. Before
   * paying for a fresh page, scan every other page this thread has ever
   * created for this class -- src/hz10_class_pages.h drains each
   * candidate as it checks it, so a page that looked exhausted earlier
   * but has since received a remote free is recovered instead of staying
   * lost forever. This replaces Box 5's original abandon-on-exhaustion
   * policy, which measured 15-17x slower than system malloc on
   * remote-heavy rows (current_task.md). */
  Hz10FreelistPage* found = hz10_class_pages_find_with_capacity(&state->list);
  if (found) {
    state->active = found;
    return hz10_freelist_page_alloc(found);
  }

  uint32_t slot_size = hz10_size_class_slot_size(class_id);
  uint32_t slot_count = hz10_size_class_slot_count(class_id);
  /* _with_owner registers the owner tag in the same pagemap call instead
   * of a second one afterward -- see hz10_pooled_page.h. */
  Hz10FreelistPage* fresh =
      hz10_pooled_page_create_with_owner(slot_size, slot_count);
  if (!fresh) {
    return NULL;
  }
  hz10_freelist_page_set_owner_thread(fresh, HZ10_THREAD_TOKEN);
  hz10_class_pages_add(&state->list, fresh);
  state->active = fresh;
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
  if (route.flags == HZ10_PAGEMAP_FLAG_LARGE) {
    hz10_large_free(route.page_base, route.slot_size);
    return 1;
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
