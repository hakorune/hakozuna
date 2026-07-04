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

void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page) {
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
   * churn). */
  hz10_page_sublist_prepend(&list->retired, evict);
  list->retired_count += 1u;
  if (list->retired.length > list->max_retired_length) {
    list->max_retired_length = list->retired.length;
  }

  if (list->retired.length <= HZ10_CLASS_PAGES_RETIRED_LIMIT) {
    return;
  }

  /* `retired` is itself bounded: its own tail gets one final drain+check
   * before this design truly gives up on it (retired.length > LIMIT >= 1
   * guarantees pop_tail's precondition holds here too). */
  Hz10FreelistPage* give_up = hz10_page_sublist_pop_tail(&list->retired);
  hz10_page_drain_remote(give_up);
  if (give_up->free_count == give_up->slot_count) {
    list->retired_reclaimed_by_overflow_count += 1u;
    hz10_pooled_page_destroy(give_up);
  } else {
    list->retired_dropped_count += 1u;
    /* Same fate an evicted page always had before this design existed:
     * still safely freeable via Box 1's pagemap route either way, list
     * membership was never load-bearing for hz10_free's correctness. */
  }
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

void hz10_class_pages_sweep_retired(Hz10ClassPageList* list) {
  uint32_t checked = 0u;
  Hz10FreelistPage* node = list->retired.tail;
  while (node && checked < HZ10_CLASS_PAGES_SWEEP_BUDGET) {
    /* Save prev before a possible reclaim unlinks `node`. */
    Hz10FreelistPage* prev = node->prev_in_owner_list;
    hz10_page_drain_remote(node);
    if (node->free_count == node->slot_count) {
      hz10_page_sublist_remove(&list->retired, node);
      list->retired_reclaimed_by_sweep_count += 1u;
      hz10_pooled_page_destroy(node);
    }
    node = prev;
    ++checked;
  }
}
