#include "hz10_class_pages.h"
#include "hz10_pooled_page.h"
#include "hz10_remote_stack.h"

/* Generic doubly-linked splice helpers shared by `active` and `retired`
 * (a page is in at most one of the two sublists at a time, so reusing
 * Hz10FreelistPage's prev/next_in_owner_list fields for whichever list
 * currently holds it is safe -- same "inert storage" rule Box 2 already
 * documents for those fields). */

static void hz10_page_sublist_prepend(Hz10PageSublist* sub,
                                     Hz10FreelistPage* page) {
  page->prev_in_owner_list = NULL;
  page->next_in_owner_list = sub->head;
  if (sub->head) {
    sub->head->prev_in_owner_list = page;
  } else {
    sub->tail = page;
  }
  sub->head = page;
  sub->length += 1u;
}

/* Precondition: sub->tail != NULL (caller checks length first). */
static Hz10FreelistPage* hz10_page_sublist_pop_tail(Hz10PageSublist* sub) {
  Hz10FreelistPage* node = sub->tail;
  sub->tail = node->prev_in_owner_list;
  if (sub->tail) {
    sub->tail->next_in_owner_list = NULL;
  } else {
    sub->head = NULL;
  }
  sub->length -= 1u;
  return node;
}

/* Unlinks `node` from wherever it sits in `sub` (not necessarily an end). */
static void hz10_page_sublist_remove(Hz10PageSublist* sub,
                                    Hz10FreelistPage* node) {
  if (node->prev_in_owner_list) {
    node->prev_in_owner_list->next_in_owner_list = node->next_in_owner_list;
  } else {
    sub->head = node->next_in_owner_list;
  }
  if (node->next_in_owner_list) {
    node->next_in_owner_list->prev_in_owner_list = node->prev_in_owner_list;
  } else {
    sub->tail = node->prev_in_owner_list;
  }
  sub->length -= 1u;
}

/* Shared by hz10_class_pages_add() (a brand-new page) and
 * hz10_class_pages_harvest_retired_capacity() (a page promoted back from
 * `retired`): prepend to `active`'s head, and if that pushes length past
 * SCAN_LIMIT, evict the tail -- destroying it if a drain confirms it is
 * now fully idle, otherwise handing it to `retired` instead of dropping
 * it (see the module comment). At most one eviction per call, same O(1)
 * amortized cost either caller relies on. */
static void hz10_class_pages_prepend_active_with_eviction(
    Hz10ClassPageList* list, Hz10FreelistPage* page) {
  hz10_page_sublist_prepend(&list->active, page);

  if (list->active.length <= HZ10_CLASS_PAGES_SCAN_LIMIT) {
    return;
  }

  /* `page` was just prepended at active's head, so active's tail here is
   * always a different node, and active.length > SCAN_LIMIT >= 1
   * guarantees at least two nodes -- pop_tail's precondition holds. */
  Hz10FreelistPage* evict = hz10_page_sublist_pop_tail(&list->active);
  list->eviction_count += 1u;

  hz10_page_drain_remote(evict);
  if (evict->free_count == evict->slot_count) {
    list->eviction_reclaimed_count += 1u;
    hz10_pooled_page_destroy(evict);
    return;
  }

  /* Not idle yet: give it more chances in `retired` instead of dropping
   * it outright (see the module comment for why a single eviction-time
   * check measured as an almost-total miss under real remote-free
   * churn). `retired` is unbounded -- see the module comment for why
   * that is safe and necessary (a bounded version with its own overflow
   * eviction was tried and measured to not meaningfully help). */
  hz10_page_sublist_prepend(&list->retired, evict);
  list->retired_count += 1u;
  if (list->retired.length > list->max_retired_length) {
    list->max_retired_length = list->retired.length;
  }
}

void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page) {
  hz10_class_pages_prepend_active_with_eviction(list, page);
}

Hz10FreelistPage* hz10_class_pages_find_with_capacity(
    Hz10ClassPageList* list) {
  uint32_t scanned = 0u;
  for (Hz10FreelistPage* page = list->active.head;
       page && scanned < HZ10_CLASS_PAGES_SCAN_LIMIT;
       page = page->next_in_owner_list, ++scanned) {
    if (page->local_free_head) {
      return page;
    }
    if (hz10_page_drain_remote(page) > 0u) {
      return page;
    }
  }
  return NULL;
}

Hz10FreelistPage* hz10_class_pages_harvest_retired_capacity(
    Hz10ClassPageList* list) {
  /* Resume from the cursor left by the last call (or `retired`'s tail if
   * there wasn't one, or the last call walked off the head end) -- see
   * the module comment for why restarting from the tail every call
   * measured as a head-of-line-blocking dead end. */
  list->harvest_call_count += 1u;

  Hz10FreelistPage* node = list->retired_sweep_cursor;
  if (!node) {
    node = list->retired.tail;
  }

  uint32_t checked = 0u;
  while (node && checked < HZ10_CLASS_PAGES_SWEEP_BUDGET) {
    /* Save prev (toward head) before this node possibly gets unlinked. */
    Hz10FreelistPage* prev = node->prev_in_owner_list;
    hz10_page_drain_remote(node);

    if (node->free_count == node->slot_count) {
      /* Fully idle: nothing left to reuse, just reclaim it. */
      hz10_page_sublist_remove(&list->retired, node);
      list->retired_reclaimed_by_sweep_count += 1u;
      hz10_pooled_page_destroy(node);
    } else if (node->local_free_head) {
      /* Partial capacity: don't wait for the rest to come back too --
       * promote it into `active` right now and hand it to the caller,
       * who is about to pay for a fresh page otherwise (see the module
       * comment on RetiredPartialReuse-L1). */
      hz10_page_sublist_remove(&list->retired, node);
      list->retired_promoted_by_sweep_count += 1u;
      hz10_class_pages_prepend_active_with_eviction(list, node);
      list->retired_sweep_cursor = prev;
      return node;
    }
    /* Else: still fully non-idle, no capacity to offer -- leave it in
     * place for a future call to re-check. */

    node = prev;
    ++checked;
  }
  /* NULL here (walked off the head end) correctly means "start fresh
   * from the tail" on the next call, same as an empty cursor today. */
  list->retired_sweep_cursor = node;
  return NULL;
}
