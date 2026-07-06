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

#if HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION
typedef struct PartialAdoptState {
  void* live;
  void* freed;
  void* adopted;
  void* adopted_page;
  pthread_barrier_t barrier;
  int failed;
} PartialAdoptState;

static void* alloc_partial_orphan_thread(void* arg) {
  PartialAdoptState* state = (PartialAdoptState*)arg;
  state->live = hz10_malloc(384);
  state->freed = hz10_malloc(384);
  if (!state->live || !state->freed || !hz10_free(state->freed)) {
    state->failed = 1;
  }
  return NULL;
}

static void* adopt_partial_and_hold_thread(void* arg) {
  PartialAdoptState* state = (PartialAdoptState*)arg;
  state->adopted = hz10_malloc(384);
  if (!state->adopted) {
    state->failed = 1;
    (void)pthread_barrier_wait(&state->barrier);
    (void)pthread_barrier_wait(&state->barrier);
    return NULL;
  }
  H10RouteResult route =
      hz10_pagemap_route(state->adopted, HZ10_GENERATION_ANY);
  if (route.kind != H10_ROUTE_VALID || !route.owner) {
    state->failed = 1;
  } else {
    state->adopted_page = route.owner;
  }
  (void)pthread_barrier_wait(&state->barrier);
  (void)pthread_barrier_wait(&state->barrier);
  return NULL;
}

static void* free_partial_live_remote_thread(void* arg) {
  PartialAdoptState* state = (PartialAdoptState*)arg;
  if (!hz10_free(state->live)) {
    state->failed = 1;
  }
  return NULL;
}

static void* adopt_partial_and_free_held_thread(void* arg) {
  PartialAdoptState* state = (PartialAdoptState*)arg;
  state->adopted = hz10_malloc(384);
  if (!state->adopted || !hz10_free(state->live)) {
    state->failed = 1;
  }
  return NULL;
}

static int setup_partial_orphan(PartialAdoptState* state) {
  *state = (PartialAdoptState){0};
  pthread_t owner_thread;
  if (pthread_create(&owner_thread, NULL, alloc_partial_orphan_thread,
                     state) ||
      pthread_join(owner_thread, NULL) || state->failed || !state->live ||
      !state->freed) {
    fprintf(stderr, "partial_orphan: owner setup failed\n");
    return 1;
  }
  return 0;
}

static int check_partial_orphan_adoption_reuses_capacity(void) {
  PartialAdoptState state;
  if (setup_partial_orphan(&state)) return 1;

  H10RouteResult before = hz10_pagemap_route(state.live, HZ10_GENERATION_ANY);
  if (before.kind != H10_ROUTE_VALID || !before.owner) {
    fprintf(stderr, "partial_orphan: live route failed\n");
    return 1;
  }

  void* adopted = hz10_malloc(384);
  if (!adopted) {
    fprintf(stderr, "partial_orphan: adopter malloc failed\n");
    return 1;
  }
  H10RouteResult after = hz10_pagemap_route(adopted, HZ10_GENERATION_ANY);
  if (after.kind != H10_ROUTE_VALID || after.owner != before.owner) {
    fprintf(stderr, "partial_orphan: expected same page adoption\n");
    return 1;
  }
  return 0;
}

static int check_partial_orphan_third_thread_free(void) {
  PartialAdoptState state;
  if (setup_partial_orphan(&state)) return 1;
  H10RouteResult before = hz10_pagemap_route(state.live, HZ10_GENERATION_ANY);
  if (before.kind != H10_ROUTE_VALID || !before.owner) {
    fprintf(stderr, "partial_orphan_remote: live route failed\n");
    return 1;
  }
  if (pthread_barrier_init(&state.barrier, NULL, 2u)) {
    fprintf(stderr, "partial_orphan_remote: barrier init failed\n");
    return 1;
  }

  pthread_t adopter_thread;
  if (pthread_create(&adopter_thread, NULL, adopt_partial_and_hold_thread,
                     &state)) {
    pthread_barrier_destroy(&state.barrier);
    fprintf(stderr, "partial_orphan_remote: adopter start failed\n");
    return 1;
  }
  (void)pthread_barrier_wait(&state.barrier);
  if (state.failed || state.adopted_page != before.owner) {
    (void)pthread_barrier_wait(&state.barrier);
    (void)pthread_join(adopter_thread, NULL);
    pthread_barrier_destroy(&state.barrier);
    fprintf(stderr, "partial_orphan_remote: adoption failed\n");
    return 1;
  }

  pthread_t freer_thread;
  if (pthread_create(&freer_thread, NULL, free_partial_live_remote_thread,
                     &state) ||
      pthread_join(freer_thread, NULL) || state.failed) {
    (void)pthread_barrier_wait(&state.barrier);
    (void)pthread_join(adopter_thread, NULL);
    pthread_barrier_destroy(&state.barrier);
    fprintf(stderr, "partial_orphan_remote: third-thread free failed\n");
    return 1;
  }
  (void)pthread_barrier_wait(&state.barrier);
  if (pthread_join(adopter_thread, NULL)) {
    pthread_barrier_destroy(&state.barrier);
    fprintf(stderr, "partial_orphan_remote: adopter join failed\n");
    return 1;
  }
  pthread_barrier_destroy(&state.barrier);
  return 0;
}

static int check_partial_orphan_adopter_frees_held_object(void) {
  PartialAdoptState state;
  if (setup_partial_orphan(&state)) return 1;
  pthread_t adopter_thread;
  if (pthread_create(&adopter_thread, NULL, adopt_partial_and_free_held_thread,
                     &state) ||
      pthread_join(adopter_thread, NULL) || state.failed) {
    fprintf(stderr, "partial_orphan_held: local free after adopt failed\n");
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
#if HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION
  if (check_partial_orphan_adoption_reuses_capacity()) {
    return 4;
  }
  if (check_partial_orphan_third_thread_free()) {
    return 5;
  }
  if (check_partial_orphan_adopter_frees_held_object()) {
    return 6;
  }
#endif
  puts("hz10_public_entry_owner_smoke ok");
  return 0;
}
