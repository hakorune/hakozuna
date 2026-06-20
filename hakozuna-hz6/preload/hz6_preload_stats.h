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
  _Atomic(size_t) malloc_realloc_boundary_slack_4k;
  _Atomic(size_t) malloc_realloc_boundary_slack_8k;
  _Atomic(size_t) malloc_toy_direct_class_eligible;
  _Atomic(size_t) malloc_toy_direct_class_eligible_le1024;
  _Atomic(size_t) malloc_toy_direct_class_eligible_1025_4096;
  _Atomic(size_t) malloc_toy_direct_class_enter;
  _Atomic(size_t) malloc_toy_direct_class_enter_le1024;
  _Atomic(size_t) malloc_toy_direct_class_enter_1025_4096;
  _Atomic(size_t) calloc_calls;
  _Atomic(size_t) calloc_zero_bytes;
  _Atomic(size_t) calloc_size_zero;
  _Atomic(size_t) calloc_size_le1024;
  _Atomic(size_t) calloc_size_1025_4096;
  _Atomic(size_t) calloc_size_4097_16384;
  _Atomic(size_t) calloc_size_gt16384;
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
  _Atomic(size_t) free_source_run_route_attempt;
  _Atomic(size_t) free_source_run_route_hit;
  _Atomic(size_t) free_source_run_route_midpage8_hit;
  _Atomic(size_t) free_source_run_route_midpage32_hit;
  _Atomic(size_t) free_source_run_route_invalid;
  _Atomic(size_t) free_source_run_route_fallback;
  _Atomic(size_t) free_source_run_route_prechecked;
  _Atomic(size_t) free_route_lookup_after_maps;
  _Atomic(size_t) free_route_valid_owned;
  _Atomic(size_t) free_route_valid_foreign_visible;
  _Atomic(size_t) free_route_invalid;
  _Atomic(size_t) free_route_miss_real;
  _Atomic(size_t) free_route_retry_abort;
  _Atomic(size_t) free_route_integrity_abort;
  _Atomic(size_t) free_route_real_free_unproven;
  _Atomic(size_t) free_prechecked_candidate;
  _Atomic(size_t) free_route_before_maps_attempt;
  _Atomic(size_t) free_route_before_maps_arm_skip;
  _Atomic(size_t) free_route_before_maps_foreign_dispatch;
  _Atomic(size_t) free_route_before_maps_fallback;
  _Atomic(size_t) free_resolved_foreign_direct_dispatch;
  _Atomic(size_t) free_resolved_duplicate_route_lookup_avoided;
  _Atomic(size_t) free_midpage_hint_probe;
  _Atomic(size_t) free_midpage_hint_would_first;
  _Atomic(size_t) free_midpage_hint_true_midpage;
  _Atomic(size_t) free_midpage_hint_false_positive;
  _Atomic(size_t) free_midpage_hint_missed_midpage;
  _Atomic(size_t) free_page_kind_selector_probe;
  _Atomic(size_t) free_page_kind_selector_unknown;
  _Atomic(size_t) free_page_kind_selector_toy;
  _Atomic(size_t) free_page_kind_selector_midpage;
  _Atomic(size_t) free_page_kind_selector_mixed;
  _Atomic(size_t) free_page_kind_selector_toy_hit;
  _Atomic(size_t) free_page_kind_selector_midpage_hit;
  _Atomic(size_t) free_page_kind_selector_wrong_toy_page_mid_hit;
  _Atomic(size_t) free_page_kind_selector_wrong_midpage_page_toy_hit;
  _Atomic(size_t) realloc_calls;
  _Atomic(size_t) realloc_owned;
  _Atomic(size_t) realloc_in_place;
  _Atomic(size_t) realloc_real_fallback;
  _Atomic(size_t) realloc_copy_calls;
  _Atomic(size_t) realloc_copy_bytes;
  _Atomic(size_t) realloc_copy_same_class;
  _Atomic(size_t) realloc_copy_cross_class;
  _Atomic(size_t) realloc_copy_boundary_toy_to_midpage;
  _Atomic(size_t) realloc_copy_boundary_mid8_to_mid32;
  _Atomic(size_t) realloc_copy_boundary_midpage_to_large;
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
  _Atomic(size_t) posix_memalign_calls;
  _Atomic(size_t) posix_memalign_hz6_path;
  _Atomic(size_t) posix_memalign_real_fallback;
  _Atomic(size_t) posix_memalign_align_le16;
  _Atomic(size_t) posix_memalign_align_17_64;
  _Atomic(size_t) posix_memalign_align_65_4096;
  _Atomic(size_t) posix_memalign_align_gt4096;
  _Atomic(size_t) posix_memalign_size_zero;
  _Atomic(size_t) posix_memalign_size_le1024;
  _Atomic(size_t) posix_memalign_size_1025_4096;
  _Atomic(size_t) posix_memalign_size_4097_16384;
  _Atomic(size_t) posix_memalign_size_gt16384;
  _Atomic(size_t) aligned_alloc_calls;
  _Atomic(size_t) aligned_alloc_hz6_path;
  _Atomic(size_t) aligned_alloc_real_fallback;
  _Atomic(size_t) aligned_alloc_align_le16;
  _Atomic(size_t) aligned_alloc_align_17_64;
  _Atomic(size_t) aligned_alloc_align_65_4096;
  _Atomic(size_t) aligned_alloc_align_gt4096;
  _Atomic(size_t) aligned_alloc_size_zero;
  _Atomic(size_t) aligned_alloc_size_le1024;
  _Atomic(size_t) aligned_alloc_size_1025_4096;
  _Atomic(size_t) aligned_alloc_size_4097_16384;
  _Atomic(size_t) aligned_alloc_size_gt16384;
  _Atomic(size_t) real_aligned_record_set;
  _Atomic(size_t) real_aligned_record_fail;
  _Atomic(size_t) real_aligned_free_skip_hit;
  _Atomic(size_t) real_aligned_free_skip_miss;
  _Atomic(size_t) calloc_real_record_set;
  _Atomic(size_t) calloc_real_record_fail;
  _Atomic(size_t) calloc_real_free_skip_hit;
  _Atomic(size_t) calloc_real_free_skip_miss;
} Hz6PreloadPhaseStats;

extern Hz6PreloadPhaseStats g_hz6_preload_phase_stats;

size_t hz6_preload_scavenge_local_free(size_t max_bytes);
int hz6_preload_quiescent_release(size_t max_bytes);

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
