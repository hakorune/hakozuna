#include "hz10_public_entry.h"
#include "hz10_class_pages.h"
#include "hz10_large_alloc.h"
#include "hz10_pagemap.h"
#include "hz10_pooled_page.h"
#include "hz10_remote_stack.h"
#include "hz10_retired_ready.h"
#include "hz10_size_class.h"

#include <stdatomic.h>

typedef struct Hz10ClassState {
  Hz10FreelistPage* active;  /* fast-path cache; always also in `list` */
  Hz10ClassPageList list;    /* every page this thread ever made for the
                              * class -- see src/hz10_class_pages.h */
#if HZ10_ENABLE_TWO_SLOT_STATS
  /* HZ10TwoSlotActivePattern-L0: the page that was active immediately
   * before `active`, and how many allocs `active` has served since it
   * became the active page -- see the counters' doc comment in
   * src/hz10_class_pages.h. */
  Hz10FreelistPage* prior_active;
  uint64_t ops_since_activate;
#endif
} Hz10ClassState;

static _Thread_local Hz10ClassState hz10_class_state[HZ10_CLASS_COUNT];
static _Thread_local char hz10_thread_token_storage;
/* &hz10_thread_token_storage is a genuinely distinct address per thread
 * (each thread gets its own TLS instance) -- a standard, portable way to
 * get a per-thread identity token without inventing a thread-id concept. */
#define HZ10_THREAD_TOKEN ((void*)&hz10_thread_token_storage)

#if HZ10_ENABLE_TWO_SLOT_STATS
/* Called right before state->active is reassigned to `next_active`.
 * `from_find` distinguishes the find_with_capacity() switch site (the
 * only one the second-active hypothesis targets) from harvest/fresh,
 * which reach here only after find_with_capacity() already returned NULL
 * and so cannot be predicted by a single remembered pointer either way. */
static void hz10_class_state_note_switch(Hz10ClassState* state,
                                         Hz10FreelistPage* next_active,
                                         int from_find) {
  if (state->active) {
    state->list.active_switch_count += 1u;
    state->list.active_ops_served_sum += state->ops_since_activate;
    if (state->ops_since_activate <= 1u) {
      state->list.active_ops_served_immediate_count += 1u;
    }
  }
  if (from_find && state->prior_active) {
    state->list.second_active_check_count += 1u;
    if (next_active == state->prior_active) {
      state->list.second_active_hit_count += 1u;
    }
  }
  state->prior_active = state->active;
  state->ops_since_activate = 0u;
}
#else
static inline void hz10_class_state_note_switch(Hz10ClassState* state,
                                                Hz10FreelistPage* next_active,
                                                int from_find) {
  (void)state;
  (void)next_active;
  (void)from_find;
}
#endif

static void hz10_public_entry_reclaim_keep_page(Hz10PageSublist* survivors,
                                                Hz10FreelistPage* page) {
  page->prev_in_owner_list = survivors->tail;
  page->next_in_owner_list = NULL;
  if (survivors->tail) {
    survivors->tail->next_in_owner_list = page;
  } else {
    survivors->head = page;
  }
  survivors->tail = page;
  survivors->length += 1u;
}

static void hz10_public_entry_retired_remove(Hz10ClassPageList* list,
                                             Hz10FreelistPage* page) {
  if (list->retired_sweep_cursor == page) {
    list->retired_sweep_cursor = page->prev_in_owner_list;
  }
  if (page->prev_in_owner_list) {
    page->prev_in_owner_list->next_in_owner_list = page->next_in_owner_list;
  } else {
    list->retired.head = page->next_in_owner_list;
  }
  if (page->next_in_owner_list) {
    page->next_in_owner_list->prev_in_owner_list = page->prev_in_owner_list;
  } else {
    list->retired.tail = page->prev_in_owner_list;
  }
  list->retired.length -= 1u;
}

static void hz10_public_entry_reclaim_ready_idle_pages(
    Hz10ClassPageList* list, Hz10PublicEntryThreadReclaimStats* stats) {
  Hz10FreelistPage* page;
  while ((page = hz10_retired_ready_pop(&list->ready)) != NULL) {
    if (page->generation != page->retired_ready_generation) {
      list->ready_stale_generation_count += 1u;
      continue;
    }
    stats->pages_seen += 1u;
    stats->slots_merged += hz10_page_drain_remote(page);
    if (page->free_count == page->slot_count) {
      hz10_public_entry_retired_remove(list, page);
      list->retired_reclaimed_by_ready_count += 1u;
      stats->pages_reclaimed += 1u;
      hz10_pooled_page_destroy(page);
    } else {
      list->ready_false_positive_count += 1u;
      stats->pages_busy += 1u;
    }
  }
}

static void hz10_public_entry_reclaim_sublist_idle_pages(
    Hz10PageSublist* sub, Hz10PublicEntryThreadReclaimStats* stats,
    int is_retired) {
  Hz10PageSublist survivors = {0};
  for (Hz10FreelistPage* page = sub->head; page;) {
    Hz10FreelistPage* next = page->next_in_owner_list;
    page->next_in_owner_list = NULL;
    page->prev_in_owner_list = NULL;
    stats->pages_seen += 1u;
    stats->slots_merged += hz10_page_drain_remote(page);

    if (page->free_count != page->slot_count) {
      stats->pages_busy += 1u;
      hz10_public_entry_reclaim_keep_page(&survivors, page);
      page = next;
      continue;
    }

    if (is_retired &&
        atomic_load_explicit(&page->retired_ready_on_stack,
                             memory_order_acquire)) {
      stats->pages_deferred_ready += 1u;
      hz10_public_entry_reclaim_keep_page(&survivors, page);
      page = next;
      continue;
    }

    if (is_retired && !hz10_retired_ready_cancel(page)) {
      stats->pages_deferred_cancel += 1u;
      hz10_public_entry_reclaim_keep_page(&survivors, page);
      page = next;
      continue;
    }

    stats->pages_reclaimed += 1u;
    hz10_pooled_page_destroy(page);
    page = next;
  }
  *sub = survivors;
}

void hz10_public_entry_reclaim_thread_idle_pages(
    Hz10PublicEntryThreadReclaimStats* stats_out) {
  Hz10PublicEntryThreadReclaimStats stats = {0};
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassState* state = &hz10_class_state[c];
    hz10_public_entry_reclaim_ready_idle_pages(&state->list, &stats);
    hz10_public_entry_reclaim_sublist_idle_pages(&state->list.active, &stats,
                                                 0);
    hz10_public_entry_reclaim_sublist_idle_pages(&state->list.retired, &stats,
                                                 1);
    state->list.retired_sweep_cursor = NULL;
    state->active = state->list.active.head;
#if HZ10_ENABLE_TWO_SLOT_STATS
    state->prior_active = NULL;
    state->ops_since_activate = 0u;
#endif
  }
  if (stats_out) {
    *stats_out = stats;
  }
}

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
      hz10_class_pages_note_active_cache_hit(&state->list);
#if HZ10_ENABLE_TWO_SLOT_STATS
      state->ops_since_activate += 1u;
#endif
      return ptr;
    }
    hz10_class_pages_note_active_cache_alloc_fail(&state->list);
    uint32_t merged = hz10_page_drain_remote(state->active);
    if (merged > 0u) {
      ptr = hz10_freelist_page_alloc(state->active);
      hz10_class_pages_note_active_cache_drain(&state->list, merged,
                                               ptr != NULL);
      if (ptr) {
#if HZ10_ENABLE_TWO_SLOT_STATS
        state->ops_since_activate += 1u;
#endif
        return ptr;
      }
    } else {
      hz10_class_pages_note_active_cache_drain(&state->list, merged, 0);
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
    hz10_class_state_note_switch(state, found, 1);
    state->active = found;
#if HZ10_ENABLE_TWO_SLOT_STATS
    state->ops_since_activate += 1u;
#endif
    return hz10_freelist_page_alloc(found);
  }

  /* Genuine miss: about to pay for a fresh page below, so this is exactly
   * the moment a retired-list harvest pays off most -- a page found idle
   * right here is destroyed and returned to Box 4's pool, and one found
   * with only partial capacity is promoted straight back into `active`
   * and handed back below, skipping the fresh-page cost entirely. Slow
   * path only, bounded cost (HZ10_CLASS_PAGES_SWEEP_BUDGET), never on the
   * hot per-op alloc path above -- see src/hz10_class_pages.h. */
  Hz10FreelistPage* harvested =
      hz10_class_pages_harvest_retired_capacity(&state->list);
  if (harvested) {
    hz10_class_state_note_switch(state, harvested, 0);
    state->active = harvested;
#if HZ10_ENABLE_TWO_SLOT_STATS
    state->ops_since_activate += 1u;
#endif
    return hz10_freelist_page_alloc(harvested);
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
  hz10_class_state_note_switch(state, fresh, 0);
  state->active = fresh;
#if HZ10_ENABLE_TWO_SLOT_STATS
  state->ops_since_activate += 1u;
#endif
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
  /* claim() then publish(), not a single hz10_page_remote_free() call,
   * with hz10_retired_ready_note_remote_free() run in between -- see
   * src/hz10_remote_stack.h's module comment for why this gap has to
   * exist: note_remote_free() (cheap, a no-op unless `page` is currently
   * tracked by HZ10RetiredReadyQueue-L0) needs a point to run BEFORE this
   * slot becomes visible to the owner's drain, or its own reads/writes
   * to `page` can race a destroy the owner performs the instant it
   * observes this slot merged. */
  uint32_t slot_index;
  if (!hz10_page_remote_free_claim(page, ptr, route.generation,
                                  &slot_index)) {
    return 0;
  }
  hz10_retired_ready_note_remote_free(page);
  hz10_page_remote_free_publish(page, ptr);
  return 1;
}

void hz10_public_entry_class_list_stats(uint32_t class_id,
                                        Hz10ClassPageListStats* stats_out) {
  if (!stats_out) {
    return;
  }
  if (class_id >= HZ10_CLASS_COUNT) {
    *stats_out = (Hz10ClassPageListStats){0};
    return;
  }
  const Hz10ClassPageList* list = &hz10_class_state[class_id].list;
  stats_out->active_length = list->active.length;
  stats_out->retired_length = list->retired.length;
  stats_out->max_retired_length = list->max_retired_length;
  stats_out->eviction_count = list->eviction_count;
  stats_out->eviction_reclaimed_count = list->eviction_reclaimed_count;
  stats_out->retired_count = list->retired_count;
  stats_out->retired_reclaimed_by_sweep_count =
      list->retired_reclaimed_by_sweep_count;
  stats_out->retired_promoted_by_sweep_count =
      list->retired_promoted_by_sweep_count;
  stats_out->harvest_call_count = list->harvest_call_count;
  stats_out->retired_reclaimed_by_ready_count =
      list->retired_reclaimed_by_ready_count;
  stats_out->retired_promoted_by_ready_count =
      list->retired_promoted_by_ready_count;
  stats_out->ready_false_positive_count = list->ready_false_positive_count;
  stats_out->ready_push_count = atomic_load_explicit(
      &list->ready.push_count, memory_order_relaxed);
  stats_out->ready_stale_generation_count =
      list->ready_stale_generation_count;
  stats_out->sweep_cancel_lost_race_count =
      list->sweep_cancel_lost_race_count;
  stats_out->active_cache_hit_count = list->active_cache_hit_count;
  stats_out->active_cache_alloc_fail_count =
      list->active_cache_alloc_fail_count;
  stats_out->active_cache_drain_call_count =
      list->active_cache_drain_call_count;
  stats_out->active_cache_drain_fail_count =
      list->active_cache_drain_fail_count;
  stats_out->active_cache_nonempty_drain_count =
      list->active_cache_nonempty_drain_count;
  stats_out->active_cache_slots_merged_count =
      list->active_cache_slots_merged_count;
  stats_out->active_cache_drain_hit_count =
      list->active_cache_drain_hit_count;
  stats_out->find_call_count = list->find_call_count;
  stats_out->find_miss_count = list->find_miss_count;
  stats_out->find_pages_visited_count = list->find_pages_visited_count;
  stats_out->find_drain_call_count = list->find_drain_call_count;
  stats_out->find_nonempty_drain_count = list->find_nonempty_drain_count;
  stats_out->find_slots_merged_count = list->find_slots_merged_count;
  stats_out->find_local_hit_count = list->find_local_hit_count;
  stats_out->find_drain_hit_count = list->find_drain_hit_count;
  stats_out->find_hit_depth_sum = list->find_hit_depth_sum;
  stats_out->find_miss_pages_visited_count =
      list->find_miss_pages_visited_count;
  stats_out->find_hit_depth_max = list->find_hit_depth_max;
  for (uint32_t i = 0; i < HZ10_CLASS_PAGES_SCAN_DEPTH_HIST_BUCKETS; ++i) {
    stats_out->find_depth_hist[i] = list->find_depth_hist[i];
  }
  stats_out->active_switch_count = list->active_switch_count;
  stats_out->active_ops_served_sum = list->active_ops_served_sum;
  stats_out->active_ops_served_immediate_count =
      list->active_ops_served_immediate_count;
  stats_out->second_active_check_count = list->second_active_check_count;
  stats_out->second_active_hit_count = list->second_active_hit_count;
}
