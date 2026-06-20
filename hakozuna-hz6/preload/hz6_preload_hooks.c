#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_page_kind.h"
#include "hz6_allocator_toy_small_diag.h"
#include "hz6_midpage_front.h"
#include "hz6_profiles.h"
#include "hz6_preload_midpage.h"
#include "hz6_preload_real.h"
#include "hz6_preload_stats.h"

#include "../fronts/hz6_front.h"

#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

typedef struct Hz6PreloadRoute {
  Hz6RouteResult route;
  Hz6FreeRouteResolveKind resolve_kind;
  int visible_hit;
} Hz6PreloadRoute;

#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_PRELOAD_UNLIKELY(expr) (expr)
#endif

static __thread Hz6Allocator* g_hz6_preload_allocator;

#if HZ6_PRELOAD_FOREIGN_ROUTE_BEFORE_MAPS_ARMED_L1
static __thread unsigned char g_hz6_preload_foreign_route_before_maps_armed;
#endif

#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1
static __thread unsigned char g_hz6_preload_realloc_boundary_adapt_4k;
#endif

#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1
static __thread unsigned char g_hz6_preload_realloc_boundary_adapt_8k;
#endif

#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1 || \
    HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1
#define HZ6_PRELOAD_REAL_ALIGNED_TOMBSTONE ((void*)(uintptr_t)1u)
#define HZ6_PRELOAD_REAL_PTR_KIND_ALIGNED 1u
#define HZ6_PRELOAD_REAL_PTR_KIND_CALLOC 2u
static pthread_mutex_t g_hz6_preload_real_aligned_mutex =
    PTHREAD_MUTEX_INITIALIZER;
static void* g_hz6_preload_real_aligned_ptrs
    [HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY];
static unsigned char g_hz6_preload_real_aligned_kinds
    [HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY];
static atomic_size_t g_hz6_preload_real_aligned_count;

static size_t hz6_preload_real_aligned_hash(void* ptr) {
  uintptr_t value = (uintptr_t)ptr >> 4;
  value ^= value >> 33;
  value *= (uintptr_t)0xff51afd7ed558ccdULL;
  value ^= value >> 33;
  return (size_t)value;
}

static int hz6_preload_real_pointer_record(void* ptr, unsigned char kind) {
  if (!ptr || HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY == 0) {
    return 0;
  }
  size_t base = hz6_preload_real_aligned_hash(ptr) %
                HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
  size_t first_tombstone = HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
  int recorded = 0;

  pthread_mutex_lock(&g_hz6_preload_real_aligned_mutex);
  for (size_t probe = 0; probe < HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
       ++probe) {
    size_t index =
        (base + probe) % HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
    void* entry = g_hz6_preload_real_aligned_ptrs[index];
    if (entry == ptr) {
      g_hz6_preload_real_aligned_kinds[index] = kind;
      recorded = 1;
      break;
    }
    if (entry == HZ6_PRELOAD_REAL_ALIGNED_TOMBSTONE) {
      if (first_tombstone == HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY) {
        first_tombstone = index;
      }
      continue;
    }
    if (!entry) {
      size_t target =
          first_tombstone == HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY
              ? index
              : first_tombstone;
      g_hz6_preload_real_aligned_ptrs[target] = ptr;
      g_hz6_preload_real_aligned_kinds[target] = kind;
      recorded = 1;
      (void)atomic_fetch_add_explicit(&g_hz6_preload_real_aligned_count, 1u,
                                      memory_order_relaxed);
      break;
    }
  }
  pthread_mutex_unlock(&g_hz6_preload_real_aligned_mutex);

  if (kind == HZ6_PRELOAD_REAL_PTR_KIND_CALLOC) {
    hz6_preload_phase_count(
        recorded ? &g_hz6_preload_phase_stats.calloc_real_record_set
                 : &g_hz6_preload_phase_stats.calloc_real_record_fail);
  } else if (recorded) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.real_aligned_record_set);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.real_aligned_record_fail);
  }
  return recorded;
}

static unsigned char hz6_preload_real_pointer_forget(void* ptr) {
  if (!ptr || HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY == 0) {
    return 0;
  }
  size_t base = hz6_preload_real_aligned_hash(ptr) %
                HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
  unsigned char kind = 0;

  pthread_mutex_lock(&g_hz6_preload_real_aligned_mutex);
  for (size_t probe = 0; probe < HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
       ++probe) {
    size_t index =
        (base + probe) % HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
    void* entry = g_hz6_preload_real_aligned_ptrs[index];
    if (!entry) {
      break;
    }
    if (entry == ptr) {
      kind = g_hz6_preload_real_aligned_kinds[index];
      g_hz6_preload_real_aligned_ptrs[index] =
          HZ6_PRELOAD_REAL_ALIGNED_TOMBSTONE;
      g_hz6_preload_real_aligned_kinds[index] = 0;
      (void)atomic_fetch_sub_explicit(&g_hz6_preload_real_aligned_count, 1u,
                                      memory_order_relaxed);
      break;
    }
  }
  pthread_mutex_unlock(&g_hz6_preload_real_aligned_mutex);
  return kind;
}
#endif

#if HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
#define hz6_preload_phase_count_alignment_bucket( \
    alignment, le16, range17_64, range65_4096, gt4096) \
  ((void)0)
#else
static void hz6_preload_phase_count_alignment_bucket(
    size_t alignment,
    _Atomic(size_t)* le16,
    _Atomic(size_t)* range17_64,
    _Atomic(size_t)* range65_4096,
    _Atomic(size_t)* gt4096) {
  if (alignment <= 16u) {
    hz6_preload_phase_count(le16);
  } else if (alignment <= 64u) {
    hz6_preload_phase_count(range17_64);
  } else if (alignment <= 4096u) {
    hz6_preload_phase_count(range65_4096);
  } else {
    hz6_preload_phase_count(gt4096);
  }
}
#endif

#if HZ6_PAGE_KIND_FREE_SELECTOR_ACTIVE_L1
static uint16_t hz6_preload_page_kind_selector_probe(Hz6Allocator* allocator,
                                                     const void* ptr) {
  uint16_t kind = hz6_allocator_page_kind_lookup(allocator, ptr);
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_page_kind_selector_probe);
  if (kind == HZ6_PAGE_KIND_TOY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_page_kind_selector_toy);
  } else if (kind == HZ6_PAGE_KIND_MIDPAGE) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_page_kind_selector_midpage);
  } else if (kind == HZ6_PAGE_KIND_MIXED) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_page_kind_selector_mixed);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_page_kind_selector_unknown);
  }
  return kind;
}

static void hz6_preload_page_kind_selector_note_toy_hit(uint16_t kind) {
  if (kind == HZ6_PAGE_KIND_TOY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_page_kind_selector_toy_hit);
  } else if (kind == HZ6_PAGE_KIND_MIDPAGE) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats
                                 .free_page_kind_selector_wrong_midpage_page_toy_hit);
  }
}

static void hz6_preload_page_kind_selector_note_midpage_hit(uint16_t kind) {
  if (kind == HZ6_PAGE_KIND_MIDPAGE) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_page_kind_selector_midpage_hit);
  } else if (kind == HZ6_PAGE_KIND_TOY) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats
                                 .free_page_kind_selector_wrong_toy_page_mid_hit);
  }
}
#else
#define hz6_preload_page_kind_selector_probe(allocator, ptr) \
  (HZ6_PAGE_KIND_UNKNOWN)
#define hz6_preload_page_kind_selector_note_toy_hit(kind) ((void)(kind))
#define hz6_preload_page_kind_selector_note_midpage_hit(kind) ((void)(kind))
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
  preload_route.resolve_kind = HZ6_FREE_ROUTE_PROVEN_EXTERNAL;
  if (!allocator || !ptr) {
    return preload_route;
  }
#if HZ6_REMOTE_FREE_ROUTE_RESOLVE_L1
  Hz6FreeRouteResolveResult resolved =
      hz6_allocator_route_resolve_free(allocator, ptr);
  preload_route.route = resolved.route;
  preload_route.resolve_kind = resolved.kind;
  preload_route.visible_hit = resolved.visible_hit;
  return preload_route;
#else
  preload_route.route = hz6_allocator_route_lookup(allocator, ptr);
  if (preload_route.route.kind == HZ6_ROUTE_MISS) {
    preload_route.route =
        hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
    preload_route.visible_hit =
        preload_route.route.kind != HZ6_ROUTE_MISS ? 1 : 0;
  }
  if (preload_route.route.kind == HZ6_ROUTE_VALID) {
    preload_route.resolve_kind = preload_route.visible_hit
                                     ? HZ6_FREE_ROUTE_FOREIGN_VALID
                                     : HZ6_FREE_ROUTE_LOCAL_VALID;
  } else if (preload_route.route.kind == HZ6_ROUTE_INVALID) {
    preload_route.resolve_kind = HZ6_FREE_ROUTE_OWNED_INVALID;
  }
  return preload_route;
#endif
}

static int hz6_preload_route_real_fallback_allowed(Hz6PreloadRoute route) {
  return route.resolve_kind == HZ6_FREE_ROUTE_PROVEN_EXTERNAL;
}

static void hz6_preload_route_fail_fast_if_integrity(Hz6PreloadRoute route) {
  if (route.resolve_kind == HZ6_FREE_ROUTE_RETRY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_retry_abort);
    abort();
  }
  if (route.resolve_kind == HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_integrity_abort);
    abort();
  }
}

#if HZ6_PRELOAD_FOREIGN_ROUTE_BEFORE_MAPS_L1
static int hz6_preload_foreign_route_before_maps_try(Hz6Allocator* allocator,
                                                     void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }
#if HZ6_PRELOAD_FOREIGN_ROUTE_BEFORE_MAPS_ARMED_L1
  if (!g_hz6_preload_foreign_route_before_maps_armed) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_before_maps_arm_skip);
    return 0;
  }
#endif
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_before_maps_attempt);
  Hz6PreloadRoute before_maps_route = hz6_preload_route(allocator, ptr);
  if (before_maps_route.resolve_kind == HZ6_FREE_ROUTE_FOREIGN_VALID &&
      before_maps_route.route.kind == HZ6_ROUTE_VALID &&
      before_maps_route.visible_hit) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_before_maps_foreign_dispatch);
    hz6_free_with_resolved_route_after_maps(allocator, ptr,
                                            before_maps_route.route,
                                            before_maps_route.visible_hit);
    return 1;
  }
#if HZ6_PRELOAD_ROUTE_BEFORE_MAPS_LOCAL_DISPATCH_L1
  if (before_maps_route.resolve_kind == HZ6_FREE_ROUTE_LOCAL_VALID &&
      before_maps_route.route.kind == HZ6_ROUTE_VALID &&
      !before_maps_route.visible_hit) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_before_maps_local_dispatch);
    hz6_free_with_resolved_route_after_maps(allocator, ptr,
                                            before_maps_route.route,
                                            before_maps_route.visible_hit);
    return 1;
  }
#endif
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_before_maps_fallback);
  if (before_maps_route.resolve_kind == HZ6_FREE_ROUTE_LOCAL_VALID) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats
                                 .free_route_before_maps_fallback_local_valid);
  } else if (before_maps_route.resolve_kind == HZ6_FREE_ROUTE_OWNED_INVALID) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats
                                 .free_route_before_maps_fallback_owned_invalid);
  } else if (before_maps_route.resolve_kind ==
             HZ6_FREE_ROUTE_PROVEN_EXTERNAL) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats
             .free_route_before_maps_fallback_proven_external);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_before_maps_fallback_other);
  }
  return 0;
}
#endif

#if HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1 || \
    HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1
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

static size_t hz6_preload_usable_size_from_route(
    Hz6PreloadRoute preload_route) {
  if (preload_route.route.kind != HZ6_ROUTE_VALID ||
      !preload_route.route.descriptor) {
    return 0;
  }
  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)preload_route.route.descriptor;
  return descriptor->bytes;
}

#if !HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
static void hz6_preload_note_realloc_copy_class(Hz6PreloadRoute route,
                                                size_t new_size) {
  uint16_t new_class_id = 0;
  const Hz6FrontOps* new_front =
      hz6_front_for_allocation(new_size, 16, &new_class_id);
  if (new_front && new_front->front_id == route.route.front_id &&
      new_class_id == route.route.class_id) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.realloc_copy_same_class);
    return;
  }

  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.realloc_copy_cross_class);
  if (route.route.front_id == HZ6_FRONT_TOY &&
      new_front && new_front->front_id == HZ6_FRONT_MIDPAGE) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.realloc_copy_boundary_toy_to_midpage);
  } else if (route.route.front_id == HZ6_FRONT_MIDPAGE &&
             route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID &&
             new_front && new_front->front_id == HZ6_FRONT_MIDPAGE &&
             new_class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.realloc_copy_boundary_mid8_to_mid32);
  } else if (route.route.front_id == HZ6_FRONT_MIDPAGE &&
             new_front && new_front->front_id == HZ6_FRONT_LARGE) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.realloc_copy_boundary_midpage_to_large);
  }
}
#else
#define hz6_preload_note_realloc_copy_class(route, new_size) ((void)0)
#endif

#if !HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
static void hz6_preload_count_toy_direct_class_bucket(
    _Atomic(size_t)* total,
    _Atomic(size_t)* le1024,
    _Atomic(size_t)* range1025_4096,
    size_t size) {
  if (size == 0 || size > 4096u) {
    return;
  }
  hz6_preload_phase_count(total);
  if (size <= 1024u) {
    hz6_preload_phase_count(le1024);
  } else {
    hz6_preload_phase_count(range1025_4096);
  }
}
#endif

#if HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_TOY_BOUNDARY_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_PRELOAD_TOY_BOUNDARY_UNLIKELY(expr) (expr)
#endif
#endif

static void* hz6_preload_malloc_hz6(Hz6Allocator* allocator, size_t size) {
#if (HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1 || \
     HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1 || \
     HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1 || \
     HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1) && \
    HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1
#if HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1 || \
    HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1
  if (size == 4096u
#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1 && \
    !HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1
      && g_hz6_preload_realloc_boundary_adapt_4k
#endif
  ) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_realloc_boundary_slack_4k);
#if HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
    return hz6_preload_malloc_midpage_boundary(allocator, 4097u);
#else
    size = 4097u;
#endif
  }
#endif
#if HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1 || \
    HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1
  if (size == HZ6_MIDPAGE_8K_BYTES
#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1 && \
    !HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1
      && g_hz6_preload_realloc_boundary_adapt_8k
#endif
  ) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_realloc_boundary_slack_8k);
#if HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
    return hz6_preload_malloc_midpage_boundary(allocator,
                                               HZ6_MIDPAGE_8K_BYTES + 1u);
#else
    size = HZ6_MIDPAGE_8K_BYTES + 1u;
#endif
  }
#endif
#endif
#if !HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
  hz6_preload_count_toy_direct_class_bucket(
      &g_hz6_preload_phase_stats.malloc_toy_direct_class_eligible,
      &g_hz6_preload_phase_stats.malloc_toy_direct_class_eligible_le1024,
      &g_hz6_preload_phase_stats
           .malloc_toy_direct_class_eligible_1025_4096,
      size);
#endif
#if HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1
  if (HZ6_PRELOAD_TOY_BOUNDARY_UNLIKELY(
          size != 0 &&
          size >= HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MIN_BYTES &&
          size <= HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES &&
          size <= 4096u)) {
#if !HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
    hz6_preload_count_toy_direct_class_bucket(
        &g_hz6_preload_phase_stats.malloc_toy_direct_class_enter,
        &g_hz6_preload_phase_stats.malloc_toy_direct_class_enter_le1024,
        &g_hz6_preload_phase_stats
             .malloc_toy_direct_class_enter_1025_4096,
        size);
#endif
    return hz6_allocator_preload_toy_malloc_direct_class(allocator, size);
  }
#endif
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
#if HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 && \
    HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1
    uint16_t class_id = size <= HZ6_MIDPAGE_8K_BYTES
                            ? HZ6_MIDPAGE_8K_CLASS_ID
                            : HZ6_MIDPAGE_32K_CLASS_ID;
    void* ptr = hz6_allocator_preload_midpage_malloc_class_skip_transfer(
        allocator, class_id, size);
#else
    void* ptr =
        hz6_allocator_preload_midpage_malloc_skip_transfer(allocator, size);
#endif
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

#if HZ6_PRELOAD_CALLOC_DIRECT_HZ6_L1
static void* hz6_preload_calloc_malloc_bytes(size_t bytes) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_calls);
  hz6_preload_phase_count_size_bucket(
      bytes,
      &g_hz6_preload_phase_stats.malloc_size_zero,
      &g_hz6_preload_phase_stats.malloc_size_le1024,
      &g_hz6_preload_phase_stats.malloc_size_1025_4096,
      &g_hz6_preload_phase_stats.malloc_size_4097_16384,
      &g_hz6_preload_phase_stats.malloc_size_gt16384);
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  void* ptr = allocator ? hz6_preload_malloc_hz6(allocator, bytes) : NULL;
  g_hz6_preload_reentry = 0;
  if (ptr) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_hz6_success);
    return ptr;
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_real_fallback);
  return hz6_preload_real_malloc(bytes);
}
#else
#define hz6_preload_calloc_malloc_bytes(bytes) malloc(bytes)
#endif

#include "hz6_preload_hooks_tail.inc"
