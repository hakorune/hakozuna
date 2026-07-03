#include "../src/hz10_freelist_page.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_remote_stack.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HZ10_SMOKE_SLOT_SIZE 64u
#define HZ10_SMOKE_SLOT_COUNT 1024u
#define HZ10_SMOKE_STRESS_THREADS 8u

/* Case 1: basic push/drain. A remote free must not be visible in
 * local_free_head until drained. */
static int check_basic_push_drain(void) {
  Hz10FreelistPage* page =
      hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, 4u);
  if (!page) {
    fprintf(stderr, "basic: create failed\n");
    return 1;
  }
  void* p = hz10_freelist_page_alloc(page);
  if (!p) {
    fprintf(stderr, "basic: alloc failed\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }

  int failed = 0;
  if (!hz10_page_remote_free(page, p, page->generation)) {
    fprintf(stderr, "basic: remote_free rejected a valid pointer\n");
    failed = 1;
  }
  /* Not yet drained: local alloc must not see it (page had 4 slots, 1
   * taken, so 3 remain locally free -- p itself must not come back yet). */
  void* other[3];
  for (uint32_t i = 0; i < 3u; ++i) {
    other[i] = hz10_freelist_page_alloc(page);
    if (other[i] == p) {
      fprintf(stderr, "basic: remote-freed slot visible before drain\n");
      failed = 1;
    }
  }
  if (hz10_freelist_page_alloc(page) != NULL) {
    fprintf(stderr, "basic: page not empty before drain as expected\n");
    failed = 1;
  }

  uint32_t merged = hz10_page_drain_remote(page);
  if (merged != 1u) {
    fprintf(stderr, "basic: drain merged %u, expected 1\n", merged);
    failed = 1;
  }
  void* recovered = hz10_freelist_page_alloc(page);
  if (recovered != p) {
    fprintf(stderr, "basic: drained slot did not come back as the same ptr\n");
    failed = 1;
  }

  hz10_freelist_page_destroy(page);
  return failed;
}

/* Case 2: duplicate remote free of the same pointer is rejected before it
 * drains, so the slot is recovered exactly once, never twice. Every slot
 * is allocated first so local_free_head is genuinely empty going in --
 * otherwise a still-free sibling slot would mask a real double-drain. */
static int check_duplicate_rejected(void) {
  Hz10FreelistPage* page =
      hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, 4u);
  if (!page) {
    fprintf(stderr, "duplicate: create failed\n");
    return 1;
  }
  void* p = hz10_freelist_page_alloc(page);
  for (uint32_t i = 0; i < 3u; ++i) {
    if (!hz10_freelist_page_alloc(page)) {
      fprintf(stderr, "duplicate: setup alloc failed\n");
      hz10_freelist_page_destroy(page);
      return 1;
    }
  }
  int failed = 0;
  if (!hz10_page_remote_free(page, p, page->generation)) {
    fprintf(stderr, "duplicate: first remote_free unexpectedly rejected\n");
    failed = 1;
  }
  if (hz10_page_remote_free(page, p, page->generation)) {
    fprintf(stderr, "duplicate: second remote_free of same ptr accepted\n");
    failed = 1;
  }
  if (atomic_load_explicit(&page->remote_duplicate_count,
                          memory_order_relaxed) != 1u) {
    fprintf(stderr, "duplicate: remote_duplicate_count not incremented\n");
    failed = 1;
  }

  uint32_t merged = hz10_page_drain_remote(page);
  if (merged != 1u) {
    fprintf(stderr, "duplicate: drain merged %u, expected exactly 1\n",
            merged);
    failed = 1;
  }
  /* Only one of the 4 slots should now be free (the other 3 were never
   * allocated in this test, so this also implicitly checks no phantom
   * extra slot was created). */
  void* a = hz10_freelist_page_alloc(page);
  void* b = hz10_freelist_page_alloc(page);
  if (a != p) {
    fprintf(stderr, "duplicate: recovered slot mismatch\n");
    failed = 1;
  }
  if (b != NULL) {
    fprintf(stderr,
            "duplicate: a second slot came back -- slot was drained twice\n");
    failed = 1;
  }

  hz10_freelist_page_destroy(page);
  return failed;
}

/* Case 3: stale generation and out-of-range/misaligned/interior pointers
 * are rejected immediately, counted as invalid, and never pushed. */
static int check_stale_and_invalid_rejected(void) {
  Hz10FreelistPage* page =
      hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, 4u);
  if (!page) {
    fprintf(stderr, "stale: create failed\n");
    return 1;
  }
  void* p = hz10_freelist_page_alloc(page);
  uint32_t stale_gen = page->generation;

  hz10_freelist_page_free(page, p); /* local free so create() below is
                                      * driving a clean re-register */
  hz10_freelist_page_destroy(page);

  page = hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, 4u);
  if (!page) {
    fprintf(stderr, "stale: re-create failed\n");
    return 1;
  }

  int failed = 0;
  if (hz10_page_remote_free(page, page->base, stale_gen)) {
    fprintf(stderr, "stale: remote_free accepted a stale generation\n");
    failed = 1;
  }

  void* misaligned = (char*)page->base + 8u;    /* not 16-aligned */
  void* interior = (char*)page->base + 16u;      /* aligned, not a slot */
  void* out_of_range =
      (char*)page->base + (size_t)page->slot_size * page->slot_count;
  if (hz10_page_remote_free(page, misaligned, page->generation) ||
      hz10_page_remote_free(page, interior, page->generation) ||
      hz10_page_remote_free(page, out_of_range, page->generation)) {
    fprintf(stderr, "stale: an invalid pointer was accepted\n");
    failed = 1;
  }
  if (atomic_load_explicit(&page->remote_invalid_count, memory_order_relaxed) !=
      4u) {
    fprintf(stderr, "stale: remote_invalid_count expected 4, got %llu\n",
            (unsigned long long)atomic_load_explicit(
                &page->remote_invalid_count, memory_order_relaxed));
    failed = 1;
  }
  if (hz10_page_drain_remote(page) != 0u) {
    fprintf(stderr, "stale: drain recovered a slot that should never have "
                    "been pushed\n");
    failed = 1;
  }

  hz10_freelist_page_destroy(page);
  return failed;
}

/* Case 4: a foreign free of a slot that is already on the owner local
 * freelist is rejected before Treiber push. Otherwise the remote push would
 * overwrite the same first word the local freelist uses for its next pointer. */
static int check_remote_double_free_rejected_before_push(void) {
  Hz10FreelistPage* page =
      hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, 4u);
  if (!page) {
    fprintf(stderr, "drain-dup: create failed\n");
    return 1;
  }
  void* p = hz10_freelist_page_alloc(page);
  if (!p) {
    fprintf(stderr, "drain-dup: setup alloc failed\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }
  hz10_freelist_page_free(page, p);

  int failed = 0;
  if (hz10_page_remote_free(page, p, page->generation)) {
    fprintf(stderr, "drain-dup: remote publish accepted local-free slot\n");
    failed = 1;
  }
  uint32_t before = page->free_count;
  uint32_t merged = hz10_page_drain_remote(page);
  if (merged != 0u) {
    fprintf(stderr, "drain-dup: merged duplicate local-free slot\n");
    failed = 1;
  }
  if (page->free_count != before) {
    fprintf(stderr, "drain-dup: free_count changed on duplicate rejection\n");
    failed = 1;
  }
  if (page->drain_invalid_count != 0u) {
    fprintf(stderr, "drain-dup: drain_invalid_count=%llu, expected 0\n",
            (unsigned long long)page->drain_invalid_count);
    failed = 1;
  }
  if (atomic_load_explicit(&page->remote_invalid_count,
                           memory_order_relaxed) != 1u) {
    fprintf(stderr, "drain-dup: remote_invalid_count expected 1\n");
    failed = 1;
  }

  hz10_freelist_page_destroy(page);
  return failed;
}

typedef struct StressArg {
  Hz10FreelistPage* page;
  void** slots;
  uint32_t begin;
  uint32_t end;
} StressArg;

static void* stress_worker(void* raw) {
  StressArg* arg = (StressArg*)raw;
  for (uint32_t i = arg->begin; i < arg->end; ++i) {
    if (!hz10_page_remote_free(arg->page, arg->slots[i],
                              arg->page->generation)) {
      fprintf(stderr, "stress: thread rejected a fresh valid remote free\n");
      return (void*)(intptr_t)1;
    }
  }
  return NULL;
}

/* Case 5: concurrent stress. HZ10_SMOKE_STRESS_THREADS threads each remote
 * free a disjoint slice of already-allocated slots. This is the actual
 * proof that the Treiber stack never loses a push under contention -- the
 * duplicate case above already covers the pending-bit dedup logic
 * single-threaded, so this test uses disjoint slots per thread to isolate
 * concurrent-push correctness specifically. */
static int check_concurrent_stress(void) {
  Hz10FreelistPage* page = hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE,
                                                    HZ10_SMOKE_SLOT_COUNT);
  if (!page) {
    fprintf(stderr, "stress: create failed\n");
    return 1;
  }

  void** original = calloc(HZ10_SMOKE_SLOT_COUNT, sizeof(*original));
  if (!original) {
    fprintf(stderr, "stress: allocation failure\n");
    hz10_freelist_page_destroy(page);
    return 1;
  }
  for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT; ++i) {
    original[i] = hz10_freelist_page_alloc(page);
    if (!original[i]) {
      fprintf(stderr, "stress: setup alloc %u failed\n", i);
      free(original);
      hz10_freelist_page_destroy(page);
      return 1;
    }
  }

  pthread_t threads[HZ10_SMOKE_STRESS_THREADS];
  StressArg args[HZ10_SMOKE_STRESS_THREADS];
  uint32_t per_thread = HZ10_SMOKE_SLOT_COUNT / HZ10_SMOKE_STRESS_THREADS;
  int failed = 0;
  for (uint32_t t = 0; t < HZ10_SMOKE_STRESS_THREADS; ++t) {
    args[t].page = page;
    args[t].slots = original;
    args[t].begin = t * per_thread;
    args[t].end = (t + 1u == HZ10_SMOKE_STRESS_THREADS)
                     ? HZ10_SMOKE_SLOT_COUNT
                     : (t + 1u) * per_thread;
    if (pthread_create(&threads[t], NULL, stress_worker, &args[t]) != 0) {
      fprintf(stderr, "stress: pthread_create failed\n");
      failed = 1;
    }
  }
  for (uint32_t t = 0; t < HZ10_SMOKE_STRESS_THREADS; ++t) {
    void* ret = NULL;
    pthread_join(threads[t], &ret);
    if (ret != NULL) {
      failed = 1;
    }
  }

  uint32_t merged = hz10_page_drain_remote(page);
  if (merged != HZ10_SMOKE_SLOT_COUNT) {
    fprintf(stderr,
            "stress: drain merged %u, expected all %u (a push was lost)\n",
            merged, HZ10_SMOKE_SLOT_COUNT);
    failed = 1;
  }

  int* recovered_seen = calloc(HZ10_SMOKE_SLOT_COUNT, sizeof(*recovered_seen));
  if (!recovered_seen) {
    fprintf(stderr, "stress: allocation failure\n");
    failed = 1;
  } else {
    for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT && !failed; ++i) {
      void* p = hz10_freelist_page_alloc(page);
      if (!p) {
        fprintf(stderr, "stress: re-alloc %u returned NULL\n", i);
        failed = 1;
        break;
      }
      int matched = -1;
      for (uint32_t j = 0; j < HZ10_SMOKE_SLOT_COUNT; ++j) {
        if (original[j] == p) {
          matched = (int)j;
          break;
        }
      }
      if (matched < 0 || recovered_seen[matched]) {
        fprintf(stderr, "stress: re-alloc %u is unknown or duplicate\n", i);
        failed = 1;
        break;
      }
      recovered_seen[matched] = 1;
    }
    free(recovered_seen);
  }

  free(original);
  hz10_freelist_page_destroy(page);
  return failed;
}

int main(void) {
  hz10_pagemap_reset_for_tests();

  if (check_basic_push_drain()) {
    return 2;
  }
  if (check_duplicate_rejected()) {
    return 3;
  }
  if (check_stale_and_invalid_rejected()) {
    return 4;
  }
  if (check_remote_double_free_rejected_before_push()) {
    return 5;
  }
  if (check_concurrent_stress()) {
    return 6;
  }

  puts("hz10_remote_stack_drain_smoke ok");
  return 0;
}
