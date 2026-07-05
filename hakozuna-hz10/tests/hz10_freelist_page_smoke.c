#include "../src/hz10_freelist_page.h"
#include "../src/hz10_pagemap.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HZ10_SMOKE_SLOT_SIZE 64u
#define HZ10_SMOKE_SLOT_COUNT 16u

/* Case 1: page exhaustion. alloc() returns slot_count distinct non-NULL
 * pointers, then NULL once the page is empty. */
static int check_exhaustion(Hz10FreelistPage* page) {
  void* seen[HZ10_SMOKE_SLOT_COUNT];
  for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT; ++i) {
    void* p = hz10_freelist_page_alloc(page);
    if (!p) {
      fprintf(stderr, "exhaustion: alloc %u returned NULL early\n", i);
      return 1;
    }
    for (uint32_t j = 0; j < i; ++j) {
      if (seen[j] == p) {
        fprintf(stderr, "exhaustion: alloc %u repeated a live pointer\n", i);
        return 1;
      }
    }
    seen[i] = p;
  }
  if (hz10_freelist_page_alloc(page) != NULL) {
    fprintf(stderr, "exhaustion: alloc past slot_count did not return NULL\n");
    return 1;
  }
  /* free everything back for the next case */
  for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT; ++i) {
    hz10_freelist_page_free(page, seen[i]);
  }
  return 0;
}

/* Case 2: LIFO reuse. The most recently freed slot is the next allocated. */
static int check_lifo_reuse(Hz10FreelistPage* page) {
  void* a = hz10_freelist_page_alloc(page);
  void* b = hz10_freelist_page_alloc(page);
  if (!a || !b) {
    fprintf(stderr, "lifo: setup alloc failed\n");
    return 1;
  }
  hz10_freelist_page_free(page, a);
  hz10_freelist_page_free(page, b);
  /* b was freed last, so it must come back first. */
  void* first = hz10_freelist_page_alloc(page);
  void* second = hz10_freelist_page_alloc(page);
  int failed = 0;
  if (first != b) {
    fprintf(stderr, "lifo: expected most-recently-freed slot first\n");
    failed = 1;
  }
  if (second != a) {
    fprintf(stderr, "lifo: expected second-most-recently-freed slot next\n");
    failed = 1;
  }
  hz10_freelist_page_free(page, first);
  hz10_freelist_page_free(page, second);
  return failed;
}

/* Case 3: non-LIFO reuse. alloc every slot, free them in a shuffled (not
 * reverse-of-alloc) order, then re-alloc every slot and verify the
 * recovered set matches the original set exactly -- no duplicates, no
 * drops. A depth-1 last-pointer cache (HZ9 ProductEntry's mistake) would
 * fail this the moment more than one slot is live at a time. */
static int check_non_lifo_reuse(Hz10FreelistPage* page) {
  void* original[HZ10_SMOKE_SLOT_COUNT];
  for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT; ++i) {
    original[i] = hz10_freelist_page_alloc(page);
    if (!original[i]) {
      fprintf(stderr, "non_lifo: setup alloc %u failed\n", i);
      return 1;
    }
  }

  /* Free in a fixed, deliberately non-reverse permutation. */
  static const uint32_t free_order[HZ10_SMOKE_SLOT_COUNT] = {
      3, 7, 0, 15, 1, 14, 2, 13, 4, 12, 5, 11, 6, 10, 8, 9};
  for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT; ++i) {
    hz10_freelist_page_free(page, original[free_order[i]]);
  }

  int recovered_seen[HZ10_SMOKE_SLOT_COUNT];
  memset(recovered_seen, 0, sizeof(recovered_seen));
  int failed = 0;
  for (uint32_t i = 0; i < HZ10_SMOKE_SLOT_COUNT; ++i) {
    void* p = hz10_freelist_page_alloc(page);
    if (!p) {
      fprintf(stderr, "non_lifo: re-alloc %u returned NULL\n", i);
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
    if (matched < 0) {
      fprintf(stderr, "non_lifo: re-alloc %u is not a slot this page owns\n",
              i);
      failed = 1;
      break;
    }
    if (recovered_seen[matched]) {
      fprintf(stderr, "non_lifo: slot %d recovered twice (duplicate)\n",
              matched);
      failed = 1;
      break;
    }
    recovered_seen[matched] = 1;
  }
  if (!failed) {
    for (uint32_t j = 0; j < HZ10_SMOKE_SLOT_COUNT; ++j) {
      if (!recovered_seen[j]) {
        fprintf(stderr, "non_lifo: slot %u never recovered (dropped)\n", j);
        failed = 1;
      }
    }
  }
  return failed;
}

/* Case 4: pagemap integration. create() must publish to
 * HZ10PageMapRoute-L0 exactly at the boundary (not per-op); destroy() must
 * release it. */
static int check_pagemap_integration(void) {
  Hz10FreelistPage* page =
      hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, HZ10_SMOKE_SLOT_COUNT);
  if (!page) {
    fprintf(stderr, "pagemap_integration: create failed\n");
    return 1;
  }

  H10RouteResult before = hz10_pagemap_route(page->base, page->generation);
  int failed = 0;
  if (before.kind != H10_ROUTE_VALID) {
    fprintf(stderr, "pagemap_integration: route not VALID after create\n");
    failed = 1;
  }

  void* base = page->base;
  uint32_t generation = page->generation;
  hz10_freelist_page_destroy(page);

  H10RouteResult after = hz10_pagemap_route(base, generation);
  if (after.kind == H10_ROUTE_VALID) {
    fprintf(stderr, "pagemap_integration: route still VALID after destroy\n");
    failed = 1;
  }
  return failed;
}

/* Case 5: F2/D1 pending storage shape. One-word pending state is inline in
 * the page next to the remote heads; multi-word pending state stays outside
 * the public page struct, now backed by the private metadata slab. */
static int check_pending_storage_shape(void) {
  Hz10FreelistPage* small = hz10_freelist_page_create(1024u, 64u);
  Hz10FreelistPage* tiny_many = hz10_freelist_page_create(16u, 128u);
  Hz10FreelistPage* medium_many = hz10_freelist_page_create(128u, 128u);
  if (!small || !tiny_many || !medium_many) {
    fprintf(stderr, "pending_storage: create failed\n");
    hz10_freelist_page_destroy(small);
    hz10_freelist_page_destroy(tiny_many);
    hz10_freelist_page_destroy(medium_many);
    return 1;
  }

  int failed = 0;
  if (small->pending_words != 1u ||
      small->pending_bits != &small->pending_inline_word) {
    fprintf(stderr, "pending_storage: <=64 slots not using inline word\n");
    failed = 1;
  }
  if (small->remote_free_spread != NULL) {
    fprintf(stderr, "pending_storage: <=64 slots using spread stripes\n");
    failed = 1;
  }
  if (tiny_many->pending_words <= 1u ||
      tiny_many->pending_bits == &tiny_many->pending_inline_word ||
      medium_many->pending_words <= 1u ||
      medium_many->pending_bits == &medium_many->pending_inline_word) {
    fprintf(stderr, "pending_storage: >64 slots not using metadata words\n");
    failed = 1;
  }
#if HZ10_ENABLE_STRIPE_SPREAD
  if (tiny_many->remote_free_spread == NULL) {
    fprintf(stderr, "pending_storage: tiny >64 slots missing spread stripes\n");
    failed = 1;
  }
  if (medium_many->remote_free_spread != NULL) {
    fprintf(stderr, "pending_storage: medium >64 slots using spread stripes\n");
    failed = 1;
  }
#else
  if (tiny_many->remote_free_spread != NULL ||
      medium_many->remote_free_spread != NULL) {
    fprintf(stderr, "pending_storage: spread stripes enabled unexpectedly\n");
    failed = 1;
  }
#endif

  hz10_freelist_page_destroy(small);
  hz10_freelist_page_destroy(tiny_many);
  hz10_freelist_page_destroy(medium_many);
  return failed;
}

int main(void) {
  hz10_pagemap_reset_for_tests();

  Hz10FreelistPage* page =
      hz10_freelist_page_create(HZ10_SMOKE_SLOT_SIZE, HZ10_SMOKE_SLOT_COUNT);
  if (!page) {
    fprintf(stderr, "setup: create failed\n");
    return 1;
  }

  if (check_exhaustion(page)) {
    return 2;
  }
  if (check_lifo_reuse(page)) {
    return 3;
  }
  if (check_non_lifo_reuse(page)) {
    return 4;
  }
  hz10_freelist_page_destroy(page);

  if (check_pagemap_integration()) {
    return 5;
  }
  if (check_pending_storage_shape()) {
    return 6;
  }

  puts("hz10_freelist_page_smoke ok");
  return 0;
}
