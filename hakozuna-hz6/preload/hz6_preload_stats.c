/* LD_PRELOAD stats registry, aggregation, and diagnostic printing. */
#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_api_scavenge.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_toy_small_diag.h"
#include "hz6_midpage_front.h"
#include "linux_source_mmap.h"
#include "hz6_preload_real.h"
#include "hz6_preload_stats.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>

#define HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY 256u

static pthread_mutex_t g_hz6_preload_allocator_registry_mutex =
    PTHREAD_MUTEX_INITIALIZER;
static Hz6Allocator*
    g_hz6_preload_allocator_registry[HZ6_PRELOAD_ALLOCATOR_REGISTRY_CAPACITY];
static size_t g_hz6_preload_allocator_registry_count;

Hz6PreloadPhaseStats g_hz6_preload_phase_stats;
static int g_hz6_preload_phase_stats_enabled;

static size_t hz6_preload_phase_load(const _Atomic(size_t)* counter) {
  return atomic_load_explicit(counter, memory_order_relaxed);
}

#define HZ6_PRELOAD_PHASE_LOAD_FIELD(name) hz6_preload_phase_load(&g_hz6_preload_phase_stats.name)

#if !HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
void hz6_preload_phase_count(_Atomic(size_t)* counter) {
  if (g_hz6_preload_phase_stats_enabled) {
    (void)atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
  }
}

void hz6_preload_phase_add(_Atomic(size_t)* counter, size_t value) {
  if (g_hz6_preload_phase_stats_enabled && value != 0) {
    (void)atomic_fetch_add_explicit(counter, value, memory_order_relaxed);
  }
}

void hz6_preload_phase_count_size_bucket(
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
#endif

__attribute__((constructor)) static void hz6_preload_on_load(void) {
  const char* value = getenv("HZ6_PRELOAD_STATS");
  g_hz6_preload_phase_stats_enabled =
      value && value[0] != '\0' && strcmp(value, "0") != 0;
}

void hz6_preload_register_allocator(Hz6Allocator* allocator) {
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

size_t hz6_preload_scavenge_local_free(size_t max_bytes) {
  if (max_bytes == 0) {
    max_bytes = (size_t)-1;
  }

  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;

  size_t released = 0;
  pthread_mutex_lock(&g_hz6_preload_allocator_registry_mutex);
  for (size_t i = 0; i < g_hz6_preload_allocator_registry_count; ++i) {
    Hz6Allocator* allocator = g_hz6_preload_allocator_registry[i];
    if (allocator) {
      released += hz6_allocator_scavenge_local_free(allocator, max_bytes);
    }
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);

  g_hz6_preload_reentry = saved_reentry;
  return released;
}

int hz6_preload_quiescent_release(size_t max_bytes) {
  size_t released_objects = hz6_preload_scavenge_local_free(max_bytes);
  size_t flushed_bytes = hz6_linux_mmap_retain_flush(max_bytes);
  return released_objects != 0 || flushed_bytes != 0;
}

static void hz6_preload_print_stats(void) {
#include "hz6_preload_stats_print_head.inc"
#include "hz6_preload_stats_print_mid.inc"
#include "hz6_preload_stats_print_tail.inc"
}
__attribute__((destructor)) static void hz6_preload_on_unload(void) {
  hz6_preload_print_stats();
}
