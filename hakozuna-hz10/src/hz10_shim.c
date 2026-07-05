#include "hz10_freelist_page.h"
#include "hz10_large_alloc.h"
#include "hz10_page_pool.h"
#include "hz10_pagemap.h"
#include "hz10_public_entry.h"
#include "hz10_size_class.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static _Atomic(uint64_t) hz10_shim_foreign_free_count;

static int hz10_shim_is_power_of_two(size_t value) {
  return value != 0u && (value & (value - 1u)) == 0u;
}

static void hz10_shim_atfork_prepare(void) {
  hz10_pagemap_atfork_prepare();
  hz10_freelist_page_atfork_prepare();
}

static void hz10_shim_atfork_parent(void) {
  hz10_freelist_page_atfork_parent();
  hz10_pagemap_atfork_parent();
}

static void hz10_shim_atfork_child(void) {
  hz10_freelist_page_atfork_child();
  hz10_pagemap_atfork_child();
}

static int hz10_shim_exit_stats_enabled(void) {
  const char* value = getenv("HZ10_SHIM_EXIT_STATS");
  return value && value[0] == '1' && value[1] == '\0';
}

static void hz10_shim_write_all(const char* text, size_t len);

static void hz10_shim_writef(const char* fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) {
    return;
  }
  size_t len = (size_t)n;
  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1u;
  }
  hz10_shim_write_all(buf, len);
}

void hz10_shim_dump_exit_stats(void) {
  uint64_t foreign_frees = atomic_load_explicit(&hz10_shim_foreign_free_count,
                                                memory_order_relaxed);
  Hz10FreelistMetadataStats metadata = {0};
  hz10_freelist_metadata_stats(&metadata);

  uint32_t pool_cached = hz10_page_pool_cached_count();
  uint64_t pool_reuse = hz10_page_pool_reuse_count();
  uint64_t pool_release = hz10_page_pool_release_count();
  uint64_t pool_purged = hz10_page_pool_purged_count();

  uint64_t active_pages = 0u;
  uint64_t retired_pages = 0u;
  uint64_t max_retired_pages = 0u;
  uint64_t eviction_count = 0u;
  uint64_t retired_count = 0u;
  uint64_t ready_reclaimed = 0u;
  uint64_t sweep_reclaimed = 0u;
  uint64_t local_free_reclaimed = 0u;

  hz10_shim_writef(
      "hz10_shim_exit_stats summary class_count=%u foreign_frees=%llu "
      "pool_cached=%u pool_cached_bytes=%llu pool_reuse=%llu "
      "pool_release=%llu pool_purged=%llu metadata_slabs=%llu "
      "metadata_slab_bytes=%u metadata_node_bytes=%u "
      "metadata_capacity=%llu metadata_live=%llu metadata_free=%llu\n",
      (unsigned)HZ10_CLASS_COUNT, (unsigned long long)foreign_frees,
      (unsigned)pool_cached,
      (unsigned long long)pool_cached * (unsigned long long)HZ10_PAGE_QUANTUM,
      (unsigned long long)pool_reuse, (unsigned long long)pool_release,
      (unsigned long long)pool_purged,
      (unsigned long long)metadata.slab_count, (unsigned)metadata.slab_bytes,
      (unsigned)metadata.node_bytes,
      (unsigned long long)metadata.node_capacity,
      (unsigned long long)metadata.live_nodes,
      (unsigned long long)metadata.free_nodes);

  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassPageListStats stats = {0};
    hz10_public_entry_class_list_stats(c, &stats);
    if (stats.active_length == 0u && stats.retired_length == 0u &&
        stats.max_retired_length == 0u && stats.eviction_count == 0u &&
        stats.retired_count == 0u) {
      continue;
    }

    uint32_t slot_size = hz10_size_class_slot_size(c);
    uint32_t slot_count = hz10_size_class_slot_count(c);
    active_pages += stats.active_length;
    retired_pages += stats.retired_length;
    max_retired_pages += stats.max_retired_length;
    eviction_count += stats.eviction_count;
    retired_count += stats.retired_count;
    ready_reclaimed += stats.retired_reclaimed_by_ready_count;
    sweep_reclaimed += stats.retired_reclaimed_by_sweep_count;
    local_free_reclaimed += stats.retired_reclaimed_by_local_free_count;

    hz10_shim_writef(
        "hz10_shim_exit_stats class=%u slot_size=%u slot_count=%u "
        "active_pages=%u retired_pages=%u max_retired=%u "
        "evictions=%llu retired=%llu reclaimed_ready=%llu "
        "reclaimed_sweep=%llu reclaimed_local_free=%llu "
        "find_calls=%llu find_misses=%llu find_pages_visited=%llu\n",
        (unsigned)c, (unsigned)slot_size, (unsigned)slot_count,
        (unsigned)stats.active_length, (unsigned)stats.retired_length,
        (unsigned)stats.max_retired_length,
        (unsigned long long)stats.eviction_count,
        (unsigned long long)stats.retired_count,
        (unsigned long long)stats.retired_reclaimed_by_ready_count,
        (unsigned long long)stats.retired_reclaimed_by_sweep_count,
        (unsigned long long)stats.retired_reclaimed_by_local_free_count,
        (unsigned long long)stats.find_call_count,
        (unsigned long long)stats.find_miss_count,
        (unsigned long long)stats.find_pages_visited_count);
  }

  hz10_shim_writef(
      "hz10_shim_exit_stats class_totals active_pages=%llu "
      "retired_pages=%llu max_retired_sum=%llu page_bytes=%llu "
      "evictions=%llu retired=%llu reclaimed_ready=%llu "
      "reclaimed_sweep=%llu reclaimed_local_free=%llu\n",
      (unsigned long long)active_pages, (unsigned long long)retired_pages,
      (unsigned long long)max_retired_pages,
      (unsigned long long)(active_pages + retired_pages) *
          (unsigned long long)HZ10_PAGE_QUANTUM,
      (unsigned long long)eviction_count, (unsigned long long)retired_count,
      (unsigned long long)ready_reclaimed,
      (unsigned long long)sweep_reclaimed,
      (unsigned long long)local_free_reclaimed);
}

__attribute__((constructor)) static void hz10_shim_init(void) {
  (void)pthread_atfork(hz10_shim_atfork_prepare, hz10_shim_atfork_parent,
                       hz10_shim_atfork_child);
  if (hz10_shim_exit_stats_enabled()) {
    (void)atexit(hz10_shim_dump_exit_stats);
  }
}

static size_t hz10_shim_request_size(size_t size) {
  return size == 0u ? (size_t)HZ10_MIN_ALIGN : size;
}

static void hz10_shim_write_all(const char* text, size_t len) {
  while (len != 0u) {
    ssize_t n = write(STDERR_FILENO, text, len);
    if (n <= 0) {
      return;
    }
    text += (size_t)n;
    len -= (size_t)n;
  }
}

static void hz10_shim_abort_unknown(const char* api) {
  static const char prefix[] = "hz10 shim: unknown pointer in ";
  static const char suffix[] = "\n";
  hz10_shim_write_all(prefix, sizeof(prefix) - 1u);
  hz10_shim_write_all(api, strlen(api));
  hz10_shim_write_all(suffix, sizeof(suffix) - 1u);
  abort();
}

static int hz10_shim_tolerate_foreign(void) {
  const char* value = getenv("HZ10_SHIM_TOLERATE_FOREIGN");
  return value && value[0] == '1' && value[1] == '\0';
}

static size_t hz10_shim_usable_size_or_abort(void* ptr, const char* api) {
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  if (route.kind != H10_ROUTE_VALID) {
    hz10_shim_abort_unknown(api);
  }
  return (size_t)route.slot_size;
}

static void* hz10_shim_aligned_alloc_impl(size_t alignment, size_t size) {
  size_t request = hz10_shim_request_size(size);
  if (alignment <= (size_t)HZ10_MIN_ALIGN) {
    return hz10_malloc(request);
  }
  if (alignment > (size_t)HZ10_PAGE_QUANTUM) {
    errno = ENOMEM;
    return NULL;
  }
  return hz10_large_alloc(request);
}

static void* hz10_shim_malloc_impl(size_t size) {
  return hz10_malloc(hz10_shim_request_size(size));
}

static void hz10_shim_free_impl(void* ptr, const char* api) {
  if (!ptr) {
    return;
  }
  if (!hz10_free(ptr)) {
    if (!hz10_shim_tolerate_foreign()) {
      hz10_shim_abort_unknown(api);
    }
    atomic_fetch_add_explicit(&hz10_shim_foreign_free_count, 1u,
                              memory_order_relaxed);
  }
}

uint64_t hz10_shim_tolerated_foreign_free_count(void) {
  return atomic_load_explicit(&hz10_shim_foreign_free_count,
                              memory_order_relaxed);
}

void* malloc(size_t size) {
  return hz10_shim_malloc_impl(size);
}

void free(void* ptr) {
  hz10_shim_free_impl(ptr, "free");
}

void* calloc(size_t count, size_t size) {
  if (count != 0u && size > SIZE_MAX / count) {
    errno = ENOMEM;
    return NULL;
  }
  size_t total = count * size;
  void* ptr = hz10_shim_malloc_impl(total);
  if (ptr) {
    memset(ptr, 0, hz10_shim_request_size(total));
  }
  return ptr;
}

void* realloc(void* ptr, size_t size) {
  if (!ptr) {
    return hz10_shim_malloc_impl(size);
  }
  if (size == 0u) {
    hz10_shim_free_impl(ptr, "realloc");
    return NULL;
  }

  size_t old_size = hz10_shim_usable_size_or_abort(ptr, "realloc");
  if (size <= old_size) {
    return ptr;
  }

  void* next = hz10_shim_malloc_impl(size);
  if (!next) {
    return NULL;
  }
  memcpy(next, ptr, old_size);
  hz10_shim_free_impl(ptr, "realloc");
  return next;
}

size_t malloc_usable_size(void* ptr) {
  if (!ptr) {
    return 0u;
  }
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  return route.kind == H10_ROUTE_VALID ? (size_t)route.slot_size : 0u;
}

int posix_memalign(void** out, size_t alignment, size_t size) {
  if (alignment < sizeof(void*) || !hz10_shim_is_power_of_two(alignment)) {
    return EINVAL;
  }
  void* ptr = hz10_shim_aligned_alloc_impl(alignment, size);
  if (!ptr) {
    return errno == ENOMEM ? ENOMEM : EINVAL;
  }
  *out = ptr;
  return 0;
}

void* aligned_alloc(size_t alignment, size_t size) {
  if (!hz10_shim_is_power_of_two(alignment) ||
      (alignment != 0u && (size % alignment) != 0u)) {
    errno = EINVAL;
    return NULL;
  }
  return hz10_shim_aligned_alloc_impl(alignment, size);
}

void* memalign(size_t alignment, size_t size) {
  if (!hz10_shim_is_power_of_two(alignment)) {
    errno = EINVAL;
    return NULL;
  }
  return hz10_shim_aligned_alloc_impl(alignment, size);
}
