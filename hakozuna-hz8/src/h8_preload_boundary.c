#include "h8_internal.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef H8_BUILD_LD_PRELOAD
#include <dlfcn.h>

typedef size_t (*H8RealMallocUsableSizeFn)(void*);
typedef int (*H8RealPosixMemalignFn)(void**, size_t, size_t);
typedef void* (*H8RealAlignedAllocFn)(size_t, size_t);
typedef void* (*H8RealMemalignFn)(size_t, size_t);

static H8RealMallocUsableSizeFn h8_real_malloc_usable_size_fn(void) {
  static H8RealMallocUsableSizeFn fn;
  static bool loaded;
  if (!loaded) {
    fn = (H8RealMallocUsableSizeFn)dlsym(RTLD_NEXT, "malloc_usable_size");
    loaded = true;
  }
  return fn;
}

static H8RealPosixMemalignFn h8_real_posix_memalign_fn(void) {
  static H8RealPosixMemalignFn fn;
  static bool loaded;
  if (!loaded) {
    fn = (H8RealPosixMemalignFn)dlsym(RTLD_NEXT, "posix_memalign");
    loaded = true;
  }
  return fn;
}

static H8RealAlignedAllocFn h8_real_aligned_alloc_fn(void) {
  static H8RealAlignedAllocFn fn;
  static bool loaded;
  if (!loaded) {
    fn = (H8RealAlignedAllocFn)dlsym(RTLD_NEXT, "aligned_alloc");
    loaded = true;
  }
  return fn;
}

static H8RealMemalignFn h8_real_memalign_fn(void) {
  static H8RealMemalignFn fn;
  static bool loaded;
  if (!loaded) {
    fn = (H8RealMemalignFn)dlsym(RTLD_NEXT, "memalign");
    loaded = true;
  }
  return fn;
}

static bool h8_is_power_of_two(size_t value) {
  return value != 0 && (value & (value - 1u)) == 0;
}

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

__attribute__((visibility("default"))) size_t malloc_usable_size(void* ptr) {
  size_t usable = 0;
  bool owned = false;
  if (h8_usable_size_inner(ptr, &usable, &owned)) {
    return usable;
  }
  if (owned) {
    return 0;
  }
  H8RealMallocUsableSizeFn next = h8_real_malloc_usable_size_fn();
  return next ? next(ptr) : 0;
}

__attribute__((visibility("default"))) int posix_memalign(void** memptr,
                                                          size_t alignment,
                                                          size_t size) {
  H8RealPosixMemalignFn next = h8_real_posix_memalign_fn();
  if (next) {
    return next(memptr, alignment, size);
  }
  if (alignment < sizeof(void*) || !h8_is_power_of_two(alignment)) {
    return EINVAL;
  }
  if (alignment > _Alignof(max_align_t)) {
    *memptr = NULL;
    return ENOMEM;
  }
  void* ptr = h8_malloc_inner(size);
  if (!ptr) {
    *memptr = NULL;
    return ENOMEM;
  }
  *memptr = ptr;
  return 0;
}

__attribute__((visibility("default"))) void* aligned_alloc(size_t alignment,
                                                           size_t size) {
  H8RealAlignedAllocFn next = h8_real_aligned_alloc_fn();
  if (next) {
    return next(alignment, size);
  }
  if (!h8_is_power_of_two(alignment) || alignment > _Alignof(max_align_t) ||
      (alignment != 0 && (size % alignment) != 0)) {
    errno = EINVAL;
    return NULL;
  }
  return h8_malloc_inner(size);
}

__attribute__((visibility("default"))) void* memalign(size_t alignment,
                                                      size_t size) {
  H8RealMemalignFn next = h8_real_memalign_fn();
  if (next) {
    return next(alignment, size);
  }
  if (!h8_is_power_of_two(alignment) || alignment > _Alignof(max_align_t)) {
    errno = EINVAL;
    return NULL;
  }
  return h8_malloc_inner(size);
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
