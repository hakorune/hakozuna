#include "hz10_public_entry.h"
#include "hz10_class_pages.h"
#include "hz10_large_alloc.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"
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

typedef struct Hz10ThreadOwner { Hz10ClassState classes[HZ10_CLASS_COUNT]; }
    Hz10ThreadOwner;

static _Thread_local Hz10ThreadOwner* hz10_current_owner;

static Hz10ThreadOwner* hz10_public_entry_current_owner(void) {
  Hz10ThreadOwner* owner = hz10_current_owner;
  if (owner) return owner;
  owner = (Hz10ThreadOwner*)hz10_platform_reserve_rw(sizeof(*owner));
  if (!owner) return NULL;
  hz10_current_owner = owner;
  return owner;
}

static inline Hz10ThreadOwner* hz10_public_entry_current_owner_if_any(void) {
  return hz10_current_owner;
}

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

#if HZ10_ENABLE_FRONT_CACHE
/* Defined with the rest of HZ10FrontCache-L1 below hz10_malloc()'s page
 * layer; declared here because the lifecycle flush above it runs this as
 * its phase 0. */
static void hz10_front_cache_flush_all(void);
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

static inline void hz10_public_entry_note_owner_local_free(
    uint32_t class_id, Hz10FreelistPage* page) {
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
  if (class_id >= HZ10_RETIRED_LOCAL_IDLE_MIN_CLASS_ID &&
      class_id <= HZ10_RETIRED_LOCAL_IDLE_MAX_CLASS_ID && page &&
      page->slot_size >= HZ10_RETIRED_LOCAL_IDLE_MIN_SLOT_SIZE &&
      page->slot_size <= HZ10_RETIRED_LOCAL_IDLE_MAX_SLOT_SIZE &&
      page->free_count == page->slot_count &&
      page->owner_list_kind == HZ10_FREELIST_PAGE_OWNER_LIST_RETIRED &&
      class_id < HZ10_CLASS_COUNT) {
    Hz10ThreadOwner* owner = hz10_public_entry_current_owner_if_any();
    if (!owner) {
      return;
    }
    (void)hz10_class_pages_reclaim_retired_idle_after_local_free(
        &owner->classes[class_id].list, page);
  }
#else
  (void)class_id;
  (void)page;
#endif
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

/* See src/hz10_public_entry.h's HZ10LifecycleFlushContract-L1 doc comment
 * for the precondition/postcondition contract and the load-bearing
 * ready/cancel guard this function and its helpers above must preserve. */
void hz10_public_entry_flush_thread_cache_quiescent(
    Hz10PublicEntryThreadReclaimStats* stats_out) {
  Hz10PublicEntryThreadReclaimStats stats = {0};
#if HZ10_ENABLE_FRONT_CACHE
  /* Phase 0 (HZ10FrontCache-L1): return every front-cached object to its
   * page first, so the idle checks below see true free_count values --
   * see the POSTCONDITION note in hz10_public_entry.h. */
  hz10_front_cache_flush_all();
#endif
  Hz10ThreadOwner* owner = hz10_public_entry_current_owner_if_any();
  if (!owner) {
    if (stats_out) {
      *stats_out = stats;
    }
    return;
  }
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassState* state = &owner->classes[c];
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

/*
 * The page-layer alloc body, exactly what hz10_malloc() ran directly
 * before HZ10FrontCache-L1: active-page pop -> active drain ->
 * find_with_capacity -> harvest_retired -> fresh pooled page. Split out
 * (per docs/HZ10_FRONT_CACHE_DESIGN_L0.md) so the front cache's refill
 * can call the page layer without recursing into the public front-cache
 * entry point; with the flag off, hz10_malloc() is a plain tail call
 * into this and behavior is unchanged.
 */
static void* hz10_public_entry_alloc_from_page_layer(uint32_t class_id) {
  Hz10ThreadOwner* owner = hz10_public_entry_current_owner();
  if (!owner) {
    return NULL;
  }
  Hz10ClassState* state = &owner->classes[class_id];

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
  hz10_freelist_page_set_owner_thread(fresh, owner);
  hz10_freelist_page_set_class_id(fresh, class_id);
  hz10_class_pages_add(&state->list, fresh);
  hz10_class_state_note_switch(state, fresh, 0);
  state->active = fresh;
#if HZ10_ENABLE_TWO_SLOT_STATS
  state->ops_since_activate += 1u;
#endif
  return hz10_freelist_page_alloc(fresh); /* fresh page: never NULL */
}

#if HZ10_ENABLE_FRONT_CACHE

/* HZ10FrontCache-L1: a per-thread, per-class object freelist in front of
 * the page layer. A cached slot still counts as allocated from its page's
 * point of view; only overflow/lifecycle flush returns it through
 * hz10_freelist_page_free(). Keep the in-slot local-free marker in the
 * same states as the page freelist so remote duplicate-free rejection
 * stays unchanged. See docs/HZ10_FRONT_CACHE_DESIGN_L0.md for the capacity
 * rationale and A/B record. */
/* Classes below this id bypass the front cache entirely (page-centric
 * path, exactly the flag-off behavior). Measured NO-GO
 * (bench_results/20260705T041052Z_hz10_front_cache_l1): setting this to
 * 17 to bypass small classes made the mixed rows worse, likely from the
 * extra data-dependent branch. 0 is the default: no threshold, all
 * classes front-cached when HZ10_ENABLE_FRONT_CACHE is on. Keep the knob
 * only for future measurement. */
#ifndef HZ10_FRONT_CACHE_MIN_CLASS
#define HZ10_FRONT_CACHE_MIN_CLASS 0u
#endif

#ifndef HZ10_FRONT_CACHE_CLASS_BYTES
#define HZ10_FRONT_CACHE_CLASS_BYTES (2u * 1024u * 1024u)
#endif
#ifndef HZ10_FRONT_CACHE_MIN_OBJECTS
#define HZ10_FRONT_CACHE_MIN_OBJECTS 8u
#endif
#ifndef HZ10_FRONT_CACHE_MAX_OBJECTS
#define HZ10_FRONT_CACHE_MAX_OBJECTS 4096u
#endif
/* Refill batch is capped at one quantum's worth of slots (and 32 objects)
 * so a single cold malloc can never force the page layer to create more
 * than about one fresh page beyond what today's path would -- for the
 * 1-slot classes this makes refill_batch exactly 1, i.e. a cold miss
 * costs the same as today and batching only ever happens where it is
 * free. */
#ifndef HZ10_FRONT_CACHE_REFILL_BYTES
#define HZ10_FRONT_CACHE_REFILL_BYTES ((uint32_t)HZ10_PAGE_QUANTUM)
#endif
#ifndef HZ10_FRONT_CACHE_MAX_REFILL_OBJECTS
#define HZ10_FRONT_CACHE_MAX_REFILL_OBJECTS 32u
#endif
#ifndef HZ10_ENABLE_FRONT_CACHE_ARRAY
#define HZ10_ENABLE_FRONT_CACHE_ARRAY 0
#endif

typedef struct Hz10FrontCache {
#if HZ10_ENABLE_FRONT_CACHE_ARRAY
  /* HZ10FrontCacheArray-L1: dense LIFO storage. The +1 preserves the
   * existing push-then-overflow boundary when cap == MAX_OBJECTS. */
  void* slots[HZ10_FRONT_CACHE_MAX_OBJECTS + 1u];
#else
  void* head;            /* intrusive singly-linked object freelist */
#endif
  uint32_t count;
  uint32_t cap;          /* 0 == this class not initialized yet */
  uint32_t refill_batch;
  uint32_t slot_size;    /* cached here so the alloc fast path needs no
                          * cross-TU hz10_size_class_slot_size() call */
#if HZ10_ENABLE_FRONT_CACHE_STATS
  uint64_t hit_count;
  uint64_t refill_count;
  uint64_t refill_objects;
  uint64_t flush_count;
  uint64_t flush_objects;
#endif
} Hz10FrontCache;

static _Thread_local Hz10FrontCache hz10_front_cache[HZ10_CLASS_COUNT];

/* Marker helpers by slot_size, for call sites that have no page pointer
 * in hand (refill push / front pop). Same layout rule as
 * hz10_freelist_page_slot_marker_ptr(): second word, only if the slot is
 * big enough to have one. */
static inline void hz10_front_cache_set_marker(uint32_t slot_size,
                                              void* ptr) {
  if (slot_size >= (uint32_t)(2u * sizeof(void*))) {
    *(uintptr_t*)((char*)ptr + sizeof(void*)) = HZ10_FREELIST_LOCAL_MARKER;
  }
}

static inline void hz10_front_cache_clear_marker(uint32_t slot_size,
                                                void* ptr) {
  if (slot_size >= (uint32_t)(2u * sizeof(void*))) {
    *(uintptr_t*)((char*)ptr + sizeof(void*)) = 0u;
  }
}

static void hz10_front_cache_init(Hz10FrontCache* fc, uint32_t class_id) {
  uint32_t slot_size = hz10_size_class_slot_size(class_id);
  uint32_t cap = HZ10_FRONT_CACHE_CLASS_BYTES / slot_size;
  if (cap < HZ10_FRONT_CACHE_MIN_OBJECTS) {
    cap = HZ10_FRONT_CACHE_MIN_OBJECTS;
  }
  if (cap > HZ10_FRONT_CACHE_MAX_OBJECTS) {
    cap = HZ10_FRONT_CACHE_MAX_OBJECTS;
  }
  uint32_t batch = HZ10_FRONT_CACHE_REFILL_BYTES / slot_size;
  if (batch < 1u) {
    batch = 1u;
  }
  if (batch > HZ10_FRONT_CACHE_MAX_REFILL_OBJECTS) {
    batch = HZ10_FRONT_CACHE_MAX_REFILL_OBJECTS;
  }
  if (batch > cap) {
    batch = cap;
  }
  fc->slot_size = slot_size;
  fc->refill_batch = batch;
  fc->cap = cap; /* last: cap != 0 is the "initialized" signal */
}

/* Returns down to target_count objects to their owning pages. Every
 * object in a front cache belongs to a page this thread owns (only the
 * owner-token fast path ever pushes here), so the page free below is
 * plain owner-thread local-free work; the route re-lookup is the
 * boundary cost the design doc accepts (~2.4ns/object, flush-only). */
static void hz10_front_cache_flush_to_count(Hz10FrontCache* fc,
                                           uint32_t target_count) {
  while (fc->count > target_count) {
#if HZ10_ENABLE_FRONT_CACHE_ARRAY
    fc->count -= 1u;
    void* slot = fc->slots[fc->count];
#else
    void* slot = fc->head;
    fc->head = *(void**)slot;
    fc->count -= 1u;
#endif
    H10RouteResult route = hz10_pagemap_route(slot, HZ10_GENERATION_ANY);
    if (route.kind != H10_ROUTE_VALID || !route.owner) {
      /* Unreachable by the accounting rule (front-cached slots keep
       * their page's free_count below slot_count, so the page cannot
       * pass idle verification and be destroyed while this cache holds
       * its slots). Fail closed -- drop the reference rather than write
       * through a pointer the pagemap no longer vouches for. */
      continue;
    }
    Hz10FreelistPage* page = (Hz10FreelistPage*)route.owner;
    hz10_freelist_page_free(page, slot);
    hz10_public_entry_note_owner_local_free(page->class_id, page);
#if HZ10_ENABLE_FRONT_CACHE_STATS
    fc->flush_objects += 1u;
#endif
  }
}

static void* hz10_front_cache_refill_and_alloc(uint32_t class_id,
                                              Hz10FrontCache* fc) {
  if (fc->cap == 0u) {
    hz10_front_cache_init(fc, class_id);
  }
  /* First object goes straight to the caller -- page alloc already
   * cleared its marker, no front push/pop round trip. */
  void* first = hz10_public_entry_alloc_from_page_layer(class_id);
  if (!first) {
    return NULL;
  }
#if HZ10_ENABLE_FRONT_CACHE_STATS
  fc->refill_count += 1u;
  fc->refill_objects += 1u;
#endif
  for (uint32_t i = 1u; i < fc->refill_batch; ++i) {
    void* extra = hz10_public_entry_alloc_from_page_layer(class_id);
    if (!extra) {
      break;
    }
    hz10_front_cache_set_marker(fc->slot_size, extra);
#if HZ10_ENABLE_FRONT_CACHE_ARRAY
    fc->slots[fc->count] = extra;
#else
    *(void**)extra = fc->head;
    fc->head = extra;
#endif
    fc->count += 1u;
#if HZ10_ENABLE_FRONT_CACHE_STATS
    fc->refill_objects += 1u;
#endif
  }
  return first;
}

/* The free fast path's single "count > cap" test doubles as the lazy-init
 * check: an uninitialized class has cap == 0, so its very first push
 * lands here (count 1 > cap 0), gets the class initialized, and -- since
 * count is then far below the real cap -- skips the flush. This keeps
 * the per-free branch count at exactly one instead of a separate
 * cap==0 test per push (measured: the extra branch was visible on the
 * 16/64-byte rows). */
static void hz10_front_cache_overflow(Hz10FrontCache* fc,
                                     uint32_t class_id) {
  if (fc->cap == 0u) {
    hz10_front_cache_init(fc, class_id);
    if (fc->count <= fc->cap) {
      return;
    }
  }
  hz10_front_cache_flush_to_count(fc, fc->cap / 2u);
#if HZ10_ENABLE_FRONT_CACHE_STATS
  fc->flush_count += 1u;
#endif
}

/* Owner-thread boundary flush of every class's front cache, phase 0 of
 * hz10_public_entry_flush_thread_cache_quiescent() below. */
static void hz10_front_cache_flush_all(void) {
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10FrontCache* fc = &hz10_front_cache[c];
    if (fc->cap != 0u && fc->count != 0u) {
      hz10_front_cache_flush_to_count(fc, 0u);
#if HZ10_ENABLE_FRONT_CACHE_STATS
      fc->flush_count += 1u;
#endif
    }
  }
}

#endif /* HZ10_ENABLE_FRONT_CACHE */

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
#if HZ10_ENABLE_FRONT_CACHE
#if HZ10_FRONT_CACHE_MIN_CLASS > 0
  if (class_id < HZ10_FRONT_CACHE_MIN_CLASS) {
    return hz10_public_entry_alloc_from_page_layer(class_id);
  }
#endif
  Hz10FrontCache* fc = &hz10_front_cache[class_id];
#if HZ10_ENABLE_FRONT_CACHE_ARRAY
  if (fc->count != 0u) {
    fc->count -= 1u;
    void* slot = fc->slots[fc->count];
#else
  void* slot = fc->head;
  if (slot) {
    fc->head = *(void**)slot;
    fc->count -= 1u;
#endif
    hz10_front_cache_clear_marker(fc->slot_size, slot);
#if HZ10_ENABLE_FRONT_CACHE_STATS
    fc->hit_count += 1u;
#endif
    return slot;
  }
  return hz10_front_cache_refill_and_alloc(class_id, fc);
#else
  return hz10_public_entry_alloc_from_page_layer(class_id);
#endif
}

int hz10_free(void* ptr) {
  if (!ptr) {
    return 1;
  }
  H10RouteLocalResult local_route = {0};
  Hz10FreelistPage* page = NULL;
  uint32_t route_generation = 0u;
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
  uint32_t route_slot_size = 0u;
#endif
  if (hz10_pagemap_route_local_fast(ptr, &local_route)) {
    page = (Hz10FreelistPage*)local_route.owner;
    route_generation = local_route.generation;
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
    route_slot_size = local_route.slot_size;
#endif
  } else {
    H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
    if (route.kind != H10_ROUTE_VALID) {
      return 0;
    }
    if (route.flags == HZ10_PAGEMAP_FLAG_LARGE) {
      hz10_large_free(route.page_base, route.slot_size);
      return 1;
    }
    page = (Hz10FreelistPage*)route.owner;
    route_generation = route.generation;
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
    route_slot_size = route.slot_size;
#endif
  }
  if (!page) {
    return 0;
  }
  Hz10ThreadOwner* owner = hz10_public_entry_current_owner_if_any();
  if (owner && hz10_freelist_page_owner_thread(page) == owner) {
#if HZ10_ENABLE_FRONT_CACHE
    /* Front-cache push, NOT hz10_freelist_page_free() -- see the
     * HZ10FrontCache-L1 block comment above: the slot stays allocated
     * from the page's point of view until overflow/lifecycle flush
     * returns it through the page-layer boundary. */
    uint32_t class_id = page->class_id;
#if HZ10_FRONT_CACHE_MIN_CLASS > 0
    if (class_id < HZ10_FRONT_CACHE_MIN_CLASS) {
      hz10_freelist_page_free(page, ptr);
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
      if (route_slot_size >= HZ10_RETIRED_LOCAL_IDLE_MIN_SLOT_SIZE &&
          route_slot_size <= HZ10_RETIRED_LOCAL_IDLE_MAX_SLOT_SIZE) {
        hz10_public_entry_note_owner_local_free(class_id, page);
      }
#endif
      return 1;
    }
#endif
    Hz10FrontCache* fc = &hz10_front_cache[class_id];
    hz10_freelist_page_mark_local_free(page, ptr);
#if HZ10_ENABLE_FRONT_CACHE_ARRAY
    fc->slots[fc->count] = ptr;
#else
    *(void**)ptr = fc->head;
    fc->head = ptr;
#endif
    fc->count += 1u;
    if (fc->count > fc->cap) { /* also the lazy-init entry, see overflow() */
      hz10_front_cache_overflow(fc, class_id);
    }
    return 1;
#else
    hz10_freelist_page_free(page, ptr);
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
    if (route_slot_size >= HZ10_RETIRED_LOCAL_IDLE_MIN_SLOT_SIZE &&
        route_slot_size <= HZ10_RETIRED_LOCAL_IDLE_MAX_SLOT_SIZE) {
      hz10_public_entry_note_owner_local_free(page->class_id, page);
    }
#endif
    return 1;
#endif
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
  if (!hz10_page_remote_free_claim(page, ptr, route_generation,
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
  const Hz10ThreadOwner* owner = hz10_public_entry_current_owner_if_any();
  if (!owner) {
    *stats_out = (Hz10ClassPageListStats){0};
    return;
  }
  const Hz10ClassPageList* list = &owner->classes[class_id].list;
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
  stats_out->retired_reclaimed_by_local_free_count =
      list->retired_reclaimed_by_local_free_count;
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

void hz10_public_entry_front_cache_stats(uint32_t class_id,
                                         Hz10FrontCacheStats* stats_out) {
  if (!stats_out) {
    return;
  }
  *stats_out = (Hz10FrontCacheStats){0};
#if HZ10_ENABLE_FRONT_CACHE
  if (class_id >= HZ10_CLASS_COUNT) {
    return;
  }
  const Hz10FrontCache* fc = &hz10_front_cache[class_id];
  stats_out->count = fc->count;
  stats_out->cap = fc->cap;
  stats_out->refill_batch = fc->refill_batch;
  stats_out->slot_size = fc->slot_size;
#if HZ10_ENABLE_FRONT_CACHE_STATS
  stats_out->hit_count = fc->hit_count;
  stats_out->refill_count = fc->refill_count;
  stats_out->refill_objects = fc->refill_objects;
  stats_out->flush_count = fc->flush_count;
  stats_out->flush_objects = fc->flush_objects;
#endif
#else
  (void)class_id;
#endif
}
