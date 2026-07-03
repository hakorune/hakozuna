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

/* Case 2: rejected inputs -- oversized, zero, NULL, and genuinely unknown
 * pointers, per hz10_public_entry.h's stated L0 scope. */
static int check_rejected_inputs(void) {
  int failed = 0;
  if (hz10_malloc(0) != NULL) {
    fprintf(stderr, "rejected: malloc(0) should return NULL\n");
    failed = 1;
  }
  if (hz10_malloc((size_t)HZ10_PAGE_QUANTUM + 1u) != NULL) {
    fprintf(stderr, "rejected: malloc(quantum+1) should return NULL\n");
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
  if (check_cross_thread_free()) {
    return 6;
  }

  puts("hz10_public_entry_smoke ok");
  return 0;
}
