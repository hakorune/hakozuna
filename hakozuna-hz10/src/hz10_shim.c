#include "hz10_freelist_page.h"
#include "hz10_large_alloc.h"
#include "hz10_pagemap.h"
#include "hz10_public_entry.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
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

__attribute__((constructor)) static void hz10_shim_init(void) {
  (void)pthread_atfork(hz10_shim_atfork_prepare, hz10_shim_atfork_parent,
                       hz10_shim_atfork_child);
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
