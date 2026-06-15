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
  int visible_hit;
} Hz6PreloadRoute;

#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_PRELOAD_UNLIKELY(expr) (expr)
#endif

static __thread Hz6Allocator* g_hz6_preload_allocator;

#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1
static __thread unsigned char g_hz6_preload_realloc_boundary_adapt_4k;
#endif

#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1
static __thread unsigned char g_hz6_preload_realloc_boundary_adapt_8k;
#endif

#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
#define HZ6_PRELOAD_REAL_ALIGNED_TOMBSTONE ((void*)(uintptr_t)1u)
static pthread_mutex_t g_hz6_preload_real_aligned_mutex =
    PTHREAD_MUTEX_INITIALIZER;
static void* g_hz6_preload_real_aligned_ptrs
    [HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY];
static atomic_size_t g_hz6_preload_real_aligned_count;

static size_t hz6_preload_real_aligned_hash(void* ptr) {
  uintptr_t value = (uintptr_t)ptr >> 4;
  value ^= value >> 33;
  value *= (uintptr_t)0xff51afd7ed558ccdULL;
  value ^= value >> 33;
  return (size_t)value;
}

static int hz6_preload_real_aligned_record(void* ptr) {
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
      recorded = 1;
      (void)atomic_fetch_add_explicit(&g_hz6_preload_real_aligned_count, 1u,
                                      memory_order_relaxed);
      break;
    }
  }
  pthread_mutex_unlock(&g_hz6_preload_real_aligned_mutex);

  if (recorded) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.real_aligned_record_set);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.real_aligned_record_fail);
  }
  return recorded;
}

static int hz6_preload_real_aligned_forget(void* ptr) {
  if (!ptr || HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY == 0) {
    return 0;
  }
  size_t base = hz6_preload_real_aligned_hash(ptr) %
                HZ6_PRELOAD_REAL_ALIGNED_PTR_TABLE_CAPACITY;
  int found = 0;

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
      g_hz6_preload_real_aligned_ptrs[index] =
          HZ6_PRELOAD_REAL_ALIGNED_TOMBSTONE;
      (void)atomic_fetch_sub_explicit(&g_hz6_preload_real_aligned_count, 1u,
                                      memory_order_relaxed);
      found = 1;
      break;
    }
  }
  pthread_mutex_unlock(&g_hz6_preload_real_aligned_mutex);
  return found;
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
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
  if (atomic_load_explicit(&g_hz6_preload_real_aligned_count,
                           memory_order_relaxed) != 0) {
    if (hz6_preload_real_aligned_forget(ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.real_aligned_free_skip_hit);
      hz6_preload_real_free(ptr);
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.real_aligned_free_skip_miss);
  }
#endif
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  uint16_t page_kind = hz6_preload_page_kind_selector_probe(allocator, ptr);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
  int midpage_hint_would_first = hz6_preload_midpage_hint_maybe(ptr);
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_midpage_hint_probe);
  if (midpage_hint_would_first) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_hint_would_first);
  }
#endif
#if HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1
  if (page_kind == HZ6_PAGE_KIND_MIDPAGE) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else if (page_kind == HZ6_PAGE_KIND_TOY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  } else if (HZ6_PRELOAD_UNLIKELY(
                 hz6_preload_midpage_current_bias_first(allocator))) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#elif HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1
  if (midpage_hint_would_first) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
    hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
    hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
    hz6_preload_page_kind_selector_note_toy_hit(page_kind);
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
    hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
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
#if HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 || \
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
  int source_run_route_prechecked = 0;
  Hz6PreloadRoute source_run_preload_route = {0};
  source_run_preload_route.route = hz6_route_miss();
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_source_run_route_attempt);
  source_run_preload_route.route =
      hz6_allocator_source_block_route_lookup(allocator, ptr);
  if (source_run_preload_route.route.kind == HZ6_ROUTE_VALID &&
      source_run_preload_route.route.route_allocator == allocator &&
      source_run_preload_route.route.front_id == HZ6_FRONT_MIDPAGE &&
      (source_run_preload_route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID ||
       source_run_preload_route.route.class_id == HZ6_MIDPAGE_32K_CLASS_ID)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_source_run_route_hit);
    if (source_run_preload_route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_source_run_route_midpage8_hit);
    } else {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_source_run_route_midpage32_hit);
    }
#if HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1
    source_run_route_prechecked = 1;
#endif
  } else if (source_run_preload_route.route.kind == HZ6_ROUTE_INVALID) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_source_run_route_invalid);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_source_run_route_fallback);
  }
#endif
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_lookup_after_maps);
#if HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 || \
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
  Hz6PreloadRoute preload_route =
      source_run_route_prechecked ? source_run_preload_route
                                  : hz6_preload_route(allocator, ptr);
#else
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
#endif
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
          int midpage_fast_free_class_allowed =
              preload_route.route.class_id <=
              HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS;
#if HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS > 0
          midpage_fast_free_class_allowed =
              midpage_fast_free_class_allowed &&
              preload_route.route.class_id >=
                  HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS;
#endif
          midpage_fast_free =
              midpage_fast_free_class_allowed &&
              hz6_midpage_active_map_eligible(preload_route.route.front_id,
                                              preload_route.route.class_id);
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
#elif HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 || \
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
    if (source_run_route_prechecked) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_source_run_route_prechecked);
      hz6_free_with_route_prechecked(allocator, ptr, preload_route.route, 0);
    } else {
      hz6_free(allocator, ptr);
    }
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
  hz6_preload_phase_add(&g_hz6_preload_phase_stats.calloc_zero_bytes,
                        bytes);
  hz6_preload_phase_count_size_bucket(
      bytes,
      &g_hz6_preload_phase_stats.calloc_size_zero,
      &g_hz6_preload_phase_stats.calloc_size_le1024,
      &g_hz6_preload_phase_stats.calloc_size_1025_4096,
      &g_hz6_preload_phase_stats.calloc_size_4097_16384,
      &g_hz6_preload_phase_stats.calloc_size_gt16384);
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
#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1
  if (old_size == 4096u && size > 4096u && size <= HZ6_MIDPAGE_8K_BYTES) {
    g_hz6_preload_realloc_boundary_adapt_4k = 1u;
  }
#endif
#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1
  if (old_size == HZ6_MIDPAGE_8K_BYTES &&
      size > HZ6_MIDPAGE_8K_BYTES && size <= HZ6_MIDPAGE_32K_BYTES) {
    g_hz6_preload_realloc_boundary_adapt_8k = 1u;
  }
#endif
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
  hz6_preload_note_realloc_copy_class(preload_route, size);
  memcpy(next, ptr, copy_bytes);
  free(ptr);
  return next;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.posix_memalign_calls);
  hz6_preload_phase_count_alignment_bucket(
      alignment,
      &g_hz6_preload_phase_stats.posix_memalign_align_le16,
      &g_hz6_preload_phase_stats.posix_memalign_align_17_64,
      &g_hz6_preload_phase_stats.posix_memalign_align_65_4096,
      &g_hz6_preload_phase_stats.posix_memalign_align_gt4096);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.posix_memalign_size_zero,
      &g_hz6_preload_phase_stats.posix_memalign_size_le1024,
      &g_hz6_preload_phase_stats.posix_memalign_size_1025_4096,
      &g_hz6_preload_phase_stats.posix_memalign_size_4097_16384,
      &g_hz6_preload_phase_stats.posix_memalign_size_gt16384);
  if ((alignment & (alignment - 1u)) != 0 || alignment < sizeof(void*)) {
    return EINVAL;
  }
  if (alignment <= 16u) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.posix_memalign_hz6_path);
    void* ptr = malloc(size);
    if (!ptr) {
      return ENOMEM;
    }
    *memptr = ptr;
    return 0;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.posix_memalign_real_fallback);
  int rc = hz6_preload_real_posix_memalign(memptr, alignment, size);
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
  if (rc == 0 && *memptr) {
    (void)hz6_preload_real_aligned_record(*memptr);
  }
#endif
  return rc;
}

void* aligned_alloc(size_t alignment, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.aligned_alloc_calls);
  hz6_preload_phase_count_alignment_bucket(
      alignment,
      &g_hz6_preload_phase_stats.aligned_alloc_align_le16,
      &g_hz6_preload_phase_stats.aligned_alloc_align_17_64,
      &g_hz6_preload_phase_stats.aligned_alloc_align_65_4096,
      &g_hz6_preload_phase_stats.aligned_alloc_align_gt4096);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.aligned_alloc_size_zero,
      &g_hz6_preload_phase_stats.aligned_alloc_size_le1024,
      &g_hz6_preload_phase_stats.aligned_alloc_size_1025_4096,
      &g_hz6_preload_phase_stats.aligned_alloc_size_4097_16384,
      &g_hz6_preload_phase_stats.aligned_alloc_size_gt16384);
  if (alignment <= 16u) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.aligned_alloc_hz6_path);
    return malloc(size);
  }
  if (g_hz6_preload_reentry) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.aligned_alloc_real_fallback);
    return hz6_preload_real_aligned_alloc(alignment, size);
  }
  void* ptr = NULL;
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.aligned_alloc_real_fallback);
  if (hz6_preload_real_posix_memalign(&ptr, alignment, size) != 0) {
    return NULL;
  }
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
  (void)hz6_preload_real_aligned_record(ptr);
#endif
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

int malloc_trim(size_t pad) {
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_malloc_trim(pad);
  }

  int hz6_released = hz6_preload_quiescent_release(0);
  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;
  int real_released = hz6_preload_real_malloc_trim(pad);
  g_hz6_preload_reentry = saved_reentry;
  return hz6_released || real_released != 0;
}
