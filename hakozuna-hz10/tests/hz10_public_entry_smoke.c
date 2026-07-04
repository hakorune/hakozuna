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

/* Case 4c: a retired page already linked onto the ready stack must not be
 * destroyed by the lifecycle reclaim hook, even if draining it makes it
 * fully idle. The normal harvest path has two load-bearing guards for this
 * (retired_ready_on_stack and hz10_retired_ready_cancel()); this locks the
 * same requirement for hz10_public_entry_reclaim_thread_idle_pages(). */
static int check_thread_reclaim_defers_ready_retired_page(void) {
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
  hz10_public_entry_reclaim_thread_idle_pages(&reclaim_stats);
  if (reclaim_stats.pages_deferred_ready == 0u) {
    fprintf(stderr,
            "thread_reclaim_ready: expected at least one ready-stacked "
            "retired page to be deferred\n");
    return 1;
  }
  return 0;
}

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
  if (check_thread_reclaim_defers_ready_retired_page()) {
    return 8;
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

  puts("hz10_public_entry_smoke ok");
  return 0;
}
