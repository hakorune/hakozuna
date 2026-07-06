#include "h8_internal.h"

#include <string.h>
#include <stdlib.h>

#ifdef H8_BUILD_LD_PRELOAD
__attribute__((visibility("default"))) void* malloc(size_t size) {
  return h8_malloc_inner(size);
}

__attribute__((visibility("default"))) void* calloc(size_t count, size_t size) {
  return h8_calloc(count, size);
}

__attribute__((visibility("default"))) void* realloc(void* ptr, size_t size) {
  return h8_realloc_inner(ptr, size);
}

__attribute__((visibility("default"))) void free(void* ptr) {
  h8_free_inner(ptr);
}

/* HZ8_DUMP_STATS: opt-in, default-off RSS/attribution dump at process exit.
 * Behavior-neutral when the env is unset (no atexit registered). Reads only
 * fields that already exist in the H8Stats snapshot -- no new state. Used by
 * the macro-failure attribution to locate where committed bytes live. */
#include <stdio.h>
static void hz8_dump_stats_atexit(void) {
  H8Stats s = h8_stats();
  fprintf(stderr,
          "[hz8 stats] arena_reserved=%zu arena_committed=%zu small_spans=%zu "
          "owners=%zu orphan_spans=%zu owner_exit=%zu "
          "local_alloc=%zu local_free=%zu remote_publish=%zu remote_collect=%zu "
          "direct_large_live=%zu\n",
          s.arena_reserved_bytes, s.arena_committed_bytes, s.small_span_count,
          s.owner_count, s.orphan_span_count, s.owner_exit_count,
          s.local_alloc_count, s.local_free_count, s.remote_publish_count,
          s.remote_collect_count, s.direct_large_live_bytes);
}

__attribute__((constructor)) static void hz8_dump_stats_init(void) {
  if (getenv("HZ8_DUMP_STATS") != NULL) {
    atexit(hz8_dump_stats_atexit);
  }
}
#endif

void* h8_malloc(size_t size) {
  return h8_malloc_inner(size);
}

void* h8_calloc(size_t count, size_t size) {
  if (count != 0 && size > SIZE_MAX / count) {
    return NULL;
  }
  size_t total = count * size;
  void* ptr = h8_malloc_inner(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

void* h8_realloc(void* ptr, size_t size) {
  return h8_realloc_inner(ptr, size);
}

void h8_free(void* ptr) {
  h8_free_inner(ptr);
}
