#include "hz11_public_entry.h"

#include <stdio.h>
#include <stdlib.h>

/* HZ11_DUMP_STATS=1: opt-in atexit dump of the calling thread's cache counters
 * (token hit/miss, malloc hit/refill, overflow/flush, cached_bytes). Used by the
 * gate to read the token hit rate on the bench workload. Behavior-neutral when
 * the env is unset (no atexit registered). */
static void hz11_dump_stats_atexit(void) {
  H11Stats s;
  hz11_stats(&s);
  fprintf(stderr,
          "hz11_shim_exit_stats malloc=%llu hit=%llu refill=%llu free=%llu "
          "token_hit=%llu token_miss=%llu overflow=%llu flush=%llu "
          "flush_items=%llu cached_bytes=%zu\n",
          (unsigned long long)s.malloc_count, (unsigned long long)s.malloc_hit,
          (unsigned long long)s.refill_count, (unsigned long long)s.free_count,
          (unsigned long long)s.token_hit, (unsigned long long)s.token_miss,
          (unsigned long long)s.overflow_count,
          (unsigned long long)s.flush_count,
          (unsigned long long)s.flush_items, s.cached_bytes);
}

/* LD_PRELOAD entry points. Export the full interposition surface so foreign
 * programs that call malloc_usable_size/posix_memalign/etc. do not split across
 * a different allocator (the HZ8/redis lesson). Only malloc/free use the cache;
 * the rest route to the system fallback, which is correct because the backing is
 * the system allocator. */

__attribute__((visibility("default"))) void* malloc(size_t size) {
  return hz11_malloc(size);
}

__attribute__((visibility("default"))) void free(void* ptr) {
  hz11_free(ptr);
}

__attribute__((visibility("default"))) void* calloc(size_t count, size_t size) {
  return hz11_calloc(count, size);
}

__attribute__((visibility("default"))) void* realloc(void* ptr, size_t size) {
  return hz11_realloc(ptr, size);
}

__attribute__((visibility("default"))) size_t malloc_usable_size(void* ptr) {
  return hz11_malloc_usable_size(ptr);
}

__attribute__((visibility("default"))) int posix_memalign(void** memptr,
                                                          size_t alignment,
                                                          size_t size) {
  return hz11_posix_memalign(memptr, alignment, size);
}

__attribute__((visibility("default"))) void* aligned_alloc(size_t alignment,
                                                           size_t size) {
  return hz11_aligned_alloc(alignment, size);
}

__attribute__((visibility("default"))) void* memalign(size_t alignment,
                                                      size_t size) {
  return hz11_memalign(alignment, size);
}

/* Resolve the system allocator eagerly at load time so that by the time any
 * user malloc runs (after constructors), sys_* are bound and the cache can be
 * used. Anything dlsym itself allocates during this call is caught by the
 * in-resolver/bootstrap path. */
__attribute__((constructor)) static void hz11_shim_init(void) {
  hz11_resolver_ensure();
  hz11_size_class_init();
  if (getenv("HZ11_DUMP_STATS") != NULL) {
    atexit(hz11_dump_stats_atexit);
  }
}
