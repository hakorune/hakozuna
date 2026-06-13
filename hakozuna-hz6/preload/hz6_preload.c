#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_toy_small_diag.h"
#include "linux_source_mmap.h"
#include "hz6_profiles.h"

#include <dlfcn.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
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

typedef struct Hz6PreloadPhaseStats {
  _Atomic(size_t) malloc_calls;
  _Atomic(size_t) malloc_hz6_success;
  _Atomic(size_t) malloc_real_fallback;
  _Atomic(size_t) malloc_size_zero;
  _Atomic(size_t) malloc_size_le1024;
  _Atomic(size_t) malloc_size_1025_4096;
  _Atomic(size_t) malloc_size_4097_16384;
  _Atomic(size_t) malloc_size_gt16384;
  _Atomic(size_t) calloc_calls;
  _Atomic(size_t) free_calls;
  _Atomic(size_t) free_null;
  _Atomic(size_t) free_reentry_real;
  _Atomic(size_t) free_local_route_valid;
  _Atomic(size_t) free_visible_route_hit;
  _Atomic(size_t) free_toy_active_map_hit;
  _Atomic(size_t) free_midpage_active_map_hit;
  _Atomic(size_t) free_route_invalid;
  _Atomic(size_t) free_route_miss_real;
  _Atomic(size_t) free_prechecked_candidate;
  _Atomic(size_t) realloc_calls;
  _Atomic(size_t) realloc_owned;
  _Atomic(size_t) realloc_in_place;
  _Atomic(size_t) realloc_real_fallback;
  _Atomic(size_t) realloc_copy_calls;
  _Atomic(size_t) realloc_copy_bytes;
  _Atomic(size_t) realloc_request_zero;
  _Atomic(size_t) realloc_request_le1024;
  _Atomic(size_t) realloc_request_1025_4096;
  _Atomic(size_t) realloc_request_4097_16384;
  _Atomic(size_t) realloc_request_gt16384;
  _Atomic(size_t) realloc_owned_old_zero;
  _Atomic(size_t) realloc_owned_old_le1024;
  _Atomic(size_t) realloc_owned_old_1025_4096;
  _Atomic(size_t) realloc_owned_old_4097_16384;
  _Atomic(size_t) realloc_owned_old_gt16384;
  _Atomic(size_t) malloc_usable_size_calls;
  _Atomic(size_t) malloc_usable_size_owned;
  _Atomic(size_t) malloc_usable_size_real_fallback;
} Hz6PreloadPhaseStats;

static Hz6PreloadPhaseStats g_hz6_preload_phase_stats;
static int g_hz6_preload_phase_stats_enabled;

static void hz6_preload_phase_count(_Atomic(size_t)* counter) {
  if (g_hz6_preload_phase_stats_enabled) {
    (void)atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
  }
}

static void hz6_preload_phase_add(_Atomic(size_t)* counter, size_t value) {
  if (g_hz6_preload_phase_stats_enabled && value != 0) {
    (void)atomic_fetch_add_explicit(counter, value, memory_order_relaxed);
  }
}

static size_t hz6_preload_phase_load(const _Atomic(size_t)* counter) {
  return atomic_load_explicit(counter, memory_order_relaxed);
}

static void hz6_preload_phase_count_size_bucket(
    size_t size,
    _Atomic(size_t)* zero,
    _Atomic(size_t)* le1024,
    _Atomic(size_t)* range1025_4096,
    _Atomic(size_t)* range4097_16384,
    _Atomic(size_t)* gt16384) {
  if (size == 0) {
    hz6_preload_phase_count(zero);
  } else if (size <= 1024u) {
    hz6_preload_phase_count(le1024);
  } else if (size <= 4096u) {
    hz6_preload_phase_count(range1025_4096);
  } else if (size <= 16384u) {
    hz6_preload_phase_count(range4097_16384);
  } else {
    hz6_preload_phase_count(gt16384);
  }
}

__attribute__((constructor)) static void hz6_preload_on_load(void) {
  const char* value = getenv("HZ6_PRELOAD_STATS");
  g_hz6_preload_phase_stats_enabled =
      value && value[0] != '\0' && strcmp(value, "0") != 0;
}

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
  int print_per_allocator = strcmp(value, "2") == 0 ||
                            strcmp(value, "per_allocator") == 0 ||
                            strcmp(value, "per-allocator") == 0;

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
  size_t source_prefill_attempt = 0;
  size_t source_prefill_filled = 0;
  size_t source_prefill_fallback = 0;
  size_t front_source_prefill_alloc = 0;
  size_t toy_source_prefill_call = 0;
  size_t route_lookup_probe_total = 0;
  size_t route_lookup_probe_max = 0;
  size_t route_register_probe_total = 0;
  size_t route_register_probe_max = 0;
  size_t route_unregister_probe_total = 0;
  size_t route_unregister_probe_max = 0;
  size_t route_register_reason_unknown = 0;
  size_t route_register_reason_source_run_slot = 0;
  size_t route_register_reason_direct_source = 0;
  size_t route_register_reason_materialize = 0;
  size_t route_register_reason_rehome = 0;
  size_t route_unregister_reason_unknown = 0;
  size_t route_unregister_reason_frontcache_overflow = 0;
  size_t route_unregister_reason_cap_release = 0;
  size_t route_unregister_reason_descriptorless_detach = 0;
  size_t route_unregister_reason_source_slot_release = 0;
  size_t route_unregister_reason_rehome = 0;
  size_t route_active_current = 0;
  size_t route_active_max = 0;
  size_t route_tombstone_current = 0;
  size_t route_tombstone_max = 0;
  size_t route_register_used_tombstone = 0;
  size_t route_register_full_probe_with_tombstone = 0;
  size_t route_tombstone_compact_attempt = 0;
  size_t route_tombstone_compact_success = 0;
  size_t route_tombstone_compact_fail_alloc = 0;
  size_t route_tombstone_compact_moved = 0;
  size_t route_tombstone_cond_probe = 0;
  size_t route_tombstone_cond_would_compact = 0;
  size_t route_tombstone_cond_ratio25 = 0;
  size_t route_tombstone_cond_occupancy75 = 0;
  size_t route_tombstone_cond_cooldown_blocked = 0;
  size_t route_tombstone_cond_highwater = 0;
  size_t descriptor_live_max = 0;
  size_t source_block_active_max = 0;
  size_t frontcache_total_max = 0;
  size_t frontcache_reuse_hit = 0;
  size_t frontcache_reuse_invalid = 0;
  size_t frontcache_spill_success = 0;
  size_t frontcache_spill_retry_success = 0;
  size_t frontcache_borrow_success = 0;
  size_t toy_small_malloc_frontcache_pop = 0;
  size_t toy_small_malloc_activate_success = 0;
  size_t toy_small_free_cache_attempt = 0;
  size_t toy_small_free_cache_success = 0;
  size_t toy_small_active_map_register = 0;
  size_t toy_small_active_map_register_collision = 0;
  size_t toy_small_active_map_free_attempt = 0;
  size_t toy_small_active_map_free_hit = 0;
  size_t toy_small_active_map_free_miss = 0;
  size_t toy_small_active_map_free_stale = 0;
  size_t toy_small_active_map_free_cache_fail = 0;
  size_t toy_small_active_map_route_bypass = 0;
  size_t midpage_active_map_register = 0;
  size_t midpage_active_map_register_collision = 0;
  size_t midpage_active_map_free_attempt = 0;
  size_t midpage_active_map_free_hit = 0;
  size_t midpage_active_map_free_miss = 0;
  size_t midpage_active_map_free_stale = 0;
  size_t midpage_active_map_free_cache_fail = 0;
  size_t midpage_active_map_alignment_skip = 0;
  size_t midpage_active_map_route_bypass = 0;

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
    source_prefill_attempt += stats.source_prefill_attempt;
    source_prefill_filled += stats.source_prefill_filled;
    source_prefill_fallback += stats.source_prefill_fallback;
    front_source_prefill_alloc += stats.front_source_prefill_alloc;
    toy_source_prefill_call += stats.toy_source_prefill_call;
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
    route_register_reason_unknown += stats.route_register_reason_unknown;
    route_register_reason_source_run_slot +=
        stats.route_register_reason_source_run_slot;
    route_register_reason_direct_source +=
        stats.route_register_reason_direct_source;
    route_register_reason_materialize +=
        stats.route_register_reason_materialize;
    route_register_reason_rehome += stats.route_register_reason_rehome;
    route_unregister_reason_unknown += stats.route_unregister_reason_unknown;
    route_unregister_reason_frontcache_overflow +=
        stats.route_unregister_reason_frontcache_overflow;
    route_unregister_reason_cap_release +=
        stats.route_unregister_reason_cap_release;
    route_unregister_reason_descriptorless_detach +=
        stats.route_unregister_reason_descriptorless_detach;
    route_unregister_reason_source_slot_release +=
        stats.route_unregister_reason_source_slot_release;
    route_unregister_reason_rehome += stats.route_unregister_reason_rehome;
    route_active_current += stats.route_active_current;
    if (stats.route_active_max > route_active_max) {
      route_active_max = stats.route_active_max;
    }
    route_tombstone_current += stats.route_tombstone_current;
    if (stats.route_tombstone_max > route_tombstone_max) {
      route_tombstone_max = stats.route_tombstone_max;
    }
    route_register_used_tombstone += stats.route_register_used_tombstone;
    route_register_full_probe_with_tombstone +=
        stats.route_register_full_probe_with_tombstone;
    route_tombstone_compact_attempt += stats.route_tombstone_compact_attempt;
    route_tombstone_compact_success += stats.route_tombstone_compact_success;
    route_tombstone_compact_fail_alloc +=
        stats.route_tombstone_compact_fail_alloc;
    route_tombstone_compact_moved += stats.route_tombstone_compact_moved;
    route_tombstone_cond_probe += stats.route_tombstone_cond_probe;
    route_tombstone_cond_would_compact +=
        stats.route_tombstone_cond_would_compact;
    route_tombstone_cond_ratio25 += stats.route_tombstone_cond_ratio25;
    route_tombstone_cond_occupancy75 +=
        stats.route_tombstone_cond_occupancy75;
    route_tombstone_cond_cooldown_blocked +=
        stats.route_tombstone_cond_cooldown_blocked;
    if (stats.route_tombstone_cond_highwater >
        route_tombstone_cond_highwater) {
      route_tombstone_cond_highwater =
          stats.route_tombstone_cond_highwater;
    }
    if (stats.descriptor_live_max > descriptor_live_max) {
      descriptor_live_max = stats.descriptor_live_max;
    }
    if (stats.source_block_active_max > source_block_active_max) {
      source_block_active_max = stats.source_block_active_max;
    }
    if (stats.frontcache_total_max > frontcache_total_max) {
      frontcache_total_max = stats.frontcache_total_max;
    }
    frontcache_reuse_hit += stats.frontcache_reuse_hit;
    frontcache_reuse_invalid += stats.frontcache_reuse_invalid;
    frontcache_spill_success += stats.frontcache_spill_success;
    frontcache_spill_retry_success += stats.frontcache_spill_retry_success;
    frontcache_borrow_success += stats.frontcache_borrow_success;
    toy_small_malloc_frontcache_pop += stats.toy_small_malloc_frontcache_pop;
    toy_small_malloc_activate_success +=
        stats.toy_small_malloc_activate_success;
    toy_small_free_cache_attempt += stats.toy_small_free_cache_attempt;
    toy_small_free_cache_success += stats.toy_small_free_cache_success;
    toy_small_active_map_register += stats.toy_small_active_map_register;
    toy_small_active_map_register_collision +=
        stats.toy_small_active_map_register_collision;
    toy_small_active_map_free_attempt +=
        stats.toy_small_active_map_free_attempt;
    toy_small_active_map_free_hit += stats.toy_small_active_map_free_hit;
    toy_small_active_map_free_miss += stats.toy_small_active_map_free_miss;
    toy_small_active_map_free_stale += stats.toy_small_active_map_free_stale;
    toy_small_active_map_free_cache_fail +=
        stats.toy_small_active_map_free_cache_fail;
    toy_small_active_map_route_bypass +=
        stats.toy_small_active_map_route_bypass;
    midpage_active_map_register += stats.midpage_active_map_register;
    midpage_active_map_register_collision +=
        stats.midpage_active_map_register_collision;
    midpage_active_map_free_attempt +=
        stats.midpage_active_map_free_attempt;
    midpage_active_map_free_hit += stats.midpage_active_map_free_hit;
    midpage_active_map_free_miss += stats.midpage_active_map_free_miss;
    midpage_active_map_free_stale += stats.midpage_active_map_free_stale;
    midpage_active_map_free_cache_fail +=
        stats.midpage_active_map_free_cache_fail;
    midpage_active_map_alignment_skip +=
        stats.midpage_active_map_alignment_skip;
    midpage_active_map_route_bypass +=
        stats.midpage_active_map_route_bypass;
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);

  Hz6LinuxMmapRetainStats retain_stats =
      hz6_linux_mmap_retain_stats_snapshot();

  fprintf(stderr,
          "[HZ6_PRELOAD_STATS] allocators=%zu route_valid=%zu "
          "route_invalid=%zu route_miss=%zu transfer_push=%zu "
          "transfer_pop=%zu source_alloc=%zu toy_source_alloc=%zu "
          "midpage_source_alloc=%zu large_source_alloc=%zu "
          "local2p_source_alloc=%zu alloc_fail=%zu "
          "descriptor_exhausted=%zu route_register_fail=%zu "
          "source_block_exhausted=%zu source_prefill_attempt=%zu "
          "source_prefill_filled=%zu source_prefill_fallback=%zu "
          "front_source_prefill_alloc=%zu toy_source_prefill_call=%zu "
          "route_lookup_probe_total=%zu "
          "route_lookup_probe_max=%zu route_register_probe_total=%zu "
          "route_register_probe_max=%zu route_unregister_probe_total=%zu "
          "route_unregister_probe_max=%zu "
          "retain_reserve_calls=%zu retain_reserve_64k_calls=%zu "
          "retain_64k_take_hit=%zu retain_64k_take_miss=%zu "
          "retain_generic_take_hit=%zu retain_generic_take_miss=%zu "
          "retain_mmap_fallback=%zu retain_release_calls=%zu "
          "retain_release_64k_calls=%zu retain_64k_put_hit=%zu "
          "retain_64k_put_full=%zu retain_generic_put_hit=%zu "
          "retain_generic_put_full=%zu retain_munmap_fallback=%zu "
          "retain_retained_bytes=%zu retain_retained_64k_count=%zu\n",
          allocator_count, route_valid, route_invalid, route_miss,
          transfer_push, transfer_pop, source_alloc, toy_source_alloc,
          midpage_source_alloc, large_source_alloc, local2p_source_alloc,
          alloc_fail, descriptor_exhausted, route_register_fail,
          source_block_exhausted, source_prefill_attempt,
          source_prefill_filled, source_prefill_fallback,
          front_source_prefill_alloc, toy_source_prefill_call,
          route_lookup_probe_total,
          route_lookup_probe_max, route_register_probe_total,
          route_register_probe_max, route_unregister_probe_total,
          route_unregister_probe_max, retain_stats.reserve_calls,
          retain_stats.reserve_64k_calls, retain_stats.retain_64k_take_hit,
          retain_stats.retain_64k_take_miss,
          retain_stats.retain_generic_take_hit,
          retain_stats.retain_generic_take_miss, retain_stats.mmap_fallback,
          retain_stats.release_calls, retain_stats.release_64k_calls,
          retain_stats.retain_64k_put_hit, retain_stats.retain_64k_put_full,
          retain_stats.retain_generic_put_hit,
          retain_stats.retain_generic_put_full, retain_stats.munmap_fallback,
          retain_stats.retained_bytes, retain_stats.retained_64k_count);

  fprintf(stderr,
          "[HZ6_PRELOAD_ROUTE_DETAIL] "
          "route_register_reason_unknown=%zu "
          "route_register_reason_source_run_slot=%zu "
          "route_register_reason_direct_source=%zu "
          "route_register_reason_materialize=%zu "
          "route_register_reason_rehome=%zu "
          "route_unregister_reason_unknown=%zu "
          "route_unregister_reason_frontcache_overflow=%zu "
          "route_unregister_reason_cap_release=%zu "
          "route_unregister_reason_descriptorless_detach=%zu "
          "route_unregister_reason_source_slot_release=%zu "
          "route_unregister_reason_rehome=%zu "
          "route_active_current=%zu route_active_max=%zu "
          "route_tombstone_current=%zu route_tombstone_max=%zu "
          "route_register_used_tombstone=%zu "
          "route_register_full_probe_with_tombstone=%zu "
          "route_tombstone_compact_attempt=%zu "
          "route_tombstone_compact_success=%zu "
          "route_tombstone_compact_fail_alloc=%zu "
          "route_tombstone_compact_moved=%zu "
          "route_tombstone_cond_probe=%zu "
          "route_tombstone_cond_would_compact=%zu "
          "route_tombstone_cond_ratio25=%zu "
          "route_tombstone_cond_occupancy75=%zu "
          "route_tombstone_cond_cooldown_blocked=%zu "
          "route_tombstone_cond_highwater=%zu\n",
          route_register_reason_unknown,
          route_register_reason_source_run_slot,
          route_register_reason_direct_source,
          route_register_reason_materialize, route_register_reason_rehome,
          route_unregister_reason_unknown,
          route_unregister_reason_frontcache_overflow,
          route_unregister_reason_cap_release,
          route_unregister_reason_descriptorless_detach,
          route_unregister_reason_source_slot_release,
          route_unregister_reason_rehome, route_active_current,
          route_active_max, route_tombstone_current, route_tombstone_max,
          route_register_used_tombstone,
          route_register_full_probe_with_tombstone,
          route_tombstone_compact_attempt, route_tombstone_compact_success,
          route_tombstone_compact_fail_alloc,
          route_tombstone_compact_moved, route_tombstone_cond_probe,
          route_tombstone_cond_would_compact,
          route_tombstone_cond_ratio25, route_tombstone_cond_occupancy75,
          route_tombstone_cond_cooldown_blocked,
          route_tombstone_cond_highwater);

  fprintf(stderr,
          "[HZ6_PRELOAD_FRONT_DETAIL] descriptor_live_max=%zu "
          "source_block_active_max=%zu frontcache_total_max=%zu "
          "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
          "frontcache_spill_success=%zu "
          "frontcache_spill_retry_success=%zu "
          "frontcache_borrow_success=%zu "
          "toy_small_malloc_frontcache_pop=%zu "
          "toy_small_malloc_activate_success=%zu "
          "toy_small_free_cache_attempt=%zu "
          "toy_small_free_cache_success=%zu "
          "toy_small_active_map_register=%zu "
          "toy_small_active_map_register_collision=%zu "
          "toy_small_active_map_free_attempt=%zu "
          "toy_small_active_map_free_hit=%zu "
          "toy_small_active_map_free_miss=%zu "
          "toy_small_active_map_free_stale=%zu "
          "toy_small_active_map_free_cache_fail=%zu "
          "toy_small_active_map_route_bypass=%zu "
          "midpage_active_map_register=%zu "
          "midpage_active_map_register_collision=%zu "
          "midpage_active_map_free_attempt=%zu "
          "midpage_active_map_free_hit=%zu "
          "midpage_active_map_free_miss=%zu "
          "midpage_active_map_free_stale=%zu "
          "midpage_active_map_free_cache_fail=%zu "
          "midpage_active_map_alignment_skip=%zu "
          "midpage_active_map_route_bypass=%zu\n",
          descriptor_live_max, source_block_active_max,
          frontcache_total_max, frontcache_reuse_hit,
          frontcache_reuse_invalid, frontcache_spill_success,
          frontcache_spill_retry_success, frontcache_borrow_success,
          toy_small_malloc_frontcache_pop,
          toy_small_malloc_activate_success,
          toy_small_free_cache_attempt, toy_small_free_cache_success,
          toy_small_active_map_register,
          toy_small_active_map_register_collision,
          toy_small_active_map_free_attempt,
          toy_small_active_map_free_hit, toy_small_active_map_free_miss,
          toy_small_active_map_free_stale,
          toy_small_active_map_free_cache_fail,
          toy_small_active_map_route_bypass,
          midpage_active_map_register,
          midpage_active_map_register_collision,
          midpage_active_map_free_attempt,
          midpage_active_map_free_hit,
          midpage_active_map_free_miss,
          midpage_active_map_free_stale,
          midpage_active_map_free_cache_fail,
          midpage_active_map_alignment_skip,
          midpage_active_map_route_bypass);

  fprintf(stderr,
          "[HZ6_PRELOAD_PHASE_STATS] malloc_calls=%zu "
          "malloc_hz6_success=%zu malloc_real_fallback=%zu "
          "calloc_calls=%zu free_calls=%zu free_null=%zu "
          "free_reentry_real=%zu free_local_route_valid=%zu "
          "free_visible_route_hit=%zu free_toy_active_map_hit=%zu "
          "free_midpage_active_map_hit=%zu "
          "free_route_invalid=%zu free_route_miss_real=%zu "
          "free_prechecked_candidate=%zu "
          "realloc_calls=%zu realloc_owned=%zu realloc_in_place=%zu "
          "realloc_real_fallback=%zu realloc_copy_bytes=%zu "
          "malloc_usable_size_calls=%zu "
          "malloc_usable_size_owned=%zu "
          "malloc_usable_size_real_fallback=%zu\n",
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.malloc_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_hz6_success),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_real_fallback),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.calloc_calls),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_calls),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_null),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_reentry_real),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_local_route_valid),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_visible_route_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_toy_active_map_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_midpage_active_map_hit),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.free_route_invalid),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_route_miss_real),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.free_prechecked_candidate),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_calls),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_owned),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_in_place),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_real_fallback),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.realloc_copy_bytes),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_usable_size_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_usable_size_owned),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_usable_size_real_fallback));

  fprintf(stderr,
          "[HZ6_PRELOAD_SIZE_STATS] "
          "malloc_size_zero=%zu malloc_size_le1024=%zu "
          "malloc_size_1025_4096=%zu malloc_size_4097_16384=%zu "
          "malloc_size_gt16384=%zu "
          "realloc_request_zero=%zu realloc_request_le1024=%zu "
          "realloc_request_1025_4096=%zu "
          "realloc_request_4097_16384=%zu "
          "realloc_request_gt16384=%zu "
          "realloc_owned_old_zero=%zu "
          "realloc_owned_old_le1024=%zu "
          "realloc_owned_old_1025_4096=%zu "
          "realloc_owned_old_4097_16384=%zu "
          "realloc_owned_old_gt16384=%zu "
          "realloc_copy_calls=%zu\n",
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_size_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_request_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_owned_old_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_copy_calls));

  if (print_per_allocator) {
    pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
    for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
      Hz6Allocator* allocator = g_hz6_preload_allocator_registry[i];
      if (!allocator) {
        continue;
      }
      Hz6StatsSnapshot stats = hz6_stats_snapshot(allocator);
      fprintf(stderr,
              "[HZ6_PRELOAD_ALLOCATOR_STATS] index=%zu allocator=%p "
              "route_valid=%zu route_invalid=%zu route_miss=%zu "
              "transfer_push=%zu transfer_pop=%zu source_alloc=%zu "
              "toy_source_alloc=%zu midpage_source_alloc=%zu "
              "large_source_alloc=%zu local2p_source_alloc=%zu "
              "alloc_fail=%zu descriptor_exhausted=%zu "
              "route_register_fail=%zu source_block_exhausted=%zu\n",
              i, (void*)allocator, stats.route_valid, stats.route_invalid,
              stats.route_miss, stats.transfer_push, stats.transfer_pop,
              stats.source_alloc, stats.toy_source_alloc,
              stats.midpage_source_alloc, stats.large_source_alloc,
              stats.local2p_source_alloc, stats.alloc_fail,
              stats.descriptor_exhausted, stats.route_register_fail,
              stats.source_block_exhausted);
    }
    pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
  }

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
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_calls);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.malloc_size_zero,
      &g_hz6_preload_phase_stats.malloc_size_le1024,
      &g_hz6_preload_phase_stats.malloc_size_1025_4096,
      &g_hz6_preload_phase_stats.malloc_size_4097_16384,
      &g_hz6_preload_phase_stats.malloc_size_gt16384);
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  void* ptr = allocator ? hz6_malloc(allocator, size) : NULL;
  g_hz6_preload_reentry = 0;
  if (ptr) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_hz6_success);
    return ptr;
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_real_fallback);
  return hz6_preload_real_malloc(size);
}

void free(void* ptr) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_calls);
  if (!ptr) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_null);
    return;
  }
  if (g_hz6_preload_reentry) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_reentry_real);
    hz6_preload_real_free(ptr);
    return;
  }
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_hit);
    g_hz6_preload_reentry = 0;
    return;
  }
  if (hz6_midpage_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
    g_hz6_preload_reentry = 0;
    return;
  }
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
  if (preload_route.route.kind == HZ6_ROUTE_VALID ||
      preload_route.route.kind == HZ6_ROUTE_INVALID) {
    if (preload_route.route.kind == HZ6_ROUTE_VALID) {
      if (preload_route.visible_hit) {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_visible_route_hit);
      } else {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_local_route_valid);
        if (preload_route.route.route_allocator == allocator) {
          hz6_preload_phase_count(
              &g_hz6_preload_phase_stats.free_prechecked_candidate);
        }
      }
    } else {
      hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_route_invalid);
    }
#if HZ6_PRELOAD_FAST_FREE_L1
    hz6_free_with_route_prechecked(allocator, ptr, preload_route.route,
                                   preload_route.visible_hit);
#else
    hz6_free(allocator, ptr);
#endif
  } else {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_route_miss_real);
    hz6_preload_real_free(ptr);
  }
  g_hz6_preload_reentry = 0;
}

void* calloc(size_t nmemb, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.calloc_calls);
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
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_calls);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.realloc_request_zero,
      &g_hz6_preload_phase_stats.realloc_request_le1024,
      &g_hz6_preload_phase_stats.realloc_request_1025_4096,
      &g_hz6_preload_phase_stats.realloc_request_4097_16384,
      &g_hz6_preload_phase_stats.realloc_request_gt16384);
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
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
  size_t old_size = 0;
  if (preload_route.route.kind == HZ6_ROUTE_VALID &&
      preload_route.route.descriptor) {
    const Hz6ObjectDescriptor* descriptor =
        (const Hz6ObjectDescriptor*)preload_route.route.descriptor;
    old_size = descriptor->bytes;
  }
  g_hz6_preload_reentry = 0;
  if (old_size == 0) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_real_fallback);
    return hz6_preload_real_realloc(ptr, size);
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_owned);
  hz6_preload_phase_count_size_bucket(
      old_size,
      &g_hz6_preload_phase_stats.realloc_owned_old_zero,
      &g_hz6_preload_phase_stats.realloc_owned_old_le1024,
      &g_hz6_preload_phase_stats.realloc_owned_old_1025_4096,
      &g_hz6_preload_phase_stats.realloc_owned_old_4097_16384,
      &g_hz6_preload_phase_stats.realloc_owned_old_gt16384);
#if HZ6_PRELOAD_REALLOC_IN_PLACE_L1
  if (size <= old_size) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_in_place);
    return ptr;
  }
#endif

  void* next = malloc(size);
  if (!next) {
    return NULL;
  }
  size_t copy_bytes = old_size < size ? old_size : size;
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_copy_calls);
  hz6_preload_phase_add(&g_hz6_preload_phase_stats.realloc_copy_bytes,
                        copy_bytes);
  memcpy(next, ptr, copy_bytes);
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
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_usable_size_calls);
  if (!ptr) {
    return 0;
  }
  if (!g_hz6_preload_reentry) {
    g_hz6_preload_reentry = 1;
    Hz6Allocator* allocator = hz6_preload_allocator();
    size_t bytes = hz6_preload_usable_size(allocator, ptr);
    g_hz6_preload_reentry = 0;
    if (bytes != 0) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.malloc_usable_size_owned);
      return bytes;
    }
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.malloc_usable_size_real_fallback);
  if (g_hz6_preload_reentry && !g_real_malloc_usable_size) {
    return 0;
  }
  hz6_preload_ensure_real();
  return g_real_malloc_usable_size ? g_real_malloc_usable_size(ptr) : 0;
}
