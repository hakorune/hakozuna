#include "../src/hz10_freelist_page.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_remote_stack.h"
#include "../src/hz10_retired_ready.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Isolated comparison bench for HZ10RetiredReadyQueue-L0 (src/
 * hz10_retired_ready.{h,c}) against Box 6's existing budgeted-cursor
 * polling (src/hz10_class_pages.c's hz10_class_pages_harvest_retired_
 * capacity(), reimplemented here in miniature against a plain doubly-
 * linked list rather than the real Hz10ClassPageList, since this bench
 * does not need the active-list/eviction machinery -- only the retired-
 * side discovery question). See current_task.md's "polling-vs-event-
 * driven" entry for the full context this responds to.
 *
 * Design: two INDEPENDENT sets of pages (a "polling" set and an "event-
 * driven" set), same slot shape (HZ10_RR_SLOT_COUNT=2, matching main_r50's
 * dominant pathological class -- see current_task.md's
 * "RetiredPartialReuse-L1" entry for why size=32768/slot_count=2
 * specifically), same total page count, same producer-thread free
 * pattern, run concurrently in the same process so both experience
 * identical scheduling pressure -- the only variable is HOW the owner
 * discovers a page has become reclaimable. A single owner thread creates
 * pages for both sets (alternating) and only checks in periodically
 * (OWNER_CHECK_INTERVAL creations apart), deliberately sparse to mirror
 * the real bench's observed rarity of hz10_malloc's genuine-miss slow
 * path. On each check-in: the polling set gets a budgeted (BUDGET=4,
 * matching HZ10_CLASS_PAGES_SWEEP_BUDGET) cursor-walk destroy pass; the
 * event-driven set gets a full drain-to-empty pop pass (O(1) per pop, no
 * scan). Producer threads free both sets' pages' slots as fast as
 * possible (no artificial delay -- discovery overhead is what is being
 * measured here, not free latency).
 *
 * What this does NOT reproduce (honest scope limit): the real bench's
 * inbox-drain-every-32-iterations cadence, multiple concurrent size
 * classes, or hz10_malloc's actual genuine-miss frequency -- this is a
 * synthetic, deliberately simplified proxy for the SHAPE of the problem
 * (sparse discovery opportunities vs. continuous resolution), not a
 * reproduction of the exact bench. Track unresolved-page-count over time
 * as the RSS proxy, not absolute ops/s.
 *
 * A documented, pre-existing, accepted gap this bench must respect (see
 * tests/README.md: "a foreign thread starting a new push concurrently
 * with destroy is not defended against"): hz10_page_remote_free()'s own
 * bookkeeping (remote_push_count) is written AFTER its Treiber-stack CAS
 * publishes the push, so an owner spinning as aggressively as this
 * bench's check-in loop does can observe free_count == slot_count and
 * destroy a page while the producer thread that made the LAST push is
 * still inside that same hz10_page_remote_free() call -- a real
 * use-after-free of the page struct, caught by TSan the first time this
 * bench ran without the guard below. This is not a bug introduced here;
 * it is the documented gap, just never exercised this aggressively by
 * any existing test/bench (all of which check in far less often than
 * this bench's unthrottled busy-loop does). The fix belongs in this
 * bench, not Box 3: each page's owner_thread_token (unused, inert
 * storage here) holds a pointer to a per-page "producer done" flag that
 * the producer sets (release) only after it has finished ALL of its
 * hz10_page_remote_free()/note_remote_free() calls for that page: both
 * checkers require this flag before trusting free_count == slot_count.
 */

#define HZ10_RR_SLOT_SIZE 32768u
#define HZ10_RR_SLOT_COUNT 2u
#define HZ10_RR_POLL_BUDGET 4u

typedef struct PollNode {
  Hz10FreelistPage* page;
  struct PollNode* prev;
  struct PollNode* next;
} PollNode;

typedef struct PollList {
  PollNode* head;
  PollNode* tail;
  PollNode* cursor;
  _Atomic(uint64_t) length;
  _Atomic(uint64_t) destroyed_count;
} PollList;

static void poll_list_prepend(PollList* list, Hz10FreelistPage* page) {
  PollNode* node = malloc(sizeof(*node));
  node->page = page;
  node->prev = NULL;
  node->next = list->head;
  if (list->head) {
    list->head->prev = node;
  } else {
    list->tail = node;
  }
  list->head = node;
  atomic_fetch_add_explicit(&list->length, 1u, memory_order_relaxed);
}

static void poll_list_remove(PollList* list, PollNode* node) {
  if (node->prev) {
    node->prev->next = node->next;
  } else {
    list->head = node->next;
  }
  if (node->next) {
    node->next->prev = node->prev;
  } else {
    list->tail = node->prev;
  }
  atomic_fetch_sub_explicit(&list->length, 1u, memory_order_relaxed);
}

/* page->owner_thread_token points to this page's producer_done flag (see
 * the module comment on the documented concurrent-push-vs-destroy gap).
 * Not reclaimable until the flag reads 1, regardless of free_count. */
static int producer_done(const Hz10FreelistPage* page) {
  return atomic_load_explicit((_Atomic(int)*)page->owner_thread_token,
                              memory_order_acquire);
}

/* Owner-only: budgeted cursor walk, destroy on idle -- same shape as
 * hz10_class_pages_harvest_retired_capacity()'s destroy path (the
 * promote path is irrelevant here: these pages are never reused). */
static void poll_list_check(PollList* list) {
  PollNode* node = list->cursor;
  if (!node) {
    node = list->tail;
  }
  uint32_t checked = 0u;
  while (node && checked < HZ10_RR_POLL_BUDGET) {
    PollNode* prev = node->prev;
    hz10_page_drain_remote(node->page);
    if (node->page->free_count == node->page->slot_count &&
        producer_done(node->page)) {
      poll_list_remove(list, node);
      hz10_freelist_page_destroy(node->page);
      atomic_fetch_add_explicit(&list->destroyed_count, 1u,
                                memory_order_relaxed);
      free(node);
    }
    node = prev;
    ++checked;
  }
  list->cursor = node;
}

/* Owner-only: drain the ready stack to empty. Each candidate is
 * re-verified (drain_remote + free_count check) before destroy, per
 * hz10_retired_ready.h's "hint only" contract. */
static _Atomic(uint64_t) g_event_destroyed_count;
static _Atomic(uint64_t) g_event_false_positive_count;

static void event_list_check(Hz10RetiredReadyStack* stack,
                            _Atomic(uint64_t)* remaining_count) {
  Hz10FreelistPage* page;
  while ((page = hz10_retired_ready_pop(stack)) != NULL) {
    hz10_page_drain_remote(page);
    if (page->free_count == page->slot_count && producer_done(page)) {
      hz10_freelist_page_destroy(page);
      atomic_fetch_add_explicit(&g_event_destroyed_count, 1u,
                                memory_order_relaxed);
      atomic_fetch_sub_explicit(remaining_count, 1u, memory_order_relaxed);
    } else {
      /* False positive (race window -- see hz10_retired_ready.h): not
       * actually idle yet. In a real integration this page would just
       * stay in the retired list for harvest's own cursor to find later;
       * here, for this isolated bench, we simply count it and drop it
       * (leaking it is harmless for a synthetic bench measuring
       * discovery behavior, not memory footprint). */
      atomic_fetch_add_explicit(&g_event_false_positive_count, 1u,
                                memory_order_relaxed);
    }
  }
}

typedef struct ProducerArg {
  Hz10FreelistPage** pages;
  void** ptrs; /* 2 * page_count entries, [2*i], [2*i+1] for page i's slots */
  uint32_t page_count;
  uint32_t stride;
  uint32_t offset;
  int event_driven; /* 1: call hz10_retired_ready_note_remote_free too */
} ProducerArg;

static void* producer_worker(void* raw) {
  ProducerArg* arg = (ProducerArg*)raw;
  for (uint32_t i = arg->offset; i < arg->page_count; i += arg->stride) {
    Hz10FreelistPage* page = arg->pages[i];
    /* All but the last slot: normal free, immediately followed by its
     * note_remote_free() companion call (per that function's own
     * contract) -- these cannot be the call that makes the page
     * reclaimable (at least one more slot is still outstanding), so no
     * ordering hazard here. */
    for (uint32_t s = 0; s + 1u < HZ10_RR_SLOT_COUNT; ++s) {
      void* ptr = arg->ptrs[2u * i + s];
      if (hz10_page_remote_free(page, ptr, HZ10_GENERATION_ANY)) {
        if (arg->event_driven) {
          hz10_retired_ready_note_remote_free(page);
        }
      }
    }
    /* Last slot: this IS the call that can make the page reclaimable, so
     * producer_done must be set only after hz10_page_remote_free() has
     * fully returned (including its own trailing bookkeeping) -- see the
     * module comment on the documented concurrent-push-vs-destroy gap.
     * note_remote_free() is called after, not before, setting the flag:
     * unlike hz10_page_remote_free(), it has no trailing writes after
     * its own publish (the ready-stack CAS), so this ordering is safe
     * without needing the same guard. */
    void* last_ptr = arg->ptrs[2u * i + (HZ10_RR_SLOT_COUNT - 1u)];
    int ok = hz10_page_remote_free(page, last_ptr, HZ10_GENERATION_ANY);
    atomic_store_explicit((_Atomic(int)*)page->owner_thread_token, 1,
                          memory_order_release);
    if (ok && arg->event_driven) {
      hz10_retired_ready_note_remote_free(page);
    }
  }
  return NULL;
}

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

int main(void) {
  uint32_t pages_per_set =
      (uint32_t)hz10_bench_env_u64("PAGES_PER_SET", 50000ull);
  uint32_t owner_check_interval =
      (uint32_t)hz10_bench_env_u64("OWNER_CHECK_INTERVAL", 500ull);
  uint32_t producer_threads =
      (uint32_t)hz10_bench_env_u64("PRODUCER_THREADS", 4ull);

  hz10_pagemap_reset_for_tests();

  Hz10FreelistPage** poll_pages = calloc(pages_per_set, sizeof(*poll_pages));
  Hz10FreelistPage** event_pages = calloc(pages_per_set, sizeof(*event_pages));
  void** poll_ptrs = calloc(2u * pages_per_set, sizeof(*poll_ptrs));
  void** event_ptrs = calloc(2u * pages_per_set, sizeof(*event_ptrs));
  _Atomic(int)* poll_done_flags = calloc(pages_per_set, sizeof(*poll_done_flags));
  _Atomic(int)* event_done_flags =
      calloc(pages_per_set, sizeof(*event_done_flags));
  if (!poll_pages || !event_pages || !poll_ptrs || !event_ptrs ||
      !poll_done_flags || !event_done_flags) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  PollList poll_list = {0};
  Hz10RetiredReadyStack event_stack = {0};
  _Atomic(uint64_t) event_remaining = 0u;

  /* Phase 1: owner creates and retires every page up front (fully
   * allocated, both slots handed out -- matches the real bench's
   * "evicted while fully exhausted" precondition), with periodic
   * check-ins interleaved, alternating one polling-set page and one
   * event-driven-set page per step so both sets age at the same rate. */
  for (uint32_t i = 0; i < pages_per_set; ++i) {
    Hz10FreelistPage* pp =
        hz10_freelist_page_create(HZ10_RR_SLOT_SIZE, HZ10_RR_SLOT_COUNT);
    Hz10FreelistPage* ep =
        hz10_freelist_page_create(HZ10_RR_SLOT_SIZE, HZ10_RR_SLOT_COUNT);
    if (!pp || !ep) {
      fprintf(stderr, "create failed at %u\n", i);
      return 1;
    }
    poll_ptrs[2u * i] = hz10_freelist_page_alloc(pp);
    poll_ptrs[2u * i + 1u] = hz10_freelist_page_alloc(pp);
    event_ptrs[2u * i] = hz10_freelist_page_alloc(ep);
    event_ptrs[2u * i + 1u] = hz10_freelist_page_alloc(ep);
    poll_pages[i] = pp;
    event_pages[i] = ep;
    /* Repurposed as inert per-page bookkeeping for this bench only (see
     * Hz10FreelistPage's own doc comment on owner_thread_token) -- not a
     * real thread token here. */
    pp->owner_thread_token = (void*)&poll_done_flags[i];
    ep->owner_thread_token = (void*)&event_done_flags[i];

    poll_list_prepend(&poll_list, pp);
    hz10_retired_ready_mark(ep, &event_stack, HZ10_RR_SLOT_COUNT);
    atomic_fetch_add_explicit(&event_remaining, 1u, memory_order_relaxed);

    if ((i + 1u) % owner_check_interval == 0u) {
      poll_list_check(&poll_list);
      event_list_check(&event_stack, &event_remaining);
    }
  }

  /* Phase 2: producer threads free every page's slots (as fast as
   * possible -- discovery overhead is the variable under test, not free
   * latency). */
  pthread_t* threads = calloc(2u * producer_threads, sizeof(*threads));
  ProducerArg* args = calloc(2u * producer_threads, sizeof(*args));
  for (uint32_t t = 0; t < producer_threads; ++t) {
    args[t].pages = poll_pages;
    args[t].ptrs = poll_ptrs;
    args[t].page_count = pages_per_set;
    args[t].stride = producer_threads;
    args[t].offset = t;
    args[t].event_driven = 0;
    pthread_create(&threads[t], NULL, producer_worker, &args[t]);
  }
  for (uint32_t t = 0; t < producer_threads; ++t) {
    uint32_t idx = producer_threads + t;
    args[idx].pages = event_pages;
    args[idx].ptrs = event_ptrs;
    args[idx].page_count = pages_per_set;
    args[idx].stride = producer_threads;
    args[idx].offset = t;
    args[idx].event_driven = 1;
    pthread_create(&threads[idx], NULL, producer_worker, &args[idx]);
  }

  /* Owner keeps checking in (one poll-budget-worth of scanning, one
   * full ready-stack drain, per spin) while producers run -- this phase
   * is what actually exercises the discovery-latency difference: no
   * artificial pacing here on purpose, so the metric that matters is HOW
   * MANY check-ins (spins) each mechanism needed to reach zero
   * unresolved, not whether it eventually does (both eventually do,
   * given enough spins -- that alone would not distinguish them). */
  uint32_t spins = 0u;
  uint32_t poll_done_at_spin = 0u;
  uint32_t event_done_at_spin = 0u;
  int all_done = 0;
  while (!all_done) {
    poll_list_check(&poll_list);
    event_list_check(&event_stack, &event_remaining);
    ++spins;
    uint64_t poll_remaining =
        atomic_load_explicit(&poll_list.length, memory_order_relaxed);
    uint64_t ev_remaining =
        atomic_load_explicit(&event_remaining, memory_order_relaxed);
    if (poll_remaining == 0u && poll_done_at_spin == 0u) {
      poll_done_at_spin = spins;
    }
    if (ev_remaining == 0u && event_done_at_spin == 0u) {
      event_done_at_spin = spins;
    }
    all_done = (poll_remaining == 0u && ev_remaining == 0u);
    if (spins > 10000000u) {
      break; /* safety valve, should not be hit */
    }
  }

  for (uint32_t t = 0; t < 2u * producer_threads; ++t) {
    pthread_join(threads[t], NULL);
  }
  /* Final drain in case the last few resolved after the loop above gave
   * up waiting (owner_check_interval-shaped stragglers). */
  poll_list_check(&poll_list);
  poll_list_check(&poll_list);
  event_list_check(&event_stack, &event_remaining);

  printf(
      "hz10_retired_ready_bench pages_per_set=%u owner_check_interval=%u "
      "producer_threads=%u poll_budget=%u\n",
      pages_per_set, owner_check_interval, producer_threads,
      HZ10_RR_POLL_BUDGET);
  printf(
      "  polling:      destroyed=%llu still_unresolved=%llu (of %u) "
      "spins_to_zero=%u\n",
      (unsigned long long)atomic_load_explicit(&poll_list.destroyed_count,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&poll_list.length,
                                              memory_order_relaxed),
      pages_per_set, poll_done_at_spin);
  printf(
      "  event-driven: destroyed=%llu still_unresolved=%llu (of %u) "
      "false_positives=%llu spins_to_zero=%u\n",
      (unsigned long long)atomic_load_explicit(&g_event_destroyed_count,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&event_remaining,
                                              memory_order_relaxed),
      pages_per_set,
      (unsigned long long)atomic_load_explicit(&g_event_false_positive_count,
                                              memory_order_relaxed),
      event_done_at_spin);
  printf("  total owner check-in spins to fully converge both: %u\n", spins);

  return 0;
}
