#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_toy_small_diag.h"
#include "hz6_midpage_front.h"
#include "hz6_profiles.h"
#include "hz6_preload_real.h"
#include "hz6_preload_stats.h"

#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Hz6PreloadRoute {
  Hz6RouteResult route;
  int visible_hit;
} Hz6PreloadRoute;

#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_PRELOAD_UNLIKELY(expr) (expr)
#endif

static __thread Hz6Allocator* g_hz6_preload_allocator;

#if HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1 ||   \
    HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1 || \
    HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1
#define HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1 1
#else
#define HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1 0
#endif

#if HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1 || \
    HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1
static __thread uintptr_t
    g_hz6_preload_midpage_page_hint
        [HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY];

static size_t hz6_preload_midpage_page_hint_slot(uintptr_t page) {
  uintptr_t mixed = page ^ (page >> 17) ^ (page >> 29);
  return (size_t)(mixed %
                  HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY);
}

static void hz6_preload_midpage_page_hint_note_one(uintptr_t page) {
  uintptr_t key = page + 1u;
  size_t slot = hz6_preload_midpage_page_hint_slot(page);
  for (size_t probe = 0;
       probe < HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_PROBE_LIMIT; ++probe) {
    uintptr_t* entry =
        &g_hz6_preload_midpage_page_hint
             [(slot + probe) % HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY];
    if (*entry == 0 || *entry == key) {
      *entry = key;
      return;
    }
  }
  g_hz6_preload_midpage_page_hint[slot] = key;
}

static void hz6_preload_midpage_hint_note(const void* ptr, size_t size) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base_page = addr >> 12;
  uintptr_t last_page = (addr + size - 1u) >> 12;
  hz6_preload_midpage_page_hint_note_one(base_page);
  if (last_page != base_page) {
    hz6_preload_midpage_page_hint_note_one(last_page);
  }
}

static int hz6_preload_midpage_hint_maybe(const void* ptr) {
  if (!ptr) {
    return 0;
  }
  uintptr_t page = ((uintptr_t)ptr) >> 12;
  uintptr_t key = page + 1u;
  size_t slot = hz6_preload_midpage_page_hint_slot(page);
  for (size_t probe = 0;
       probe < HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_PROBE_LIMIT; ++probe) {
    uintptr_t entry =
        g_hz6_preload_midpage_page_hint
            [(slot + probe) % HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY];
    if (entry == key) {
      return 1;
    }
    if (entry == 0) {
      return 0;
    }
  }
  return 0;
}
#elif HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1
static __thread uintptr_t g_hz6_preload_midpage_hint_min;
static __thread uintptr_t g_hz6_preload_midpage_hint_max;

static void hz6_preload_midpage_hint_note(const void* ptr, size_t size) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t min = addr & ~((uintptr_t)4095u);
  uintptr_t max = (addr + size - 1u) | (uintptr_t)4095u;
  if (g_hz6_preload_midpage_hint_min == 0 ||
      min < g_hz6_preload_midpage_hint_min) {
    g_hz6_preload_midpage_hint_min = min;
  }
  if (max > g_hz6_preload_midpage_hint_max) {
    g_hz6_preload_midpage_hint_max = max;
  }
}

static int hz6_preload_midpage_hint_maybe(const void* ptr) {
  if (!ptr || g_hz6_preload_midpage_hint_min == 0) {
    return 0;
  }
  uintptr_t addr = (uintptr_t)ptr;
  return addr >= g_hz6_preload_midpage_hint_min &&
         addr <= g_hz6_preload_midpage_hint_max;
}
#endif

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

#if HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1
static int hz6_preload_midpage_current_bias_first(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }
#if !HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2 || !HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  return 0;
#elif HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FAST_L1
  return allocator->midpage_active_map_current >
         allocator->toy_small_active_map_current;
#else
  size_t denominator =
      HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DENOMINATOR == 0
          ? (size_t)1
          : HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DENOMINATOR;
  size_t toy_scaled =
      allocator->toy_small_active_map_current *
      HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR;
  size_t mid_scaled = allocator->midpage_active_map_current * denominator;
  return mid_scaled >
         toy_scaled + HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DELTA;
#endif
}
#endif

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

#if HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_NOINLINE __attribute__((noinline))
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_NOINLINE
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_UNLIKELY(expr) (expr)
#endif

static HZ6_PRELOAD_MIDPAGE_BOUNDARY_NOINLINE void*
hz6_preload_malloc_midpage_boundary(Hz6Allocator* allocator, size_t size) {
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.malloc_midpage_boundary_attempt);
  void* ptr =
      hz6_allocator_preload_midpage_malloc_skip_transfer(allocator, size);
  if (ptr) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_midpage_boundary_hit);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    hz6_preload_midpage_hint_note(ptr, size);
#endif
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_midpage_boundary_fallback);
  }
  return ptr;
}
#endif

static void* hz6_preload_malloc_hz6(Hz6Allocator* allocator, size_t size) {
#if HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1
#if HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
  if (HZ6_PRELOAD_MIDPAGE_BOUNDARY_UNLIKELY(
          size > HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES &&
                                            size <= HZ6_MIDPAGE_BYTES)) {
    return hz6_preload_malloc_midpage_boundary(allocator, size);
  }
#else
  if (size > HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_MIN_BYTES &&
      size <= HZ6_MIDPAGE_BYTES) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_midpage_boundary_attempt);
    void* ptr =
        hz6_allocator_preload_midpage_malloc_skip_transfer(allocator, size);
    if (ptr) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.malloc_midpage_boundary_hit);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
      hz6_preload_midpage_hint_note(ptr, size);
#endif
    } else {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.malloc_midpage_boundary_fallback);
    }
    return ptr;
  }
#endif
#endif
  return hz6_malloc(allocator, size);
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
  void* ptr = allocator ? hz6_preload_malloc_hz6(allocator, size) : NULL;
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
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
  int midpage_hint_would_first = hz6_preload_midpage_hint_maybe(ptr);
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_midpage_hint_probe);
  if (midpage_hint_would_first) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_hint_would_first);
  }
#endif
#if HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1
  if (midpage_hint_would_first) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_true_midpage);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_false_positive);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_missed_midpage);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#elif HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
  if (hz6_midpage_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
  if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_hit);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    if (midpage_hint_would_first) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_false_positive);
    }
#endif
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_toy_active_map_miss);
#elif HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1
  if (hz6_midpage_active_map_aligned(ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#elif HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1
  if (HZ6_PRELOAD_UNLIKELY(
          hz6_preload_midpage_current_bias_first(allocator))) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#else
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
  if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_hit);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    if (midpage_hint_would_first) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_false_positive);
    }
#endif
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_toy_active_map_miss);
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
  if (hz6_midpage_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    if (midpage_hint_would_first) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_true_midpage);
    } else {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_missed_midpage);
    }
#endif
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
#endif
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_lookup_after_maps);
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
  if (preload_route.route.kind == HZ6_ROUTE_VALID ||
      preload_route.route.kind == HZ6_ROUTE_INVALID) {
#if HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
    int midpage_fast_free = 0;
#endif
    if (preload_route.route.kind == HZ6_ROUTE_VALID) {
      if (preload_route.visible_hit) {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_visible_route_hit);
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_route_valid_foreign_visible);
      } else {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_local_route_valid);
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_route_valid_owned);
        if (preload_route.route.route_allocator == allocator) {
          hz6_preload_phase_count(
              &g_hz6_preload_phase_stats.free_prechecked_candidate);
#if HZ6_DIAGNOSTIC_PROBES
          if (preload_route.route.front_id == HZ6_FRONT_MIDPAGE) {
            if (preload_route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
              ++allocator->stats.midpage_8k_preload_local_route_valid;
            } else if (preload_route.route.class_id ==
                       HZ6_MIDPAGE_32K_CLASS_ID) {
              ++allocator->stats.midpage_32k_preload_local_route_valid;
            }
          }
#endif
#if HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
          midpage_fast_free = hz6_midpage_active_map_eligible(
              preload_route.route.front_id, preload_route.route.class_id);
#endif
#if HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1
          hz6_midpage_active_map_register(
              allocator, preload_route.route.front_id,
              preload_route.route.class_id, ptr,
              (Hz6ObjectDescriptor*)preload_route.route.descriptor);
#endif
        }
      }
    } else {
      hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_route_invalid);
    }
#if HZ6_PRELOAD_FAST_FREE_L1
    hz6_free_with_route_prechecked(allocator, ptr, preload_route.route,
                                   preload_route.visible_hit);
#elif HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
    if (midpage_fast_free) {
      hz6_free_with_route_prechecked(allocator, ptr, preload_route.route,
                                     preload_route.visible_hit);
    } else {
      hz6_free(allocator, ptr);
    }
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
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_aligned_alloc(alignment, size);
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
  return hz6_preload_real_malloc_usable_size(ptr);
}
