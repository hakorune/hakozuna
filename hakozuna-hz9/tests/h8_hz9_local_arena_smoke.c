#include "../include/h8.h"

#include <stdio.h>
#include <string.h>

enum {
  H9_LOCAL_ENTRY_ARENA_ALLOC_HIT_TEST = 6,
  H9_LOCAL_ENTRY_ARENA_PAGE_CREATE_TEST = 7,
  H9_LOCAL_ENTRY_ARENA_FREE_VALID_TEST = 9,
  H9_LOCAL_ENTRY_ARENA_FREE_INVALID_TEST = 10
};

static int check_debug_counters(void) {
  H8DebugStats stats = h8_debug_stats();
  size_t page_create = 0;
  size_t create_before_remote = 0;
  size_t create_after_remote = 0;
  size_t alloc_hit = 0;
  size_t free_local = 0;
  size_t class_remote_first = 0;
  size_t remote_first_age = 0;
  size_t create_thread_seq = 0;
  size_t remote_thread_seq = 0;
  size_t create_alloc_history = 0;
  size_t remote_alloc_history = 0;
  size_t remote_first_local_free = 0;
  size_t remote_after_local = 0;
  size_t local_after_remote = 0;
  for (size_t i = 0; i < 6u; ++i) {
    page_create += stats.h9_local_arena_page_create_class[i];
    create_before_remote +=
        stats.h9_local_arena_admit_create_before_remote_class[i];
    create_after_remote +=
        stats.h9_local_arena_admit_create_after_remote_class[i];
    alloc_hit += stats.h9_local_arena_alloc_hit_class[i];
    free_local += stats.h9_local_arena_free_local_class[i];
    remote_after_local += stats.h9_local_arena_remote_free_after_local_class[i];
    local_after_remote += stats.h9_local_arena_local_free_after_remote_class[i];
    class_remote_first += stats.h9_local_arena_admit_remote_class_first[i];
  }
  for (size_t i = 0; i < 5u; ++i) {
    remote_first_age += stats.h9_local_arena_remote_first_alloc_age[i];
    remote_first_local_free +=
        stats.h9_local_arena_remote_first_local_free_history[i];
    create_thread_seq += stats.h9_local_arena_create_thread_page_seq[i];
    remote_thread_seq += stats.h9_local_arena_remote_first_thread_page_seq[i];
    create_alloc_history +=
        stats.h9_local_arena_create_thread_alloc_history[i];
    remote_alloc_history +=
        stats.h9_local_arena_remote_first_thread_alloc_history[i];
  }
  if (stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_HIT_TEST] == 0 ||
      stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_PAGE_CREATE_TEST] == 0 ||
      stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_VALID_TEST] == 0) {
    fprintf(stderr,
            "local arena counters missing: alloc=%zu page=%zu free=%zu\n",
            stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_HIT_TEST],
            stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_PAGE_CREATE_TEST],
            stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_VALID_TEST]);
    return 1;
  }
  if (stats.h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_INVALID_TEST] == 0) {
    fprintf(stderr, "local arena invalid counter did not increase\n");
    return 1;
  }
  if (page_create == 0 || alloc_hit == 0 || free_local == 0) {
    fprintf(stderr,
            "local arena class counters missing: page=%zu alloc=%zu free=%zu\n",
            page_create, alloc_hit, free_local);
    return 1;
  }
  if (create_before_remote != page_create || create_after_remote != 0 ||
      class_remote_first != 0 || remote_first_age != 0 ||
      create_thread_seq != page_create || remote_thread_seq != 0 ||
      create_alloc_history != page_create || remote_alloc_history != 0 ||
      remote_first_local_free != 0 || remote_after_local != 0 ||
      local_after_remote != 0) {
    fprintf(stderr,
            "local arena admission counters bad: before=%zu after=%zu "
            "page=%zu remote_first=%zu age=%zu thread_seq=%zu/%zu "
            "alloc_history=%zu/%zu mix=%zu/%zu/%zu\n",
            create_before_remote, create_after_remote, page_create,
            class_remote_first, remote_first_age, create_thread_seq,
            remote_thread_seq, create_alloc_history, remote_alloc_history,
            remote_first_local_free, remote_after_local, local_after_remote);
    return 1;
  }
  return 0;
}

int main(void) {
  void* a = h8_malloc(8192u);
  if (!a || h8_route(a) != H8_ROUTE_VALID ||
      h8_route((char*)a + 1) != H8_ROUTE_INVALID) {
    fprintf(stderr, "local arena route failed\n");
    return 1;
  }
  memset(a, 0x5a, 8192u);

  void* b = h8_realloc(a, 12000u);
  if (!b || h8_route(b) != H8_ROUTE_VALID) {
    fprintf(stderr, "local arena realloc failed\n");
    return 1;
  }
  h8_free(b);
  if (h8_route(b) == H8_ROUTE_VALID) {
    fprintf(stderr, "local arena free left route valid\n");
    return 1;
  }

  h8_free(b);
  if (check_debug_counters() != 0) {
    return 1;
  }
  puts("hz9_local_arena_smoke ok");
  return 0;
}
