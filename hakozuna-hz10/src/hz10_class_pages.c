#include "hz10_class_pages.h"
#include "hz10_pooled_page.h"
#include "hz10_remote_stack.h"

void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page) {
  page->prev_in_owner_list = NULL;
  page->next_in_owner_list = list->head;
  if (list->head) {
    list->head->prev_in_owner_list = page;
  } else {
    list->tail = page;
  }
  list->head = page;
  list->length += 1u;

  if (list->length <= HZ10_CLASS_PAGES_SCAN_LIMIT) {
    return;
  }

  /* Evict the tail (oldest, least-recently-created page for this class).
   * `page` was just prepended at head, so tail is always a different node
   * here (length was already >= 1 before this call). */
  Hz10FreelistPage* evict = list->tail;
  list->tail = evict->prev_in_owner_list;
  if (list->tail) {
    list->tail->next_in_owner_list = NULL;
  } else {
    list->head = NULL; /* unreachable: length > SCAN_LIMIT >= 1 */
  }
  list->length -= 1u;

  /* One last chance to notice a pending remote free before giving up on
   * this page -- same drain hz10_class_pages_find_with_capacity would
   * have done had it still been within the scan window. */
  hz10_page_drain_remote(evict);
  if (evict->free_count == evict->slot_count) {
    hz10_pooled_page_destroy(evict);
  }
  /* Otherwise: evict still has outstanding slots. Just drop it from this
   * list -- correctness-neutral (see module comment), the exact same fate
   * an out-of-window page already had before this function existed. */
}

Hz10FreelistPage* hz10_class_pages_find_with_capacity(
    Hz10ClassPageList* list) {
  uint32_t scanned = 0u;
  for (Hz10FreelistPage* page = list->head;
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
