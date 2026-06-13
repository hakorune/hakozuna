#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_profiles.h"

#include <dlfcn.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void* __libc_malloc(size_t size);
extern void __libc_free(void* ptr);
extern void* __libc_calloc(size_t nmemb, size_t size);
extern void* __libc_realloc(void* ptr, size_t size);
extern int __libc_posix_memalign(void** memptr, size_t alignment, size_t size);

typedef void* (*Hz6RealMallocFn)(size_t);
typedef void (*Hz6RealFreeFn)(void*);
typedef void* (*Hz6RealCallocFn)(size_t, size_t);
typedef void* (*Hz6RealReallocFn)(void*, size_t);
typedef int (*Hz6RealPosixMemalignFn)(void**, size_t, size_t);
typedef void* (*Hz6RealAlignedAllocFn)(size_t, size_t);
typedef size_t (*Hz6RealUsableSizeFn)(void*);

typedef struct Hz6PreloadRoute {
  Hz6RouteResult route;
  int visible_hit;
} Hz6PreloadRoute;

static Hz6RealMallocFn g_real_malloc;
static Hz6RealFreeFn g_real_free;
static Hz6RealCallocFn g_real_calloc;
static Hz6RealReallocFn g_real_realloc;
static Hz6RealPosixMemalignFn g_real_posix_memalign;
static Hz6RealAlignedAllocFn g_real_aligned_alloc;
static Hz6RealUsableSizeFn g_real_malloc_usable_size;
static pthread_once_t g_real_once = PTHREAD_ONCE_INIT;
static __thread int g_hz6_preload_reentry;
static __thread Hz6Allocator* g_hz6_preload_allocator;

#define HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY 256u

static pthread_mutex_t g_hz6_preload_allocator_registry_mutex =
    PTHREAD_MUTEX_INITIALIZER;
static Hz6Allocator*
    g_hz6_preload_allocator_registry[HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY];
static size_t g_hz6_preload_allocator_registry_count;

static void hz6_preload_resolve_real(void) {
  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;
  g_real_malloc = (Hz6RealMallocFn)dlsym(RTLD_NEXT, "malloc");
  g_real_free = (Hz6RealFreeFn)dlsym(RTLD_NEXT, "free");
  g_real_calloc = (Hz6RealCallocFn)dlsym(RTLD_NEXT, "calloc");
  g_real_realloc = (Hz6RealReallocFn)dlsym(RTLD_NEXT, "realloc");
  g_real_posix_memalign =
      (Hz6RealPosixMemalignFn)dlsym(RTLD_NEXT, "posix_memalign");
  g_real_aligned_alloc =
      (Hz6RealAlignedAllocFn)dlsym(RTLD_NEXT, "aligned_alloc");
  g_real_malloc_usable_size =
      (Hz6RealUsableSizeFn)dlsym(RTLD_NEXT, "malloc_usable_size");
  g_hz6_preload_reentry = saved_reentry;
}

static void hz6_preload_ensure_real(void) {
  (void)pthread_once(&g_real_once, hz6_preload_resolve_real);
}

static void* hz6_preload_real_malloc(size_t size) {
  if (g_hz6_preload_reentry && !g_real_malloc) {
    return __libc_malloc(size);
  }
  hz6_preload_ensure_real();
  if (g_real_malloc) {
    return g_real_malloc(size);
  }
  return __libc_malloc(size);
}

static void hz6_preload_real_free(void* ptr) {
  if (g_hz6_preload_reentry && !g_real_free) {
    __libc_free(ptr);
    return;
  }
  hz6_preload_ensure_real();
  if (g_real_free) {
    g_real_free(ptr);
    return;
  }
  __libc_free(ptr);
}

static void* hz6_preload_real_calloc(size_t nmemb, size_t size) {
  if (g_hz6_preload_reentry && !g_real_calloc) {
    return __libc_calloc(nmemb, size);
  }
  hz6_preload_ensure_real();
  if (g_real_calloc) {
    return g_real_calloc(nmemb, size);
  }
  return __libc_calloc(nmemb, size);
}

static void* hz6_preload_real_realloc(void* ptr, size_t size) {
  if (g_hz6_preload_reentry && !g_real_realloc) {
    return __libc_realloc(ptr, size);
  }
  hz6_preload_ensure_real();
  if (g_real_realloc) {
    return g_real_realloc(ptr, size);
  }
  return __libc_realloc(ptr, size);
}

static int hz6_preload_real_posix_memalign(void** memptr,
                                           size_t alignment,
                                           size_t size) {
  if (g_hz6_preload_reentry && !g_real_posix_memalign) {
    return __libc_posix_memalign(memptr, alignment, size);
  }
  hz6_preload_ensure_real();
  if (g_real_posix_memalign) {
    return g_real_posix_memalign(memptr, alignment, size);
  }
  return __libc_posix_memalign(memptr, alignment, size);
}

static Hz6ProfileId hz6_preload_profile_from_env(void) {
  const char* value = getenv("HZ6_PRELOAD_PROFILE");
  if (!value || value[0] == '\0' || strcmp(value, "speed") == 0) {
    return HZ6_PROFILE_SPEED;
  }
  if (strcmp(value, "rss") == 0) {
    return HZ6_PROFILE_RSS;
  }
  if (strcmp(value, "remote") == 0) {
    return HZ6_PROFILE_REMOTE;
  }
  if (strcmp(value, "strict") == 0) {
    return HZ6_PROFILE_STRICT;
  }
  return HZ6_PROFILE_SPEED;
}

static void hz6_preload_register_allocator(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
  pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
  for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
    if (g_hz6_preload_allocator_registry[i] == allocator) {
      pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
      return;
    }
  }
  if (g_hz6_preload_allocator_registry_count <
      HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY) {
    g_hz6_preload_allocator_registry[g_hz6_preload_allocator_registry_count++] =
        allocator;
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
}

static void hz6_preload_print_stats(void) {
  const char* value = getenv("HZ6_PRELOAD_STATS");
  if (!value || value[0] == '\0' || strcmp(value, "0") == 0) {
    return;
  }

  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;

  size_t allocator_count = 0;
  size_t route_valid = 0;
  size_t route_invalid = 0;
  size_t route_miss = 0;
  size_t transfer_push = 0;
  size_t transfer_pop = 0;
  size_t source_alloc = 0;
  size_t toy_source_alloc = 0;
  size_t midpage_source_alloc = 0;
  size_t large_source_alloc = 0;
  size_t local2p_source_alloc = 0;
  size_t alloc_fail = 0;
  size_t descriptor_exhausted = 0;
  size_t route_register_fail = 0;
  size_t source_block_exhausted = 0;
  size_t route_lookup_probe_total = 0;
  size_t route_lookup_probe_max = 0;
  size_t route_register_probe_total = 0;
  size_t route_register_probe_max = 0;
  size_t route_unregister_probe_total = 0;
  size_t route_unregister_probe_max = 0;

  pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
  allocator_count = g_hz6_preload_allocator_registry_count;
  for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
    Hz6Allocator* allocator = g_hz6_preload_allocator_registry[i];
    if (!allocator) {
      continue;
    }
    Hz6StatsSnapshot stats = hz6_stats_snapshot(allocator);
    route_valid += stats.route_valid;
    route_invalid += stats.route_invalid;
    route_miss += stats.route_miss;
    transfer_push += stats.transfer_push;
    transfer_pop += stats.transfer_pop;
    source_alloc += stats.source_alloc;
    toy_source_alloc += stats.toy_source_alloc;
    midpage_source_alloc += stats.midpage_source_alloc;
    large_source_alloc += stats.large_source_alloc;
    local2p_source_alloc += stats.local2p_source_alloc;
    alloc_fail += stats.alloc_fail;
    descriptor_exhausted += stats.descriptor_exhausted;
    route_register_fail += stats.route_register_fail;
    source_block_exhausted += stats.source_block_exhausted;
    route_lookup_probe_total += stats.route_lookup_probe_total;
    if (stats.route_lookup_probe_max > route_lookup_probe_max) {
      route_lookup_probe_max = stats.route_lookup_probe_max;
    }
    route_register_probe_total += stats.route_register_probe_total;
    if (stats.route_register_probe_max > route_register_probe_max) {
      route_register_probe_max = stats.route_register_probe_max;
    }
    route_unregister_probe_total += stats.route_unregister_probe_total;
    if (stats.route_unregister_probe_max > route_unregister_probe_max) {
      route_unregister_probe_max = stats.route_unregister_probe_max;
    }
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);

  fprintf(stderr,
          "[HZ6_PRELOAD_STATS] allocators=%zu route_valid=%zu "
          "route_invalid=%zu route_miss=%zu transfer_push=%zu "
          "transfer_pop=%zu source_alloc=%zu toy_source_alloc=%zu "
          "midpage_source_alloc=%zu large_source_alloc=%zu "
          "local2p_source_alloc=%zu alloc_fail=%zu "
          "descriptor_exhausted=%zu route_register_fail=%zu "
          "source_block_exhausted=%zu route_lookup_probe_total=%zu "
          "route_lookup_probe_max=%zu route_register_probe_total=%zu "
          "route_register_probe_max=%zu route_unregister_probe_total=%zu "
          "route_unregister_probe_max=%zu\n",
          allocator_count, route_valid, route_invalid, route_miss,
          transfer_push, transfer_pop, source_alloc, toy_source_alloc,
          midpage_source_alloc, large_source_alloc, local2p_source_alloc,
          alloc_fail, descriptor_exhausted, route_register_fail,
          source_block_exhausted, route_lookup_probe_total,
          route_lookup_probe_max, route_register_probe_total,
          route_register_probe_max, route_unregister_probe_total,
          route_unregister_probe_max);

  g_hz6_preload_reentry = saved_reentry;
}

__attribute__((destructor)) static void hz6_preload_on_unload(void) {
  hz6_preload_print_stats();
}

static Hz6Allocator* hz6_preload_allocator(void) {
  if (g_hz6_preload_allocator) {
    return g_hz6_preload_allocator;
  }

  Hz6Allocator* allocator =
      (Hz6Allocator*)hz6_preload_real_calloc(1, sizeof(Hz6Allocator));
  if (!allocator) {
    return NULL;
  }
  hz6_allocator_init_with_profile(allocator, hz6_preload_profile_from_env());
  g_hz6_preload_allocator = allocator;
  hz6_preload_register_allocator(allocator);
  return allocator;
}

static Hz6PreloadRoute hz6_preload_route(Hz6Allocator* allocator,
                                         const void* ptr) {
  Hz6PreloadRoute preload_route = {0};
  preload_route.route = hz6_route_miss();
  if (!allocator || !ptr) {
    return preload_route;
  }
  preload_route.route = hz6_allocator_route_lookup(allocator, ptr);
  if (preload_route.route.kind == HZ6_ROUTE_MISS) {
    preload_route.route =
        hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
    preload_route.visible_hit =
        preload_route.route.kind != HZ6_ROUTE_MISS ? 1 : 0;
  }
  return preload_route;
}

static size_t hz6_preload_usable_size(Hz6Allocator* allocator,
                                      const void* ptr) {
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
  if (preload_route.route.kind != HZ6_ROUTE_VALID ||
      !preload_route.route.descriptor) {
    return 0;
  }
  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)preload_route.route.descriptor;
  return descriptor->bytes;
}

void* malloc(size_t size) {
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_malloc(size);
  }
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  void* ptr = allocator ? hz6_malloc(allocator, size) : NULL;
  g_hz6_preload_reentry = 0;
  return ptr ? ptr : hz6_preload_real_malloc(size);
}

void free(void* ptr) {
  if (!ptr) {
    return;
  }
  if (g_hz6_preload_reentry) {
    hz6_preload_real_free(ptr);
    return;
  }
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
  if (preload_route.route.kind == HZ6_ROUTE_VALID ||
      preload_route.route.kind == HZ6_ROUTE_INVALID) {
#if HZ6_PRELOAD_FAST_FREE_L1
    hz6_free_with_route_prechecked(allocator, ptr, preload_route.route,
                                   preload_route.visible_hit);
#else
    hz6_free(allocator, ptr);
#endif
  } else {
    hz6_preload_real_free(ptr);
  }
  g_hz6_preload_reentry = 0;
}

void* calloc(size_t nmemb, size_t size) {
  if (size != 0 && nmemb > ((size_t)-1) / size) {
    errno = ENOMEM;
    return NULL;
  }
  size_t bytes = nmemb * size;
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_calloc(nmemb, size);
  }
  void* ptr = malloc(bytes);
  if (ptr) {
    memset(ptr, 0, bytes);
  }
  return ptr;
}

void* realloc(void* ptr, size_t size) {
  if (!ptr) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_realloc(ptr, size);
  }

  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  size_t old_size = hz6_preload_usable_size(allocator, ptr);
  g_hz6_preload_reentry = 0;
  if (old_size == 0) {
    return hz6_preload_real_realloc(ptr, size);
  }

  void* next = malloc(size);
  if (!next) {
    return NULL;
  }
  memcpy(next, ptr, old_size < size ? old_size : size);
  free(ptr);
  return next;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  if ((alignment & (alignment - 1u)) != 0 || alignment < sizeof(void*)) {
    return EINVAL;
  }
  if (alignment <= 16u) {
    void* ptr = malloc(size);
    if (!ptr) {
      return ENOMEM;
    }
    *memptr = ptr;
    return 0;
  }
  return hz6_preload_real_posix_memalign(memptr, alignment, size);
}

void* aligned_alloc(size_t alignment, size_t size) {
  if (alignment <= 16u) {
    return malloc(size);
  }
  if (g_hz6_preload_reentry && !g_real_aligned_alloc) {
    void* ptr = NULL;
    return __libc_posix_memalign(&ptr, alignment, size) == 0 ? ptr : NULL;
  }
  hz6_preload_ensure_real();
  if (g_real_aligned_alloc) {
    return g_real_aligned_alloc(alignment, size);
  }
  void* ptr = NULL;
  if (hz6_preload_real_posix_memalign(&ptr, alignment, size) != 0) {
    return NULL;
  }
  return ptr;
}

size_t malloc_usable_size(void* ptr) {
  if (!ptr) {
    return 0;
  }
  if (!g_hz6_preload_reentry) {
    g_hz6_preload_reentry = 1;
    Hz6Allocator* allocator = hz6_preload_allocator();
    size_t bytes = hz6_preload_usable_size(allocator, ptr);
    g_hz6_preload_reentry = 0;
    if (bytes != 0) {
      return bytes;
    }
  }
  if (g_hz6_preload_reentry && !g_real_malloc_usable_size) {
    return 0;
  }
  hz6_preload_ensure_real();
  return g_real_malloc_usable_size ? g_real_malloc_usable_size(ptr) : 0;
}
