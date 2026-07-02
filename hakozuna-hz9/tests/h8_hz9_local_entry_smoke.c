#include "../include/h8.h"

#include <stdio.h>
#include <string.h>

enum {
  H9_LOCAL_ENTRY_TEST_FREE_ARENA_SKIP = 5
};

static int check_entry_stats(void) {
  H8DebugStats stats = h8_debug_stats();
  size_t* event = stats.h9_local_entry_event;

  if (event[0] == 0 || event[1] != event[0]) {
    fprintf(stderr, "bad local-entry malloc counters: %zu %zu\n", event[0],
            event[1]);
    return 1;
  }
  if (event[2] == 0 || event[3] != event[2] || event[4] != 0) {
    fprintf(stderr, "bad local-entry free counters: %zu %zu %zu %zu\n",
            event[2], event[3], event[4], event[5]);
    return 1;
  }
  return 0;
}

static int check_arena_skip(size_t before) {
  enum { N = 32 };
  void* ptrs[N];
  for (size_t i = 0; i < N; ++i) {
    size_t size = 16u + ((i * 31u) % 2048u);
    ptrs[i] = h8_malloc(size);
    if (!ptrs[i]) {
      fprintf(stderr, "small h8_malloc failed at %zu\n", i);
      return 1;
    }
    memset(ptrs[i], (int)(0xa0u + i), size);
  }
  for (size_t i = 0; i < N; ++i) {
    h8_free(ptrs[i]);
  }

  H8DebugStats stats = h8_debug_stats();
  size_t after =
      stats.h9_local_entry_event[H9_LOCAL_ENTRY_TEST_FREE_ARENA_SKIP];
  if (after <= before) {
    fprintf(stderr, "arena skip did not increase: %zu -> %zu\n", before,
            after);
    return 1;
  }
  return 0;
}

int main(void) {
  enum { N = 128 };
  void* ptrs[N];
  for (size_t i = 0; i < N; ++i) {
    size_t size = 4097u + ((i * 997u) % (65536u - 4097u));
    ptrs[i] = h8_malloc(size);
    if (!ptrs[i]) {
      fprintf(stderr, "h8_malloc failed at %zu\n", i);
      return 1;
    }
    memset(ptrs[i], (int)(i & 0xffu), size);
  }
  for (size_t i = 0; i < N; ++i) {
    h8_free(ptrs[i]);
  }
  if (check_entry_stats() != 0) {
    return 1;
  }
  H8DebugStats stats = h8_debug_stats();
  if (check_arena_skip(stats.h9_local_entry_event
                           [H9_LOCAL_ENTRY_TEST_FREE_ARENA_SKIP]) != 0) {
    return 1;
  }
  puts("hz9_local_entry_smoke ok");
  return 0;
}
