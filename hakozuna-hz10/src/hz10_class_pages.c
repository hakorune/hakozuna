#include "hz10_class_pages.h"
#include "hz10_remote_stack.h"

void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page) {
  page->next_in_owner_list = list->head;
  list->head = page;
}

Hz10FreelistPage* hz10_class_pages_find_with_capacity(
    Hz10ClassPageList* list) {
  for (Hz10FreelistPage* page = list->head; page;
       page = page->next_in_owner_list) {
    if (page->local_free_head) {
      return page;
    }
    if (hz10_page_drain_remote(page) > 0u) {
      return page;
    }
  }
  return NULL;
}
