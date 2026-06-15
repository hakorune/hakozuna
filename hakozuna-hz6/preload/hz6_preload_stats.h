#ifndef HZ6_PRELOAD_STATS_H
#define HZ6_PRELOAD_STATS_H

#include "hz6_allocator.h"

#include <stddef.h>
#include <stdatomic.h>

typedef struct Hz6PreloadPhaseStats {
  _Atomic(size_t) malloc_calls;
  _Atomic(size_t) malloc_hz6_success;
  _Atomic(size_t) malloc_real_fallback;
  _Atomic(size_t) malloc_size_zero;
  _Atomic(size_t) malloc_size_le1024;
  _Atomic(size_t) malloc_size_1025_4096;
  _Atomic(size_t) malloc_size_4097_16384;
  _Atomic(size_t) malloc_size_gt16384;
  _Atomic(size_t) malloc_midpage_boundary_attempt;
  _Atomic(size_t) malloc_midpage_boundary_hit;
  _Atomic(size_t) malloc_midpage_boundary_fallback;
  _Atomic(size_t) calloc_calls;
  _Atomic(size_t) free_calls;
  _Atomic(size_t) free_null;
  _Atomic(size_t) free_reentry_real;
  _Atomic(size_t) free_local_route_valid;
  _Atomic(size_t) free_visible_route_hit;
  _Atomic(size_t) free_toy_active_map_hit;
  _Atomic(size_t) free_toy_active_map_attempt;
  _Atomic(size_t) free_toy_active_map_miss;
  _Atomic(size_t) free_midpage_active_map_hit;
  _Atomic(size_t) free_midpage_active_map_attempt;
  _Atomic(size_t) free_midpage_active_map_miss;
  _Atomic(size_t) free_route_lookup_after_maps;
  _Atomic(size_t) free_route_valid_owned;
  _Atomic(size_t) free_route_valid_foreign_visible;
  _Atomic(size_t) free_route_invalid;
  _Atomic(size_t) free_route_miss_real;
  _Atomic(size_t) free_prechecked_candidate;
  _Atomic(size_t) free_midpage_hint_probe;
  _Atomic(size_t) free_midpage_hint_would_first;
  _Atomic(size_t) free_midpage_hint_true_midpage;
  _Atomic(size_t) free_midpage_hint_false_positive;
  _Atomic(size_t) free_midpage_hint_missed_midpage;
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

extern Hz6PreloadPhaseStats g_hz6_preload_phase_stats;

size_t hz6_preload_scavenge_local_free(size_t max_bytes);

#if HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1
#define hz6_preload_phase_count(counter) ((void)0)
#define hz6_preload_phase_add(counter, value) ((void)0)
#define hz6_preload_phase_count_size_bucket(size, zero, le1024, range1025_4096, \
                                            range4097_16384, gt16384)          \
  ((void)0)
#else
void hz6_preload_phase_count(_Atomic(size_t)* counter);
void hz6_preload_phase_add(_Atomic(size_t)* counter, size_t value);
void hz6_preload_phase_count_size_bucket(
    size_t size,
    _Atomic(size_t)* zero,
    _Atomic(size_t)* le1024,
    _Atomic(size_t)* range1025_4096,
    _Atomic(size_t)* range4097_16384,
    _Atomic(size_t)* gt16384);
#endif

void hz6_preload_register_allocator(Hz6Allocator* allocator);

#endif
