#include "hz10_class_pages.h"
#include "hz10_pooled_page.h"
#include "hz10_remote_stack.h"
#include "hz10_retired_ready.h"

#if HZ10_ENABLE_ACTIVE_SCAN_STATS
static uint32_t hz10_class_pages_scan_depth_bucket(uint32_t depth) {
  if (depth == 0u) {
    return 0u;
  }
  if (depth == 1u) {
    return 1u;
  }
  if (depth <= 4u) {
    return 2u;
  }
  if (depth <= 16u) {
    return 3u;
  }
  if (depth <= 64u) {
    return 4u;
  }
  return 5u;
}

void hz10_class_pages_note_active_cache_hit(Hz10ClassPageList* list) {
  list->active_cache_hit_count += 1u;
}

void hz10_class_pages_note_active_cache_alloc_fail(Hz10ClassPageList* list) {
  list->active_cache_alloc_fail_count += 1u;
}

void hz10_class_pages_note_active_cache_drain(Hz10ClassPageList* list,
                                             uint32_t slots_merged,
                                             int produced_capacity) {
  list->active_cache_drain_call_count += 1u;
  list->active_cache_slots_merged_count += slots_merged;
  if (slots_merged > 0u) {
    list->active_cache_nonempty_drain_count += 1u;
  }
  if (produced_capacity) {
    list->active_cache_drain_hit_count += 1u;
  } else {
    list->active_cache_drain_fail_count += 1u;
  }
}
#endif

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

static void hz10_class_pages_prepend_active(Hz10ClassPageList* list,
                                            Hz10FreelistPage* page) {
  page->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_ACTIVE;
  hz10_page_sublist_prepend(&list->active, page);
}

static void hz10_class_pages_prepend_retired(Hz10ClassPageList* list,
                                             Hz10FreelistPage* page) {
  page->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_RETIRED;
  hz10_page_sublist_prepend(&list->retired, page);
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
  node->next_in_owner_list = NULL;
  node->prev_in_owner_list = NULL;
  node->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_NONE;
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
  node->next_in_owner_list = NULL;
  node->prev_in_owner_list = NULL;
  node->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_NONE;
}

#if HZ10_ENABLE_ACTIVE_MTF
static void hz10_page_sublist_move_to_head(Hz10PageSublist* sub,
                                          Hz10FreelistPage* node) {
  if (sub->head == node) {
    return;
  }
  hz10_page_sublist_remove(sub, node);
  hz10_page_sublist_prepend(sub, node);
  node->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_ACTIVE;
}
#endif

/* Same as hz10_page_sublist_remove(&list->retired, node), but also
 * redirects list->retired_sweep_cursor first if it currently points at
 * `node` -- a persistent cross-call cursor left dangling by a removal it
 * does not know about is a real use-after-free (found via ASan under
 * bench/hz10_public_entry_steady_state_bench.c's long-running,
 * continuously-checking-in workload): HZ10RetiredReadyQueue-L0's own
 * removals (below) happen independently of the budgeted walk's own
 * prev-tracking, so any removal from `retired` from ANYWHERE must go
 * through this, not the plain sublist helper directly. */
static void hz10_class_pages_retired_remove(Hz10ClassPageList* list,
                                           Hz10FreelistPage* node) {
  if (list->retired_sweep_cursor == node) {
    list->retired_sweep_cursor = node->prev_in_owner_list;
  }
  hz10_page_sublist_remove(&list->retired, node);
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
  hz10_class_pages_prepend_active(list, page);

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
  hz10_class_pages_prepend_retired(list, evict);
  list->retired_count += 1u;
  if (list->retired.length > list->max_retired_length) {
    list->max_retired_length = list->retired.length;
  }
  /* Also register for O(1) event-driven discovery (see the module
   * comment on HZ10RetiredReadyQueue-L0) -- outstanding is exactly the
   * slots this drain_remote() call above did not just account for. */
  hz10_retired_ready_mark(evict, &list->ready,
                         evict->slot_count - evict->free_count);
}

void hz10_class_pages_add(Hz10ClassPageList* list, Hz10FreelistPage* page) {
  hz10_class_pages_prepend_active_with_eviction(list, page);
}

Hz10FreelistPage* hz10_class_pages_find_with_capacity(
    Hz10ClassPageList* list) {
#if HZ10_ENABLE_ACTIVE_SCAN_STATS
  list->find_call_count += 1u;
#endif
  uint32_t scanned = 0u;
  for (Hz10FreelistPage* page = list->active.head;
       page && scanned < HZ10_CLASS_PAGES_SCAN_LIMIT;
       page = page->next_in_owner_list, ++scanned) {
    if (page->local_free_head) {
#if HZ10_ENABLE_ACTIVE_SCAN_STATS
      uint32_t depth = scanned + 1u;
      list->find_pages_visited_count += depth;
      list->find_local_hit_count += 1u;
      list->find_hit_depth_sum += depth;
      if (depth > list->find_hit_depth_max) {
        list->find_hit_depth_max = depth;
      }
      list->find_depth_hist[hz10_class_pages_scan_depth_bucket(depth)] += 1u;
#endif
#if HZ10_ENABLE_ACTIVE_MTF
      hz10_page_sublist_move_to_head(&list->active, page);
#endif
      return page;
    }
    uint32_t merged = hz10_page_drain_remote(page);
#if HZ10_ENABLE_ACTIVE_SCAN_STATS
    list->find_drain_call_count += 1u;
    list->find_slots_merged_count += merged;
    if (merged > 0u) {
      list->find_nonempty_drain_count += 1u;
    }
#endif
    if (merged > 0u) {
#if HZ10_ENABLE_ACTIVE_SCAN_STATS
      uint32_t depth = scanned + 1u;
      list->find_pages_visited_count += depth;
      list->find_drain_hit_count += 1u;
      list->find_hit_depth_sum += depth;
      if (depth > list->find_hit_depth_max) {
        list->find_hit_depth_max = depth;
      }
      list->find_depth_hist[hz10_class_pages_scan_depth_bucket(depth)] += 1u;
#endif
#if HZ10_ENABLE_ACTIVE_MTF
      hz10_page_sublist_move_to_head(&list->active, page);
#endif
      return page;
    }
  }
#if HZ10_ENABLE_ACTIVE_SCAN_STATS
  list->find_miss_count += 1u;
  list->find_pages_visited_count += scanned;
  list->find_miss_pages_visited_count += scanned;
  list->find_depth_hist[hz10_class_pages_scan_depth_bucket(scanned)] += 1u;
#endif
  return NULL;
}

Hz10FreelistPage* hz10_class_pages_harvest_retired_capacity(
    Hz10ClassPageList* list) {
  list->harvest_call_count += 1u;

  /* O(1) event-driven fast path first -- see the module comment on
   * HZ10RetiredReadyQueue-L0. Every candidate is a hint, re-verified
   * exactly like the budgeted walk below verifies its own candidates. */
  Hz10FreelistPage* candidate;
  while ((candidate = hz10_retired_ready_pop(&list->ready)) != NULL) {
    /* Generation guard FIRST, before touching any other field -- see
     * hz10_retired_ready.h's bug (2) and src/hz10_freelist_page.h's
     * retired_ready_generation comment. If this address was destroyed
     * and recycled for an unrelated page between being pushed and being
     * popped, `candidate` is no longer the page we marked -- every field
     * below (free_count, local_free_head, the retired-list prev/next
     * links) would belong to that unrelated page instead, so this
     * reference must simply be dropped here, not processed. */
    if (candidate->generation != candidate->retired_ready_generation) {
      list->ready_stale_generation_count += 1u;
      continue;
    }
    hz10_page_drain_remote(candidate);
    if (candidate->free_count == candidate->slot_count) {
      hz10_class_pages_retired_remove(list, candidate);
      list->retired_reclaimed_by_ready_count += 1u;
      hz10_pooled_page_destroy(candidate);
      /* Keep draining -- more candidates may already be waiting, and
       * each is O(1), no scan cost. */
    } else if (candidate->local_free_head) {
      hz10_class_pages_retired_remove(list, candidate);
      list->retired_promoted_by_ready_count += 1u;
      hz10_class_pages_prepend_active_with_eviction(list, candidate);
      return candidate;
    } else {
      /* Race (see hz10_retired_ready.h): leave it exactly where it
       * already is in `retired` for the budgeted walk below to find. */
      list->ready_false_positive_count += 1u;
    }
  }

  /* Resume from the cursor left by the last call (or `retired`'s tail if
   * there wasn't one, or the last call walked off the head end) -- see
   * the module comment for why restarting from the tail every call
   * measured as a head-of-line-blocking dead end. */
  Hz10FreelistPage* node = list->retired_sweep_cursor;
  if (!node) {
    node = list->retired.tail;
  }

  uint32_t checked = 0u;
  while (node && checked < HZ10_CLASS_PAGES_SWEEP_BUDGET) {
    /* Save prev (toward head) before this node possibly gets unlinked. */
    Hz10FreelistPage* prev = node->prev_in_owner_list;

    /* Skip a node currently linked into `ready` (see the struct comment
     * on retired_ready_on_stack): destroying or promoting it here, while
     * hz10_retired_ready_pop() might still reach it via retired_ready_
     * next, is a real use-after-free -- leave it for the ready-queue
     * path (drained at the top of this function) to finish instead. */
    if (atomic_load_explicit(&node->retired_ready_on_stack,
                             memory_order_acquire)) {
      node = prev;
      ++checked;
      continue;
    }

    hz10_page_drain_remote(node);

    if (node->free_count == node->slot_count) {
      /* Fully idle: nothing left to reuse, just reclaim it -- but only
       * once hz10_retired_ready_cancel() confirms no concurrent
       * note_remote_free() call is also about to act on this page (see
       * hz10_retired_ready.h's module comment, bug (3)). Losing this
       * race just means it is about to be (or already is) linked onto
       * `ready` instead -- leave it exactly where it is for that path
       * to finish, same tolerance as the retired_ready_on_stack skip
       * above. */
      if (!hz10_retired_ready_cancel(node)) {
        list->sweep_cancel_lost_race_count += 1u;
        node = prev;
        ++checked;
        continue;
      }
      hz10_class_pages_retired_remove(list, node);
      list->retired_reclaimed_by_sweep_count += 1u;
      hz10_pooled_page_destroy(node);
    } else if (node->local_free_head) {
      /* Same cancel()-first requirement as the destroy branch above --
       * see hz10_retired_ready.h's bug (3): promoting this page into
       * `active` while a note_remote_free() call still believes it owns
       * this page's ready-tracking is exactly the bug that surfaced as
       * ready-loop candidates no longer found in `retired`. */
      if (!hz10_retired_ready_cancel(node)) {
        list->sweep_cancel_lost_race_count += 1u;
        node = prev;
        ++checked;
        continue;
      }
      /* Partial capacity: don't wait for the rest to come back too --
       * promote it into `active` right now and hand it to the caller,
       * who is about to pay for a fresh page otherwise (see the module
       * comment on RetiredPartialReuse-L1). */
      hz10_class_pages_retired_remove(list, node);
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

#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
int hz10_class_pages_reclaim_retired_idle_after_local_free(
    Hz10ClassPageList* list, Hz10FreelistPage* page) {
  if (!list || !page || page->free_count != page->slot_count) {
    return 0;
  }
  if (page->slot_size < HZ10_RETIRED_LOCAL_IDLE_MIN_SLOT_SIZE ||
      page->slot_size > HZ10_RETIRED_LOCAL_IDLE_MAX_SLOT_SIZE ||
      page->owner_list_kind != HZ10_FREELIST_PAGE_OWNER_LIST_RETIRED ||
      !atomic_load_explicit(&page->retired_ready_flag, memory_order_acquire)) {
    return 0;
  }

  Hz10FreelistPage* found = NULL;
  for (Hz10FreelistPage* node = list->retired.head; node;
       node = node->next_in_owner_list) {
    if (node == page) {
      found = node;
      break;
    }
  }
  if (!found) {
    return 0;
  }

  if (atomic_load_explicit(&page->retired_ready_on_stack,
                           memory_order_acquire)) {
    return 0;
  }
  if (!hz10_retired_ready_cancel(page)) {
    list->sweep_cancel_lost_race_count += 1u;
    return 0;
  }

  hz10_class_pages_retired_remove(list, page);
  list->retired_reclaimed_by_local_free_count += 1u;
  hz10_pooled_page_destroy(page);
  return 1;
}
#endif
