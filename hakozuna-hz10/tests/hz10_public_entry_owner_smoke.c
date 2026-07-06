#include "../src/hz10_pagemap.h"
#include "../src/hz10_public_entry.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

static void* alloc_one_slot_thread(void* arg) {
  *(void**)arg = hz10_malloc(65536);
  return NULL;
}

static void* free_in_other_thread(void* arg) {
  void* ptr = *(void**)arg;
  return (void*)(intptr_t)hz10_free(ptr);
}

#if HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
static void* alloc_free_one_slot_thread(void* arg) {
  void* ptr = hz10_malloc(65536);
  if (!ptr || !hz10_free(ptr)) {
    *(void**)arg = NULL;
    return NULL;
  }
  *(void**)arg = ptr;
  return NULL;
}

static void* alloc_one_slot_compare_thread(void* arg) {
  void** ptrs = (void**)arg;
  ptrs[1] = hz10_malloc(65536);
  return NULL;
}

static int check_orphan_active_adoption_reuses_page(void) {
  void* ptrs[2] = {NULL, NULL};
  pthread_t owner_thread;
  if (pthread_create(&owner_thread, NULL, alloc_free_one_slot_thread,
                     &ptrs[0]) ||
      pthread_join(owner_thread, NULL) || !ptrs[0]) {
    fprintf(stderr, "orphan_adopt: owner setup failed\n");
    return 1;
  }

  pthread_t adopter_thread;
  if (pthread_create(&adopter_thread, NULL, alloc_one_slot_compare_thread,
                     ptrs) ||
      pthread_join(adopter_thread, NULL) || !ptrs[1]) {
    fprintf(stderr, "orphan_adopt: adopter malloc failed\n");
    return 1;
  }
  if (ptrs[1] != ptrs[0]) {
    fprintf(stderr, "orphan_adopt: expected orphan slot reuse\n");
    return 1;
  }
  return 0;
}
#endif

static int check_exited_owner_token_not_reused_as_local(void) {
  for (int i = 0; i < 16; ++i) {
    void* ptr = NULL;
    pthread_t owner_thread;
    if (pthread_create(&owner_thread, NULL, alloc_one_slot_thread, &ptr) ||
        pthread_join(owner_thread, NULL) || !ptr) {
      fprintf(stderr, "owner_token_aba: owner thread failed\n");
      return 1;
    }

    H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
    if (route.kind != H10_ROUTE_VALID || !route.owner) {
      fprintf(stderr, "owner_token_aba: route failed\n");
      return 1;
    }
    Hz10FreelistPage* page = (Hz10FreelistPage*)route.owner;
    uint32_t free_count_before = page->free_count;

    pthread_t freer_thread;
    void* ret = NULL;
    if (pthread_create(&freer_thread, NULL, free_in_other_thread, &ptr) ||
        pthread_join(freer_thread, &ret) || (intptr_t)ret != 1) {
      fprintf(stderr, "owner_token_aba: remote free failed\n");
      return 1;
    }
    if (page->free_count != free_count_before) {
      fprintf(stderr,
              "owner_token_aba: exited-owner free merged locally "
              "(free_count %u -> %u)\n",
              free_count_before, page->free_count);
      return 1;
    }
  }
  return 0;
}

int main(void) {
  hz10_pagemap_reset_for_tests();
  if (check_exited_owner_token_not_reused_as_local()) {
    return 2;
  }
#if HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
  if (check_orphan_active_adoption_reuses_page()) {
    return 3;
  }
#endif
  puts("hz10_public_entry_owner_smoke ok");
  return 0;
}
