#include "../src/hz10_class_pages.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_public_entry.h"
#include "../src/hz10_size_class.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Case 1: basic alloc/free/reuse across several distinct classes. */
static int check_multi_class_basic(void) {
  size_t sizes[] = {1, 16, 17, 64, 1000, 4096, 8192, 65536};
  void* ptrs[sizeof(sizes) / sizeof(sizes[0])];
  int failed = 0;

  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    ptrs[i] = hz10_malloc(sizes[i]);
    if (!ptrs[i]) {
      fprintf(stderr, "multi_class: malloc(%zu) failed\n", sizes[i]);
      failed = 1;
      continue;
    }
    memset(ptrs[i], (int)(0xA0u + i), sizes[i]);
  }
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    if (ptrs[i] && !hz10_free(ptrs[i])) {
      fprintf(stderr, "multi_class: free of size %zu rejected\n", sizes[i]);
      failed = 1;
    }
  }

  /* LIFO reuse: freeing then immediately re-requesting the same class
   * should hand back the same address (matches Box 2's own LIFO smoke). */
  void* p1 = hz10_malloc(64);
  if (!p1 || !hz10_free(p1)) {
    fprintf(stderr, "multi_class: reuse setup failed\n");
    return 1;
  }
  void* p2 = hz10_malloc(64);
  if (p2 != p1) {
    fprintf(stderr, "multi_class: expected LIFO reuse of the same slot\n");
    failed = 1;
  }
  if (p2 && !hz10_free(p2)) {
    failed = 1;
  }
  return failed;
}

/* Case 2: rejected inputs -- zero and genuinely unknown pointers, per
 * hz10_public_entry.h's stated L0 scope. size > HZ10_PAGE_QUANTUM is no
 * longer rejected here -- see check_large_alloc_basic below. */
static int check_rejected_inputs(void) {
  int failed = 0;
  if (hz10_malloc(0) != NULL) {
    fprintf(stderr, "rejected: malloc(0) should return NULL\n");
    failed = 1;
  }
  if (hz10_free(NULL) != 1) {
    fprintf(stderr, "rejected: free(NULL) should be a no-op success\n");
    failed = 1;
  }
  int local_stack_var;
  if (hz10_free(&local_stack_var) != 0) {
    fprintf(stderr, "rejected: free of an unknown address should fail\n");
    failed = 1;
  }
  void* p = hz10_malloc(128);
  if (!p) {
    fprintf(stderr, "rejected: setup malloc failed\n");
    return 1;
  }
  if (hz10_free((char*)p + 4) != 0) {
    fprintf(stderr, "rejected: interior pointer should be rejected\n");
    failed = 1;
  }
  if (!hz10_free(p)) {
    failed = 1;
  }
  return failed;
}

/* Case 3: a page that becomes fully exhausted stops being the active page
 * for its class (a fresh page is created instead), but it must stay
 * correctly freeable by its original (owning) thread, and -- since
 * src/hz10_class_pages.h -- it also stays trackable rather than truly
 * abandoned (exercised separately by check_exhausted_page_recovered_via_
 * remote_free below). */
static int check_abandoned_page_still_freeable(void) {
  /* size class 32768 has slot_count == 2 (HZ10_PAGE_QUANTUM / 32768),
   * so two allocations exhaust it deterministically. */
  void* p1 = hz10_malloc(32768);
  void* p2 = hz10_malloc(32768);
  if (!p1 || !p2) {
    fprintf(stderr, "abandoned: setup malloc failed\n");
    return 1;
  }
  /* This class's active page is now exhausted with nothing to drain, so
   * the next malloc creates a genuinely fresh page. */
  void* p3 = hz10_malloc(32768);
  if (!p3) {
    fprintf(stderr, "abandoned: malloc after exhaustion failed\n");
    return 1;
  }

  int failed = 0;
  if (!hz10_free(p1)) {
    fprintf(stderr, "abandoned: free of p1 (abandoned page) rejected\n");
    failed = 1;
  }
  if (!hz10_free(p2)) {
    fprintf(stderr, "abandoned: free of p2 (abandoned page) rejected\n");
    failed = 1;
  }
  if (!hz10_free(p3)) {
    fprintf(stderr, "abandoned: free of p3 (current active page) rejected\n");
    failed = 1;
  }
  return failed;
}

static void* hz10_smoke_free_in_other_thread(void* arg) {
  void* ptr = *(void**)arg;
  return (void*)(intptr_t)hz10_free(ptr);
}

/* Case 3b: the actual fix under test. A page exhausted locally, then freed
 * from a foreign thread (so the slot lands in Box 3's remote stack, not the
 * local freelist), must be found and drained by src/hz10_class_pages.h's
 * scan -- proven by getting the *exact same address* back out, not merely a
 * fresh page. Before this fix, the page was abandoned the moment it looked
 * exhausted and this remotely-freed slot would never have been revisited. */
static int check_exhausted_page_recovered_via_remote_free(void) {
#if HZ10_ENABLE_FRONT_CACHE
  /* This case asserts PAGE-layer recovery (drain hands the exact remotely-
   * freed slot back). Under the front cache, stock left in this class by
   * earlier cases in this same main thread would answer the malloc first
   * and mask the page-layer behavior being tested -- start from an empty
   * front cache instead. The main thread is quiescent here (every earlier
   * case joined its worker threads), satisfying the flush precondition.
   * With the flush, p1 comes from a fresh page's refill and p2 pops that
   * refill's one extra slot, so both still share one 2-slot page and the
   * drain-recovery assertion below tests the same mechanism as the
   * flag-off build (recovery now happens inside refill's page-layer
   * call, which is the point). */
  hz10_public_entry_flush_thread_cache_quiescent(NULL);
#endif
  void* p1 = hz10_malloc(32768);
  void* p2 = hz10_malloc(32768);
  if (!p1 || !p2) {
    fprintf(stderr, "recovered: setup malloc failed\n");
    return 1;
  }

  pthread_t thread;
  if (pthread_create(&thread, NULL, hz10_smoke_free_in_other_thread, &p1) !=
      0) {
    fprintf(stderr, "recovered: pthread_create failed\n");
    return 1;
  }
  void* ret = NULL;
  pthread_join(thread, &ret);
  if ((intptr_t)ret != 1) {
    fprintf(stderr, "recovered: remote free of p1 was rejected\n");
    return 1;
  }

  /* This test's own p1/p2 page has no local capacity (both of its 2 slots
   * were allocated) but does have a pending remote free sitting in Box 3's
   * stack, so find_with_capacity must drain it and hand p1 straight back
   * out -- rather than paying for yet another fresh page. */
  void* p3 = hz10_malloc(32768);
  if (p3 != p1) {
    fprintf(stderr,
            "recovered: expected the remotely-freed slot back (got a "
            "different address -- the exhausted page was not recovered)\n");
    hz10_free(p3);
    hz10_free(p2);
    return 1;
  }

  int failed = 0;
  if (!hz10_free(p2)) {
    failed = 1;
  }
  if (!hz10_free(p3)) {
    failed = 1;
  }
  return failed;
}

/* Case 3c: locks in the accepted scan-limit tradeoff documented in
 * src/hz10_class_pages.h directly -- a page freed while buried deeper
 * than HZ10_CLASS_PAGES_SCAN_LIMIT other pages from the list head has
 * real capacity, but hz10_malloc must not find it (the scan gives up
 * before reaching it), so the next allocation gets a genuinely fresh
 * page instead. */
static int check_scan_limit_tradeoff(void) {
  void* buried = hz10_malloc(65536); /* slot_count==1 for this class */
  if (!buried) {
    fprintf(stderr, "scan_limit: setup malloc failed\n");
    return 1;
  }
  if (!hz10_free(buried)) {
    fprintf(stderr, "scan_limit: free of buried pointer rejected\n");
    return 1;
  }

  /* Bury it under enough fresh, never-freed pages (each its own
   * slot_count==1 page, added at the list head) to push it past the
   * scan window. */
  for (uint32_t i = 0; i < HZ10_CLASS_PAGES_SCAN_LIMIT + 8u; ++i) {
    void* p = hz10_malloc(65536);
    if (!p) {
      fprintf(stderr, "scan_limit: fresh malloc %u failed\n", i);
      return 1;
    }
  }

  void* next = hz10_malloc(65536);
  if (!next) {
    fprintf(stderr, "scan_limit: malloc after burying failed\n");
    return 1;
  }
  if (next == buried) {
    fprintf(stderr,
            "scan_limit: expected the buried page's capacity to stay "
            "invisible past the scan window (got it back anyway -- "
            "HZ10_CLASS_PAGES_SCAN_LIMIT may have changed without "
            "updating this test)\n");
    return 1;
  }
  return 0;
}

/* Case 4b: hz10_public_entry_class_list_stats() (src/hz10_public_entry.h)
 * reports what check_scan_limit_tradeoff() just did -- length bounded at
 * HZ10_CLASS_PAGES_SCAN_LIMIT and at least one eviction recorded (that
 * function's setup guarantees several: it pushes HZ10_CLASS_PAGES_SCAN_
 * LIMIT + 8 never-freed pages through a slot_count==1 class). Must run
 * right after check_scan_limit_tradeoff so this reflects that state, not
 * a fresh one. Does not assert on eviction_reclaimed_count here -- every
 * page that function's loop evicts still has an outstanding, never-freed
 * slot (non-idle), so 0 is the correct value for this specific scenario;
 * the idle-reclaim path itself is tested directly in
 * tests/hz10_class_pages_smoke.c, where a fabricated idle tail can be
 * constructed without hz10_malloc's own state->active reuse getting in
 * the way first. */
static int check_class_list_stats_accessor(void) {
  uint32_t class_id = hz10_size_class_for(65536u);
  if (class_id >= HZ10_CLASS_COUNT) {
    fprintf(stderr, "class_list_stats: no class for probe size\n");
    return 1;
  }

  Hz10ClassPageListStats stats;
  hz10_public_entry_class_list_stats(class_id, &stats);

  if (stats.active_length != HZ10_CLASS_PAGES_SCAN_LIMIT) {
    fprintf(stderr, "class_list_stats: active_length=%u, expected %u\n",
            stats.active_length, HZ10_CLASS_PAGES_SCAN_LIMIT);
    return 1;
  }
  if (stats.eviction_count == 0u) {
    fprintf(stderr,
            "class_list_stats: eviction_count=0, expected > 0 after "
            "check_scan_limit_tradeoff's setup\n");
    return 1;
  }
  /* check_scan_limit_tradeoff's setup never frees anything after its one
   * initial reuse (see that function), so every evicted page there is
   * non-idle -- they all route through `retired`, none reclaimed
   * immediately. This also doubles as a real-workload check that the
   * active/retired split actually engages, not just the isolated unit
   * tests in tests/hz10_class_pages_smoke.c. */
  if (stats.retired_count == 0u) {
    fprintf(stderr,
            "class_list_stats: retired_count=0, expected > 0 (evicted "
            "pages here are never idle, so they should all route through "
            "retired)\n");
    return 1;
  }

  /* Out-of-range class_id is a documented no-op (zero-filled), not
   * undefined behavior -- seed with sentinel values first so a no-op is
   * actually observable. */
  Hz10ClassPageListStats oob_stats;
  oob_stats.active_length = 999u;
  oob_stats.eviction_count = 999u;
  oob_stats.retired_count = 999u;
  hz10_public_entry_class_list_stats(HZ10_CLASS_COUNT, &oob_stats);
  if (oob_stats.active_length != 0u || oob_stats.eviction_count != 0u ||
      oob_stats.retired_count != 0u) {
    fprintf(stderr,
            "class_list_stats: out-of-range class_id was not a no-op "
            "(active_length=%u eviction_count=%llu retired_count=%llu)\n",
            oob_stats.active_length,
            (unsigned long long)oob_stats.eviction_count,
            (unsigned long long)oob_stats.retired_count);
    return 1;
  }
  return 0;
}

/* Case 4c: lifecycle reclaim must process a retired page already linked
 * onto the ready stack through the ready owner path, not through the direct
 * retired walk. Before the ready-drain phase this page was either
 * unsafely destroyed by the direct walk or safely but incompletely
 * deferred; L1 should safely reclaim it after pop()+generation check. */
static int check_thread_reclaim_drains_ready_retired_page(void) {
  void* target_a = hz10_malloc(24576);
  void* target_b = hz10_malloc(24576);
  if (!target_a || !target_b) {
    fprintf(stderr, "thread_reclaim_ready: target setup failed\n");
    return 1;
  }

  for (uint32_t i = 0; i < HZ10_CLASS_PAGES_SCAN_LIMIT + 1u; ++i) {
    void* a = hz10_malloc(24576);
    void* b = hz10_malloc(24576);
    if (!a || !b) {
      fprintf(stderr, "thread_reclaim_ready: filler %u failed\n", i);
      return 1;
    }
  }

  pthread_t thread_a;
  pthread_t thread_b;
  if (pthread_create(&thread_a, NULL, hz10_smoke_free_in_other_thread,
                     &target_a) != 0 ||
      pthread_create(&thread_b, NULL, hz10_smoke_free_in_other_thread,
                     &target_b) != 0) {
    fprintf(stderr, "thread_reclaim_ready: pthread_create failed\n");
    return 1;
  }
  void* ret_a = NULL;
  void* ret_b = NULL;
  pthread_join(thread_a, &ret_a);
  pthread_join(thread_b, &ret_b);
  if ((intptr_t)ret_a != 1 || (intptr_t)ret_b != 1) {
    fprintf(stderr, "thread_reclaim_ready: remote free rejected\n");
    return 1;
  }

  Hz10PublicEntryThreadReclaimStats reclaim_stats;
  hz10_public_entry_flush_thread_cache_quiescent(&reclaim_stats);
  if (reclaim_stats.pages_reclaimed == 0u) {
    fprintf(stderr,
            "thread_reclaim_ready: expected ready-stacked retired page to "
            "be reclaimed by the ready-drain phase\n");
    return 1;
  }
  if (reclaim_stats.pages_deferred_ready != 0u) {
    fprintf(stderr,
            "thread_reclaim_ready: ready-stacked page was deferred instead "
            "of being drained first\n");
    return 1;
  }
  return 0;
}

/* Case 4d: the basic half of the HZ10LifecycleFlushContract-L1 contract in
 * src/hz10_public_entry.h -- a genuinely busy page (a live, un-freed
 * allocation, never evicted to retired, never linked onto any ready stack)
 * must survive hz10_public_entry_flush_thread_cache_quiescent() completely
 * untouched: reported via pages_busy, not destroyed, not corrupted, still
 * freeable afterward. Distinct from check_thread_reclaim_drains_ready_
 * retired_page() above, which covers the harder ready-stacked case. */
static int check_thread_reclaim_leaves_busy_page(void) {
  void* live = hz10_malloc(24576);
  if (!live) {
    fprintf(stderr, "thread_reclaim_busy: setup malloc failed\n");
    return 1;
  }
  memset(live, 0x7A, 24576);

  Hz10PublicEntryThreadReclaimStats stats;
  hz10_public_entry_flush_thread_cache_quiescent(&stats);
  if (stats.pages_busy == 0u) {
    fprintf(stderr,
            "thread_reclaim_busy: expected the live page to be reported "
            "busy, not destroyed\n");
    return 1;
  }

  unsigned char* bytes = (unsigned char*)live;
  if (bytes[0] != 0x7A || bytes[24576 - 1] != 0x7A) {
    fprintf(stderr,
            "thread_reclaim_busy: live allocation corrupted by flush\n");
    return 1;
  }
  if (!hz10_free(live)) {
    fprintf(stderr,
            "thread_reclaim_busy: live allocation not freeable after "
            "flush\n");
    return 1;
  }
  return 0;
}

#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
static int check_retired_local_idle_reclaim(void) {
  const size_t size = 3072u;
  const uint32_t class_id = hz10_size_class_for(size);
  const uint32_t slot_count = hz10_size_class_slot_count(class_id);
  if (class_id >= HZ10_CLASS_COUNT) return 1;

  void* retired_slots[32];
  if (slot_count > (uint32_t)(sizeof(retired_slots) / sizeof(retired_slots[0])))
    return 1;
  for (uint32_t i = 0; i < slot_count; ++i) {
    retired_slots[i] = hz10_malloc(size);
    if (!retired_slots[i]) {
      fprintf(stderr, "retired_local_idle: initial malloc %u failed\n", i);
      return 1;
    }
  }
  for (uint32_t i = 0; i < HZ10_CLASS_PAGES_SCAN_LIMIT * slot_count; ++i) {
    void* ptr = hz10_malloc(size);
    if (!ptr) {
      fprintf(stderr, "retired_local_idle: filler %u failed\n", i);
      return 1;
    }
  }
  Hz10ClassPageListStats before;
  hz10_public_entry_class_list_stats(class_id, &before);
  if (before.retired_length == 0u) return 1;
  for (uint32_t i = 0; i < slot_count; ++i) {
    if (!hz10_free(retired_slots[i])) {
      fprintf(stderr, "retired_local_idle: local free %u failed\n", i);
      return 1;
    }
  }

  Hz10ClassPageListStats after;
  hz10_public_entry_class_list_stats(class_id, &after);
  if (after.retired_reclaimed_by_local_free_count !=
      before.retired_reclaimed_by_local_free_count + 1u) {
    fprintf(stderr,
            "retired_local_idle: local-free reclaim counter did not advance "
            "(before=%llu after=%llu)\n",
            (unsigned long long)before.retired_reclaimed_by_local_free_count,
            (unsigned long long)after.retired_reclaimed_by_local_free_count);
    return 1;
  }
  if (after.retired_length + 1u != before.retired_length) {
    fprintf(stderr,
            "retired_local_idle: retired_length %u -> %u, expected -1\n",
            before.retired_length, after.retired_length);
    return 1;
  }
  return 0;
}
#endif
/* Case 5: basic large-object path (src/hz10_large_alloc.h) -- sizes at and
 * beyond HZ10_PAGE_QUANTUM+1 must now succeed (they used to be rejected
 * outright), the full requested range must be genuinely writable (not just
 * the base pointer), and rounding to different HZ10_PAGE_QUANTUM multiples
 * must not collide with each other. */
static int check_large_alloc_basic(void) {
  size_t sizes[] = {(size_t)HZ10_PAGE_QUANTUM + 1u, /* rounds up to 2 quanta */
                    (size_t)HZ10_PAGE_QUANTUM * 2u,  /* exactly 2 quanta */
                    (size_t)HZ10_PAGE_QUANTUM * 5u + 100u};
  void* ptrs[sizeof(sizes) / sizeof(sizes[0])];
  int failed = 0;

  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    ptrs[i] = hz10_malloc(sizes[i]);
    if (!ptrs[i]) {
      fprintf(stderr, "large_alloc: malloc(%zu) failed\n", sizes[i]);
      failed = 1;
      continue;
    }
    memset(ptrs[i], (int)(0xC0u + i), sizes[i]);
  }
  /* Distinct large allocations must not overlap -- corrupting one via a
   * write to another would show up as a mismatch here. */
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    if (!ptrs[i]) {
      continue;
    }
    unsigned char* bytes = (unsigned char*)ptrs[i];
    unsigned char expect = (unsigned char)(0xC0u + i);
    if (bytes[0] != expect || bytes[sizes[i] - 1u] != expect) {
      fprintf(stderr, "large_alloc: allocation %zu corrupted\n", i);
      failed = 1;
    }
  }
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    if (ptrs[i] && !hz10_free(ptrs[i])) {
      fprintf(stderr, "large_alloc: free of size %zu rejected\n", sizes[i]);
      failed = 1;
    }
  }
  return failed;
}

/* Case 6: a large allocation rejects an interior pointer (matching the
 * same universal rule every small class already enforces), and a large
 * allocation freed on a different thread than it was allocated on works
 * (src/hz10_large_alloc.h documents there is no per-thread ownership
 * concept for large objects, unlike Box 3's remote path for small
 * classes -- free() there does not care which thread calls it). */
static int check_large_alloc_edges(void) {
  void* p = hz10_malloc((size_t)HZ10_PAGE_QUANTUM + 1u);
  if (!p) {
    fprintf(stderr, "large_alloc_edges: setup malloc failed\n");
    return 1;
  }
  if (hz10_free((char*)p + 16) != 0) {
    fprintf(stderr,
            "large_alloc_edges: interior pointer should be rejected\n");
    hz10_free(p);
    return 1;
  }

  pthread_t thread;
  if (pthread_create(&thread, NULL, hz10_smoke_free_in_other_thread, &p) !=
      0) {
    fprintf(stderr, "large_alloc_edges: pthread_create failed\n");
    return 1;
  }
  void* ret = NULL;
  pthread_join(thread, &ret);
  if ((intptr_t)ret != 1) {
    fprintf(stderr,
            "large_alloc_edges: cross-thread free of a large allocation "
            "was rejected\n");
    return 1;
  }
  return 0;
}

/* Case 4: cross-thread free. A pointer allocated on this thread, freed on
 * a different thread, must route through Box 3's remote path (accepted,
 * but not yet visible to this thread's allocator) and eventually come
 * back once this thread's own allocation traffic drains that page. */
static int check_cross_thread_free(void) {
  void* target = hz10_malloc(128);
  if (!target) {
    fprintf(stderr, "cross_thread: setup malloc failed\n");
    return 1;
  }

  pthread_t thread;
  if (pthread_create(&thread, NULL, hz10_smoke_free_in_other_thread,
                     &target) != 0) {
    fprintf(stderr, "cross_thread: pthread_create failed\n");
    return 1;
  }
  void* ret = NULL;
  pthread_join(thread, &ret);
  if ((intptr_t)ret != 1) {
    fprintf(stderr, "cross_thread: remote free was rejected\n");
    return 1;
  }

  /* Drive enough allocations of the same class to force exhaustion (and
   * therefore an exhaustion-time drain) until the remotely-freed slot
   * comes back. Bounded so a real regression fails instead of looping
   * forever. */
  for (int i = 0; i < 100000; ++i) {
    void* p = hz10_malloc(128);
    if (!p) {
      fprintf(stderr, "cross_thread: malloc failed while waiting for "
                      "drain at iter %d\n",
              i);
      return 1;
    }
    if (p == target) {
      return 0;
    }
  }
  fprintf(stderr, "cross_thread: remotely-freed pointer was never "
                  "recovered\n");
  return 1;
}

#if HZ10_ENABLE_FRONT_CACHE
/*
 * HZ10FrontCache-L1 gate cases (docs/HZ10_FRONT_CACHE_DESIGN_L0.md). Both
 * run in a fresh thread so the front cache / class lists under test start
 * empty and the quiescent-flush precondition trivially holds at the end.
 */

/* Gate: a front-cached slot stays ALLOCATED from its page's point of view
 * (free_count unchanged by hz10_free), and lifecycle flush phase 0 returns
 * every cached slot so the page is reclaimed, not left pinned busy. */
static void* front_cache_accounting_thread(void* raw) {
  int* failed = (int*)raw;
  *failed = 1;

  const size_t size = 4096; /* 16 slots/page: several ptrs share one page */
  const uint32_t class_id = hz10_size_class_for(size);
  enum { N = 8 };
  void* ptrs[N];
  for (int i = 0; i < N; ++i) {
    ptrs[i] = hz10_malloc(size);
    if (!ptrs[i]) {
      fprintf(stderr, "front_accounting: setup malloc %d failed\n", i);
      return NULL;
    }
    memset(ptrs[i], 0xC1, size);
  }

  H10RouteResult route = hz10_pagemap_route(ptrs[0], HZ10_GENERATION_ANY);
  if (route.kind != H10_ROUTE_VALID || !route.owner) {
    fprintf(stderr, "front_accounting: route of live ptr failed\n");
    return NULL;
  }
  Hz10FreelistPage* page = (Hz10FreelistPage*)route.owner;
  uint32_t free_count_before = page->free_count;

  for (int i = 0; i < N; ++i) {
    if (!hz10_free(ptrs[i])) {
      fprintf(stderr, "front_accounting: free %d rejected\n", i);
      return NULL;
    }
  }

  if (page->free_count != free_count_before) {
    fprintf(stderr,
            "front_accounting: free_count moved %u -> %u on front-cache "
            "push (must stay allocated until flush)\n",
            free_count_before, page->free_count);
    return NULL;
  }
  Hz10FrontCacheStats front_stats;
  hz10_public_entry_front_cache_stats(class_id, &front_stats);
  if (front_stats.cap == 0u || front_stats.count < (uint32_t)N) {
    fprintf(stderr, "front_accounting: expected >=%d cached, cap!=0; got "
                    "count=%u cap=%u\n",
            N, front_stats.count, front_stats.cap);
    return NULL;
  }

  Hz10PublicEntryThreadReclaimStats reclaim_stats;
  hz10_public_entry_flush_thread_cache_quiescent(&reclaim_stats);

  hz10_public_entry_front_cache_stats(class_id, &front_stats);
  if (front_stats.count != 0u) {
    fprintf(stderr, "front_accounting: %u objects survived phase-0 flush\n",
            front_stats.count);
    return NULL;
  }
  if (reclaim_stats.pages_busy != 0u) {
    fprintf(stderr,
            "front_accounting: %llu busy pages after flush (front-cached "
            "slots pinned their page)\n",
            (unsigned long long)reclaim_stats.pages_busy);
    return NULL;
  }
  Hz10ClassPageListStats list_stats;
  hz10_public_entry_class_list_stats(class_id, &list_stats);
  if (list_stats.active_length != 0u || list_stats.retired_length != 0u) {
    fprintf(stderr, "front_accounting: pages left after flush (active=%u "
                    "retired=%u)\n",
            list_stats.active_length, list_stats.retired_length);
    return NULL;
  }
  *failed = 0;
  return NULL;
}

static int check_front_cache_accounting_and_lifecycle(void) {
  int failed = 1;
  pthread_t thread;
  if (pthread_create(&thread, NULL, front_cache_accounting_thread, &failed)) {
    fprintf(stderr, "front_accounting: pthread_create failed\n");
    return 1;
  }
  pthread_join(thread, NULL);
  return failed;
}

typedef struct ForeignDupFreeArg {
  void* ptr;
  int result;
} ForeignDupFreeArg;

static void* foreign_dup_free_thread(void* raw) {
  ForeignDupFreeArg* arg = (ForeignDupFreeArg*)raw;
  arg->result = hz10_free(arg->ptr);
  return NULL;
}

/* Gate: a foreign thread's duplicate free of a front-cached slot is
 * rejected through the existing local-free-marker check in
 * hz10_page_remote_free_claim(), and the cached slot survives intact. */
static void* front_cache_dup_free_thread(void* raw) {
  int* failed = (int*)raw;
  *failed = 1;

  void* p = hz10_malloc(64);
  if (!p) {
    fprintf(stderr, "front_dup: setup malloc failed\n");
    return NULL;
  }
  memset(p, 0xC2, 64);
  if (!hz10_free(p)) {
    fprintf(stderr, "front_dup: owner free rejected\n");
    return NULL;
  }

  ForeignDupFreeArg arg = {p, -1};
  pthread_t thread;
  if (pthread_create(&thread, NULL, foreign_dup_free_thread, &arg)) {
    fprintf(stderr, "front_dup: pthread_create failed\n");
    return NULL;
  }
  pthread_join(thread, NULL);
  if (arg.result != 0) {
    fprintf(stderr, "front_dup: foreign duplicate free of a front-cached "
                    "slot returned %d, want 0 (rejected)\n",
            arg.result);
    return NULL;
  }

  /* The cached slot must come back usable (LIFO) despite the attempt. */
  void* q = hz10_malloc(64);
  if (q != p) {
    fprintf(stderr, "front_dup: expected LIFO reuse of the cached slot\n");
    return NULL;
  }
  memset(q, 0xC3, 64);
  if (!hz10_free(q)) {
    fprintf(stderr, "front_dup: re-free rejected\n");
    return NULL;
  }
  hz10_public_entry_flush_thread_cache_quiescent(NULL);
  *failed = 0;
  return NULL;
}

static int check_front_cache_duplicate_remote_free_rejected(void) {
  int failed = 1;
  pthread_t thread;
  if (pthread_create(&thread, NULL, front_cache_dup_free_thread, &failed)) {
    fprintf(stderr, "front_dup: pthread_create failed\n");
    return 1;
  }
  pthread_join(thread, NULL);
  return failed;
}
#endif /* HZ10_ENABLE_FRONT_CACHE */

int main(void) {
  hz10_pagemap_reset_for_tests();

  if (check_multi_class_basic()) {
    return 2;
  }
  if (check_rejected_inputs()) {
    return 3;
  }
  if (check_abandoned_page_still_freeable()) {
    return 4;
  }
  if (check_exhausted_page_recovered_via_remote_free()) {
    return 5;
  }
  if (check_scan_limit_tradeoff()) {
    return 6;
  }
  if (check_class_list_stats_accessor()) {
    return 7;
  }
  if (check_thread_reclaim_drains_ready_retired_page()) {
    return 8;
  }
  if (check_thread_reclaim_leaves_busy_page()) {
    return 12;
  }
  if (check_large_alloc_basic()) {
    return 9;
  }
  if (check_large_alloc_edges()) {
    return 10;
  }
  if (check_cross_thread_free()) {
    return 11;
  }
#if HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM
  if (check_retired_local_idle_reclaim()) {
    return 15;
  }
#endif
#if HZ10_ENABLE_FRONT_CACHE
  if (check_front_cache_accounting_and_lifecycle()) {
    return 13;
  }
  if (check_front_cache_duplicate_remote_free_rejected()) {
    return 14;
  }
#endif

  puts("hz10_public_entry_smoke ok");
  return 0;
}
