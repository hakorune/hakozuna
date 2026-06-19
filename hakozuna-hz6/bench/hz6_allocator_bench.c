#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_api_owner.h"
#include "hz6_profiles.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <time.h>
#endif

typedef enum Hz6BenchMode {
  HZ6_BENCH_LOCAL = 0,
  HZ6_BENCH_REMOTE = 1,
  HZ6_BENCH_REUSE = 2,
  HZ6_BENCH_PHASE_REUSE = 3,
} Hz6BenchMode;

static double now_sec(void) {
#if defined(_WIN32)
  LARGE_INTEGER freq;
  LARGE_INTEGER counter;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)freq.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

static const char* mode_name(Hz6BenchMode mode) {
  switch (mode) {
    case HZ6_BENCH_LOCAL:
      return "local";
    case HZ6_BENCH_REMOTE:
      return "remote";
    case HZ6_BENCH_REUSE:
      return "reuse";
    case HZ6_BENCH_PHASE_REUSE:
      return "phase-reuse";
    default:
      return "unknown";
  }
}

static const char* profile_name(Hz6ProfileId profile) {
  switch (profile) {
    case HZ6_PROFILE_STRICT:
      return "strict";
    case HZ6_PROFILE_SPEED:
      return "speed";
    case HZ6_PROFILE_RSS:
      return "rss";
    case HZ6_PROFILE_REMOTE:
      return "remote";
    default:
      return "unknown";
  }
}

static inline int bench_set_owner_slot(Hz6Allocator* allocator,
                                       uint32_t slot) {
  if (!allocator) {
    return 0;
  }
  allocator->owner.token.slot = slot;
  return 1;
}

#if HZ6_DIAGNOSTIC_PROBES
static const char* front_name(size_t index) {
  switch (index) {
    case HZ6_FRONT_ATTR_LOCAL2P:
      return "local2p";
    case HZ6_FRONT_ATTR_MIDPAGE:
      return "midpage";
    case HZ6_FRONT_ATTR_LARGE:
      return "large";
    case HZ6_FRONT_ATTR_TOY:
      return "toy";
    default:
      return "unknown";
  }
}

static void print_front_prefill_stats(const Hz6Allocator* allocator) {
  size_t front;
  for (front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
    printf("[HZ6_PREFILL] front=%s attempt=%zu filled=%zu fallback=%zu\n",
           front_name(front),
           allocator->stats.front_source_prefill_attempt[front],
           allocator->stats.front_source_prefill_filled[front],
           allocator->stats.front_source_prefill_fallback[front]);
  }
}
#endif

static int parse_mode(const char* value, Hz6BenchMode* mode) {
  if (!value || !mode) {
    return 0;
  }
  if (strcmp(value, "local") == 0) {
    *mode = HZ6_BENCH_LOCAL;
    return 1;
  }
  if (strcmp(value, "remote") == 0) {
    *mode = HZ6_BENCH_REMOTE;
    return 1;
  }
  if (strcmp(value, "reuse") == 0) {
    *mode = HZ6_BENCH_REUSE;
    return 1;
  }
  if (strcmp(value, "phase-reuse") == 0) {
    *mode = HZ6_BENCH_PHASE_REUSE;
    return 1;
  }
  return 0;
}

static int compare_ptrs(const void* lhs, const void* rhs) {
  const uintptr_t a = (uintptr_t)*(void* const*)lhs;
  const uintptr_t b = (uintptr_t)*(void* const*)rhs;
  return (a > b) - (a < b);
}

static uint64_t count_reused_ptrs(void** first, void** second, uint64_t n) {
  uint64_t i = 0;
  uint64_t j = 0;
  uint64_t reused = 0;
  qsort(first, (size_t)n, sizeof(first[0]), compare_ptrs);
  qsort(second, (size_t)n, sizeof(second[0]), compare_ptrs);
  while (i < n && j < n) {
    uintptr_t a = (uintptr_t)first[i];
    uintptr_t b = (uintptr_t)second[j];
    if (a == b) {
      ++reused;
      ++i;
      ++j;
    } else if (a < b) {
      ++i;
    } else {
      ++j;
    }
  }
  return reused;
}

static int parse_profile(const char* value, Hz6ProfileId* profile) {
  if (!value || !profile) {
    return 0;
  }
  if (strcmp(value, "strict") == 0) {
    *profile = HZ6_PROFILE_STRICT;
    return 1;
  }
  if (strcmp(value, "speed") == 0) {
    *profile = HZ6_PROFILE_SPEED;
    return 1;
  }
  if (strcmp(value, "rss") == 0) {
    *profile = HZ6_PROFILE_RSS;
    return 1;
  }
  if (strcmp(value, "remote") == 0) {
    *profile = HZ6_PROFILE_REMOTE;
    return 1;
  }
  return 0;
}

static void touch_payload(void* ptr, size_t size) {
  if (!ptr || size == 0) {
    return;
  }
  volatile unsigned char* bytes = (volatile unsigned char*)ptr;
  bytes[0] ^= 0x5au;
  bytes[size / 2u] ^= 0xa5u;
  bytes[size - 1u] ^= 0x3cu;
}

static void print_stats(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot stats = hz6_stats_snapshot(allocator);
  printf("[HZ6_STATS] route_valid=%zu route_invalid=%zu route_miss=%zu "
         "transfer_push=%zu transfer_pop=%zu source_alloc=%zu "
#if HZ6_DIAGNOSTIC_PROBES
         "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
         "transfer_reuse_hit=%zu transfer_reuse_invalid=%zu "
         "source_refill_starvation=%zu source_refill_saturation=%zu "
         "source_refill_boost=%zu source_refill_clamp=%zu "
         "source_admission_open=%zu source_admission_boosted=%zu "
         "source_admission_clamped=%zu "
         "control_plane_normal=%zu "
         "control_plane_burst_supply_would_open=%zu "
         "control_plane_close_would_start=%zu "
         "source_prefill_attempt=%zu source_prefill_filled=%zu "
         "source_prefill_fallback=%zu front_source_ops_alloc=%zu "
         "front_source_slot_alloc=%zu front_source_prefill_alloc=%zu "
         "toy_source_prefill_call=%zu "
         "negative_filter_attempt=%zu negative_filter_not_armed=%zu "
         "negative_filter_rehome_blocked=%zu "
         "negative_filter_skip_local=%zu "
         "negative_filter_maybe_local=%zu "
         "negative_filter_shadow_false_skip=%zu "
         "negative_filter_shadow_local_valid=%zu "
         "negative_filter_shadow_local_invalid=%zu "
         "negative_filter_range_probe_total=%zu "
         "negative_filter_range_probe_max=%zu "
         "shared_dir_lookup=%zu shared_dir_hit=%zu shared_dir_miss=%zu "
         "shared_dir_stale=%zu shared_dir_hit_local_allocator=%zu "
         "shared_dir_hit_foreign_allocator=%zu "
         "shared_dir_would_skip_local=%zu shared_dir_register=%zu "
         "shared_dir_unregister=%zu shared_dir_probe_total=%zu "
         "shared_dir_probe_max=%zu "
         "shared_dir_first_attempt=%zu shared_dir_first_hit=%zu "
         "shared_dir_first_fallback=%zu shared_dir_first_invalid=%zu "
         "route_exact_lookup_probe_total=%zu route_exact_lookup_probe_max=%zu "
         "owner_locality_lookup=%zu "
         "owner_locality_hit_local_allocator=%zu "
         "owner_locality_hit_foreign_allocator=%zu "
         "owner_locality_miss=%zu "
         "owner_locality_register=%zu "
         "owner_locality_unregister=%zu "
         "owner_locality_probe_total=%zu "
         "owner_locality_probe_max=%zu "
         "source_run_reuse_attempt=%zu source_run_reuse_candidate=%zu "
         "source_run_reuse_hit=%zu source_run_reuse_miss_no_block=%zu "
         "source_run_reuse_miss_no_slot=%zu source_run_reuse_rollback=%zu "
         "source_run_reuse_reserved=%zu source_run_reuse_slot_fail=%zu "
         "source_run_reuse_descriptor_fail=%zu "
         "source_run_reuse_descriptor_reclaim_attempt=%zu "
         "source_run_reuse_descriptor_reclaim_success=%zu "
         "source_run_reuse_descriptor_reclaim_no_candidate=%zu "
         "source_run_reuse_same_class_reclaim_attempt=%zu "
         "source_run_reuse_same_class_reclaim_success=%zu "
         "source_run_reuse_same_class_reclaim_no_candidate=%zu "
         "descriptor_frontcache_reuse_dryrun_calls=%zu "
         "descriptor_frontcache_reuse_requested_nonempty=%zu "
         "descriptor_frontcache_reuse_requested_total=%zu "
         "descriptor_frontcache_reuse_donor_total=%zu "
         "descriptor_frontcache_reuse_largest_donor_max=%zu "
         "descriptor_frontcache_reuse_donor_bins_max=%zu "
         "descriptorless_frontcache_push=%zu "
         "descriptorless_frontcache_pop=%zu "
         "descriptorless_frontcache_descriptor_fail=%zu "
         "descriptorless_frontcache_route_fail=%zu "
         "descriptorless_frontcache_invalid=%zu "
         "descriptorreserve_frontcache_push=%zu "
         "descriptorreserve_frontcache_pop=%zu "
         "descriptorreserve_frontcache_missing=%zu "
         "descriptorreserve_frontcache_invalid=%zu "
         "descgov_trigger_descriptor_fail=%zu "
         "descgov_detach_attempt=%zu "
         "descgov_detach_success=%zu "
         "descgov_detach_budget_denied=%zu "
         "descgov_detach_class_denied=%zu "
         "descgov_materialize_admit=%zu "
         "descgov_materialize_block_no_descriptor=%zu "
         "descgov_materialize_fail=%zu "
         "descgov_detached_current=%zu "
         "descgov_detached_max=%zu "
         "source_run_reuse_route_fail=%zu "
         "source_run_reuse_prepare_fail=%zu "
         "source_block_fail_active_max=%zu "
         "source_block_fail_registered_max=%zu "
         "source_block_fail_ref_nonzero_max=%zu "
         "source_block_fail_ref_zero_max=%zu "
         "local2p_source_alloc=%zu "
         "midpage_source_alloc=%zu large_source_alloc=%zu toy_source_alloc=%zu "
#endif
         "alloc_fail=%zu descriptor_exhausted=%zu route_register_fail=%zu "
         "source_block_exhausted=%zu descriptor_probe_total=%zu "
         "descriptor_probe_max=%zu route_lookup_probe_total=%zu "
         "route_lookup_probe_max=%zu route_register_probe_total=%zu "
         "route_register_probe_max=%zu route_unregister_probe_total=%zu "
         "route_unregister_probe_max=%zu source_block_probe_total=%zu "
         "source_block_probe_max=%zu "
         "large_span_central_push=%zu large_span_central_pop=%zu "
         "large_span_source_alloc=%zu\n",
         stats.route_valid, stats.route_invalid, stats.route_miss,
         stats.transfer_push, stats.transfer_pop, stats.source_alloc,
#if HZ6_DIAGNOSTIC_PROBES
         stats.frontcache_reuse_hit, stats.frontcache_reuse_invalid,
         stats.transfer_reuse_hit, stats.transfer_reuse_invalid,
         stats.source_refill_starvation, stats.source_refill_saturation,
         stats.source_refill_boost, stats.source_refill_clamp,
         stats.source_admission_open, stats.source_admission_boosted,
         stats.source_admission_clamped,
         stats.control_plane_normal,
         stats.control_plane_burst_supply_would_open,
         stats.control_plane_close_would_start,
         stats.source_prefill_attempt, stats.source_prefill_filled,
         stats.source_prefill_fallback, stats.front_source_ops_alloc,
         stats.front_source_slot_alloc, stats.front_source_prefill_alloc,
         stats.toy_source_prefill_call,
         stats.negative_filter_attempt, stats.negative_filter_not_armed,
         stats.negative_filter_rehome_blocked,
         stats.negative_filter_skip_local,
         stats.negative_filter_maybe_local,
         stats.negative_filter_shadow_false_skip,
         stats.negative_filter_shadow_local_valid,
         stats.negative_filter_shadow_local_invalid,
         stats.negative_filter_range_probe_total,
         stats.negative_filter_range_probe_max,
         stats.shared_dir_lookup, stats.shared_dir_hit,
         stats.shared_dir_miss, stats.shared_dir_stale,
         stats.shared_dir_hit_local_allocator,
         stats.shared_dir_hit_foreign_allocator,
         stats.shared_dir_would_skip_local, stats.shared_dir_register,
         stats.shared_dir_unregister, stats.shared_dir_probe_total,
         stats.shared_dir_probe_max,
         stats.shared_dir_first_attempt, stats.shared_dir_first_hit,
         stats.shared_dir_first_fallback, stats.shared_dir_first_invalid,
         stats.route_exact_lookup_probe_total,
         stats.route_exact_lookup_probe_max,
         stats.owner_locality_lookup,
         stats.owner_locality_hit_local_allocator,
         stats.owner_locality_hit_foreign_allocator,
         stats.owner_locality_miss,
         stats.owner_locality_register,
         stats.owner_locality_unregister,
         stats.owner_locality_probe_total,
         stats.owner_locality_probe_max,
         stats.source_run_reuse_attempt, stats.source_run_reuse_candidate,
         stats.source_run_reuse_hit, stats.source_run_reuse_miss_no_block,
         stats.source_run_reuse_miss_no_slot, stats.source_run_reuse_rollback,
         stats.source_run_reuse_reserved, stats.source_run_reuse_slot_fail,
         stats.source_run_reuse_descriptor_fail,
         stats.source_run_reuse_descriptor_reclaim_attempt,
         stats.source_run_reuse_descriptor_reclaim_success,
         stats.source_run_reuse_descriptor_reclaim_no_candidate,
         stats.source_run_reuse_same_class_reclaim_attempt,
         stats.source_run_reuse_same_class_reclaim_success,
         stats.source_run_reuse_same_class_reclaim_no_candidate,
         stats.descriptor_frontcache_reuse_dryrun_calls,
         stats.descriptor_frontcache_reuse_requested_nonempty,
         stats.descriptor_frontcache_reuse_requested_total,
         stats.descriptor_frontcache_reuse_donor_total,
         stats.descriptor_frontcache_reuse_largest_donor_max,
         stats.descriptor_frontcache_reuse_donor_bins_max,
         stats.descriptorless_frontcache_push,
         stats.descriptorless_frontcache_pop,
         stats.descriptorless_frontcache_descriptor_fail,
         stats.descriptorless_frontcache_route_fail,
         stats.descriptorless_frontcache_invalid,
         stats.descriptorreserve_frontcache_push,
         stats.descriptorreserve_frontcache_pop,
         stats.descriptorreserve_frontcache_missing,
         stats.descriptorreserve_frontcache_invalid,
         stats.descgov_trigger_descriptor_fail,
         stats.descgov_detach_attempt,
         stats.descgov_detach_success,
         stats.descgov_detach_budget_denied,
         stats.descgov_detach_class_denied,
         stats.descgov_materialize_admit,
         stats.descgov_materialize_block_no_descriptor,
         stats.descgov_materialize_fail,
         stats.descgov_detached_current,
         stats.descgov_detached_max,
         stats.source_run_reuse_route_fail,
         stats.source_run_reuse_prepare_fail,
         stats.source_block_fail_active_max,
         stats.source_block_fail_registered_max,
         stats.source_block_fail_ref_nonzero_max,
         stats.source_block_fail_ref_zero_max,
         stats.local2p_source_alloc,
         stats.midpage_source_alloc, stats.large_source_alloc,
         stats.toy_source_alloc,
#endif
         stats.alloc_fail, stats.descriptor_exhausted,
         stats.route_register_fail, stats.source_block_exhausted,
         stats.descriptor_probe_total, stats.descriptor_probe_max,
         stats.route_lookup_probe_total, stats.route_lookup_probe_max,
         stats.route_register_probe_total, stats.route_register_probe_max,
         stats.route_unregister_probe_total, stats.route_unregister_probe_max,
         stats.source_block_probe_total, stats.source_block_probe_max,
         stats.large_span_central_push, stats.large_span_central_pop,
         stats.large_span_source_alloc);
#if HZ6_DIAGNOSTIC_PROBES
  printf("[HZ6_OWNER_EQUAL] free=%zu remote_free=%zu local_cache=%zu "
         "visible_lookup=%zu transfer_locality=%zu large_central=%zu "
         "remote_pending=%zu owner_dead=%zu same_owner_fast=%zu unknown=%zu\n",
         stats.owner_equal_site_free,
         stats.owner_equal_site_remote_free,
         stats.owner_equal_site_local_cache,
         stats.owner_equal_site_visible_lookup,
         stats.owner_equal_site_transfer_locality,
         stats.owner_equal_site_large_central,
         stats.owner_equal_site_remote_pending,
         stats.owner_equal_site_owner_dead,
         stats.owner_equal_site_same_owner_fast,
         stats.owner_equal_site_unknown);
  printf("[HZ6_REMOTE_PENDING] "
         "returned_backpressure=%zu returned_uncommitted=%zu "
         "foreign_candidate=%zu origin_pending_commit=%zu "
         "pending_no_rehome=%zu pending_publish_fail=%zu "
         "enqueue_attempt=%zu enqueue_success=%zu enqueue_full=%zu "
         "duplicate_claim=%zu owner_dead=%zu current=%zu queued=%zu "
         "claimed=%zu total=%zu high_water=%zu "
         "maintenance_check=%zu maintenance_armed=%zu batch_call=%zu "
         "batch_items=%zu frontcache_push=%zu frontcache_full=%zu "
         "key_load=%zu key_hit=%zu direct_gate_load=%zu "
         "direct_gate_hit=%zu direct_claim_attempt=%zu "
         "direct_claim_success=%zu direct_claim_busy=%zu "
         "direct_empty_after_hint=%zu direct_activate_success=%zu "
         "direct_integrity_failure=%zu source_boundary_attempt=%zu "
         "source_boundary_gate_hit=%zu source_boundary_claim_success=%zu "
         "direct_prefill_avoided=%zu direct_source_alloc_avoided=%zu "
         "claim_before_existing_reuse=%zu "
         "claim_while_frontcache_nonempty=%zu "
         "claim_while_transfer_nonempty=%zu same_key_before=%zu "
         "same_key_after=%zu maintenance_immediate_reuse=%zu "
         "maintenance_batch_surplus=%zu source_block_commit_match=%zu\n",
         stats.remote_free_returned_backpressure,
         stats.remote_free_returned_uncommitted,
         stats.remote_free_foreign_candidate,
         stats.remote_free_origin_pending_commit,
         stats.remote_free_pending_no_rehome,
         stats.remote_free_pending_publish_fail,
         stats.remote_pending_enqueue_attempt,
         stats.remote_pending_enqueue_success,
         stats.remote_pending_enqueue_full,
         stats.remote_pending_duplicate_claim,
         stats.remote_pending_owner_dead,
         stats.remote_pending_current,
         stats.remote_pending_queued_current,
         stats.remote_pending_claimed_current,
         stats.remote_pending_total_current,
         stats.remote_pending_high_water,
         stats.remote_pending_maintenance_check,
         stats.remote_pending_maintenance_armed,
         stats.remote_pending_batch_call,
         stats.remote_pending_batch_items,
         stats.remote_pending_frontcache_push,
         stats.remote_pending_frontcache_full,
         stats.remote_pending_key_nonempty_load,
         stats.remote_pending_key_nonempty_hit,
         stats.remote_pending_direct_gate_load,
         stats.remote_pending_direct_gate_hit,
         stats.remote_pending_direct_claim_attempt,
         stats.remote_pending_direct_claim_success,
         stats.remote_pending_direct_claim_busy,
         stats.remote_pending_direct_claim_empty_after_hint,
         stats.remote_pending_direct_activate_success,
         stats.remote_pending_direct_integrity_failure,
         stats.remote_pending_direct_source_boundary_attempt,
         stats.remote_pending_direct_source_boundary_gate_hit,
         stats.remote_pending_direct_source_boundary_claim_success,
         stats.remote_pending_direct_prefill_avoided,
         stats.remote_pending_direct_source_alloc_avoided,
         stats.remote_pending_direct_claim_before_existing_reuse,
         stats.remote_pending_direct_claim_while_frontcache_nonempty,
         stats.remote_pending_direct_claim_while_transfer_nonempty,
         stats.pending_same_key_before_maintenance,
         stats.pending_same_key_after_maintenance,
         stats.pending_maintenance_immediate_reuse_success,
         stats.pending_maintenance_batch_surplus,
         stats.source_block_commit_with_matching_pending);
  printf("[HZ6_REMOTE_INBOX_REJECT] "
         "storage_ineligible=%zu descriptor_mismatch=%zu owner_dead=%zu "
         "owner_mismatch=%zu enqueue_fail=%zu\n",
         stats.remote_pending_owner_inbox_storage_ineligible,
         stats.remote_pending_owner_inbox_descriptor_mismatch,
         stats.remote_pending_owner_inbox_owner_dead,
         stats.remote_pending_owner_inbox_owner_mismatch,
         stats.remote_pending_owner_inbox_enqueue_fail);
  printf("[HZ6_REMOTE_EXTERNAL_TICKET] "
         "attempt=%zu success=%zu full=%zu duplicate=%zu "
         "duplicate_probe_total=%zu duplicate_probe_max=%zu "
         "duplicate_scan_skip=%zu locked_revalidate_fail=%zu consume=%zu "
         "current=%zu high_water=%zu consume_empty=%zu "
         "frontcache_full=%zu "
         "route_mismatch=%zu owner_mismatch=%zu state_mismatch=%zu "
         "storage_mismatch=%zu\n",
         stats.remote_pending_external_ticket_attempt,
         stats.remote_pending_external_ticket_success,
         stats.remote_pending_external_ticket_full,
         stats.remote_pending_external_ticket_duplicate,
         stats.remote_pending_external_ticket_duplicate_probe_total,
         stats.remote_pending_external_ticket_duplicate_probe_max,
         stats.remote_pending_external_ticket_duplicate_scan_skip,
         stats.remote_pending_external_ticket_locked_revalidate_fail,
         stats.remote_pending_external_ticket_consume,
         stats.remote_pending_external_ticket_current,
         stats.remote_pending_external_ticket_high_water,
         stats.remote_pending_external_ticket_consume_empty,
         stats.remote_pending_external_ticket_frontcache_full,
         stats.remote_pending_external_ticket_route_mismatch,
         stats.remote_pending_external_ticket_owner_mismatch,
         stats.remote_pending_external_ticket_state_mismatch,
         stats.remote_pending_external_ticket_storage_mismatch);
  printf("[HZ6_REMOTE_BACKPRESSURE] "
         "origin_transfer_full=%zu origin_full_transfer_total=%zu "
         "origin_full_class_total=%zu origin_full_class_max=%zu "
         "transfer_full=%zu transfer_full_transfer_total=%zu "
         "transfer_full_class_total=%zu transfer_full_class_max=%zu\n",
         stats.remote_free_backpressure_origin_transfer_full,
         stats.remote_free_backpressure_origin_full_transfer_count_total,
         stats.remote_free_backpressure_origin_full_class_count_total,
         stats.remote_free_backpressure_origin_full_class_count_max,
         stats.transfer_reserve_full,
         stats.transfer_reserve_full_transfer_count_total,
         stats.transfer_reserve_full_class_count_total,
         stats.transfer_reserve_full_class_count_max);
  printf("[HZ6_TOY_SMALL] "
         "malloc_fast_attempt=%zu malloc_fast_hit=%zu "
         "malloc_front_dispatch=%zu malloc_frontcache_pop=%zu "
         "malloc_activate_success=%zu "
         "free_route_lookup=%zu free_owner_equal=%zu "
         "free_fast_hit=%zu free_cache_push=%zu "
         "free_cache_attempt=%zu free_cache_success=%zu "
         "active_map_register=%zu active_map_free_attempt=%zu "
         "active_map_free_hit=%zu active_map_route_bypass=%zu\n",
         stats.toy_small_malloc_fast_attempt,
         stats.toy_small_malloc_fast_hit,
         stats.toy_small_malloc_front_dispatch,
         stats.toy_small_malloc_frontcache_pop,
         stats.toy_small_malloc_activate_success,
         stats.toy_small_free_route_lookup,
         stats.toy_small_free_owner_equal,
         stats.toy_small_free_fast_hit,
         stats.toy_small_free_cache_push,
         stats.toy_small_free_cache_attempt,
         stats.toy_small_free_cache_success,
         stats.toy_small_active_map_register,
         stats.toy_small_active_map_free_attempt,
         stats.toy_small_active_map_free_hit,
         stats.toy_small_active_map_route_bypass);
  printf("[HZ6_SOURCE_BLOCK_ROUTE] attempt=%zu block_hit=%zu slot_hit=%zu "
         "descriptor_hit=%zu miss_no_block=%zu invalid_alignment=%zu "
         "invalid_unused=%zu descriptor_miss=%zu class_mismatch=%zu "
         "probe_total=%zu probe_max=%zu "
         "descriptor_map_hit=%zu descriptor_map_miss=%zu "
         "descriptor_map_stale=%zu descriptor_map_set=%zu "
         "descriptor_map_clear=%zu "
         "range_index_register=%zu range_index_unregister=%zu "
         "range_index_register_fail=%zu range_index_lookup=%zu "
         "range_index_hit=%zu range_index_miss=%zu range_index_stale=%zu "
         "range_index_probe_total=%zu range_index_probe_max=%zu "
         "behavior_attempt=%zu behavior_valid=%zu "
         "behavior_fallback=%zu behavior_invalid_front=%zu "
         "behavior_invalid_descriptor=%zu\n",
         stats.source_block_route_dryrun_attempt,
         stats.source_block_route_block_hit,
         stats.source_block_route_slot_hit,
         stats.source_block_route_descriptor_hit,
         stats.source_block_route_miss_no_block,
         stats.source_block_route_invalid_alignment,
         stats.source_block_route_invalid_unused,
         stats.source_block_route_descriptor_miss,
         stats.source_block_route_class_mismatch,
         stats.source_block_route_probe_total,
         stats.source_block_route_probe_max,
         stats.source_block_route_descriptor_map_hit,
         stats.source_block_route_descriptor_map_miss,
         stats.source_block_route_descriptor_map_stale,
         stats.source_block_route_descriptor_map_set,
         stats.source_block_route_descriptor_map_clear,
         stats.source_block_route_range_index_register,
         stats.source_block_route_range_index_unregister,
         stats.source_block_route_range_index_register_fail,
         stats.source_block_route_range_index_lookup,
         stats.source_block_route_range_index_hit,
         stats.source_block_route_range_index_miss,
         stats.source_block_route_range_index_stale,
         stats.source_block_route_range_index_probe_total,
         stats.source_block_route_range_index_probe_max,
         stats.source_block_route_behavior_attempt,
         stats.source_block_route_behavior_valid,
         stats.source_block_route_behavior_fallback,
         stats.source_block_route_behavior_invalid_front,
         stats.source_block_route_behavior_invalid_descriptor);
  printf("[HZ6_ROUTE_AUDIT] "
         "exact_backend=%zu page_backend=%zu "
         "page_probe_total=%zu page_probe_max=%zu "
         "page_exact_probe_total=%zu page_exact_probe_max=%zu "
         "page_exact_hash_probe_total=%zu page_exact_hash_probe_max=%zu "
         "page_exact_range_probe_total=%zu page_exact_range_probe_max=%zu "
         "page_exact_page_seed_probe_total=%zu page_exact_page_seed_probe_max=%zu "
         "page_invalid_probe_total=%zu page_invalid_probe_max=%zu "
         "page_valid=%zu page_invalid=%zu page_miss=%zu "
         "last_hit_attempt=%zu last_hit_hit=%zu "
         "last_hit_stale=%zu last_hit_fill=%zu last_hit_clear=%zu "
         "overflow_lookup=%zu overflow_hit=%zu "
         "overflow_range_lookup=%zu overflow_range_hit=%zu\n",
         stats.route_lookup_exact_backend,
         stats.route_lookup_page_backend,
         stats.route_lookup_page_probe_total,
         stats.route_lookup_page_probe_max,
         stats.route_lookup_page_exact_probe_total,
         stats.route_lookup_page_exact_probe_max,
         stats.route_lookup_page_exact_hash_probe_total,
         stats.route_lookup_page_exact_hash_probe_max,
         stats.route_lookup_page_exact_range_probe_total,
         stats.route_lookup_page_exact_range_probe_max,
         stats.route_lookup_page_exact_page_seed_probe_total,
         stats.route_lookup_page_exact_page_seed_probe_max,
         stats.route_lookup_page_invalid_probe_total,
         stats.route_lookup_page_invalid_probe_max,
         stats.route_lookup_page_valid,
         stats.route_lookup_page_invalid,
         stats.route_lookup_page_miss,
         stats.route_lookup_last_hit_attempt,
         stats.route_lookup_last_hit_hit,
         stats.route_lookup_last_hit_stale,
         stats.route_lookup_last_hit_fill,
         stats.route_lookup_last_hit_clear,
         stats.route_lookup_overflow_lookup,
         stats.route_lookup_overflow_hit,
         stats.route_lookup_overflow_range_lookup,
         stats.route_lookup_overflow_range_hit);
  print_front_prefill_stats(allocator);
  printf("[HZ6_MEMORY_ATTR] "
         "descriptor_table_bytes=%zu "
         "route_table_bytes=%zu "
         "source_block_table_bytes=%zu "
         "frontcache_table_bytes=%zu "
         "transfer_table_bytes=%zu "
         "ownerlocality_index_bytes=%zu "
         "active_descriptors=%zu "
         "local_free_descriptors=%zu "
         "transfer_free_descriptors=%zu "
         "remote_pending_descriptors=%zu "
         "dead_with_ptr_descriptors=%zu "
         "active_source_blocks=%zu "
         "registered_source_blocks=%zu "
         "ref_nonzero_source_blocks=%zu "
         "source_run_active_blocks=%zu "
         "toy_source_run_blocks=%zu "
         "midpage_source_run_blocks=%zu "
         "source_run_slot_count_max=%zu "
         "toy_source_run_slot_count_max=%zu "
         "midpage_8k_source_run_slot_count_max=%zu "
         "midpage_32k_source_run_slot_count_max=%zu "
         "source_block_payload_bytes=%zu "
         "source_block_committed_estimate=%zu "
         "route_active_current=%zu "
         "route_active_max=%zu "
         "route_tombstone_current=%zu "
         "frontcache_total=%zu "
         "frontcache_largest_bin=%zu\n",
         stats.memory_descriptor_table_bytes,
         stats.memory_route_table_bytes,
         stats.memory_source_block_table_bytes,
         stats.memory_frontcache_table_bytes,
         stats.memory_transfer_table_bytes,
         stats.memory_ownerlocality_index_bytes,
         stats.memory_active_descriptors,
         stats.memory_local_free_descriptors,
         stats.memory_transfer_free_descriptors,
         stats.memory_remote_pending_descriptors,
         stats.memory_dead_with_ptr_descriptors,
         stats.memory_active_source_blocks,
         stats.memory_registered_source_blocks,
         stats.memory_ref_nonzero_source_blocks,
         stats.memory_source_run_active_blocks,
         stats.memory_toy_source_run_blocks,
         stats.memory_midpage_source_run_blocks,
         stats.memory_source_run_slot_count_max,
         stats.memory_toy_source_run_slot_count_max,
         stats.memory_midpage_8k_source_run_slot_count_max,
         stats.memory_midpage_32k_source_run_slot_count_max,
         stats.memory_source_block_payload_bytes,
         stats.memory_source_block_committed_estimate,
         stats.route_active_current,
         stats.route_active_max,
         stats.route_tombstone_current,
         stats.memory_frontcache_total,
         stats.memory_frontcache_largest_bin);
  printf("[HZ6_METADATA_SLIM] "
         "descriptor_entry_bytes=%zu "
         "descriptor_thin_hot_entry_bytes=%zu "
         "descriptor_thin_hot_table_bytes=%zu "
         "descriptor_thin_hot_savings_bytes=%zu "
         "route_entry_bytes=%zu "
         "route_slim_entry_bytes=%zu "
         "route_slim_table_bytes=%zu "
         "route_slim_savings_bytes=%zu "
         "route_bytes16_table_bytes=%zu "
         "route_bytes16_savings_bytes=%zu "
         "route_bytes16_active_checked=%zu "
         "route_bytes16_overflow=%zu "
         "route_bytes16_max=%zu "
         "route_bytes16_minus1_overflow=%zu "
         "route_bytes16_minus1_zero=%zu "
         "route_bytes16_minus1_max_stored=%zu "
         "source_block_entry_bytes=%zu "
         "source_block_slim_entry_bytes=%zu "
         "source_block_slim_table_bytes=%zu "
         "source_block_slim_savings_bytes=%zu "
         "frontcache_entry_bytes=%zu "
         "frontcache_slim_entry_bytes=%zu "
         "frontcache_slim_table_bytes=%zu "
         "frontcache_slim_savings_bytes=%zu\n",
         stats.metadata_descriptor_entry_bytes,
         stats.metadata_descriptor_thin_hot_entry_bytes,
         stats.metadata_descriptor_thin_hot_table_bytes,
         stats.metadata_descriptor_thin_hot_savings_bytes,
         stats.metadata_route_entry_bytes,
         stats.metadata_route_slim_entry_bytes,
         stats.metadata_route_slim_table_bytes,
         stats.metadata_route_slim_savings_bytes,
         stats.metadata_route_bytes16_table_bytes,
         stats.metadata_route_bytes16_savings_bytes,
         stats.metadata_route_bytes16_active_checked,
         stats.metadata_route_bytes16_overflow,
         stats.metadata_route_bytes16_max,
         stats.metadata_route_bytes16_minus1_overflow,
         stats.metadata_route_bytes16_minus1_zero,
         stats.metadata_route_bytes16_minus1_max_stored,
         stats.metadata_source_block_entry_bytes,
         stats.metadata_source_block_slim_entry_bytes,
         stats.metadata_source_block_slim_table_bytes,
         stats.metadata_source_block_slim_savings_bytes,
         stats.metadata_frontcache_entry_bytes,
         stats.metadata_frontcache_slim_entry_bytes,
         stats.metadata_frontcache_slim_table_bytes,
         stats.metadata_frontcache_slim_savings_bytes);
#endif
}

static int run_local(Hz6ProfileId profile, uint64_t iters, size_t size) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, profile);
  if (!bench_set_owner_slot(&allocator, 0)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&allocator);
    return 4;
  }

  uint64_t ops = 0;
  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = hz6_malloc(&allocator, size);
    if (!ptr) {
      fprintf(stderr, "local malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 5;
    }
    touch_payload(ptr, size);
    hz6_free(&allocator, ptr);
    ops += 2u;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=0\n",
         profile_name(profile), mode_name(HZ6_BENCH_LOCAL),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed);
  print_stats(&allocator);
  hz6_allocator_destroy(&allocator);
  return 0;
}

static int run_remote(Hz6ProfileId profile, uint64_t iters, size_t size) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, profile);
  if (!bench_set_owner_slot(&allocator, 0)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&allocator);
    return 4;
  }

  uint64_t ops = 0;
  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = hz6_malloc(&allocator, size);
    if (!ptr) {
      fprintf(stderr, "remote malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 5;
    }
    touch_payload(ptr, size);
    if (!bench_set_owner_slot(&allocator, 1)) {
      fprintf(stderr, "remote owner slot switch failed\n");
      hz6_allocator_destroy(&allocator);
      return 6;
    }
    if (!hz6_free_remote(&allocator, ptr)) {
      fprintf(stderr, "remote free failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 7;
    }
    if (!bench_set_owner_slot(&allocator, 0)) {
      fprintf(stderr, "remote owner restore failed\n");
      hz6_allocator_destroy(&allocator);
      return 8;
    }
    ops += 2u;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=0\n",
         profile_name(profile), mode_name(HZ6_BENCH_REMOTE),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed);
  print_stats(&allocator);
  hz6_allocator_destroy(&allocator);
  return 0;
}

static int run_reuse(Hz6ProfileId profile, uint64_t iters, size_t size) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, profile);
  if (!bench_set_owner_slot(&allocator, 0)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&allocator);
    return 4;
  }

  uint64_t ops = 0;
  uint64_t reuse_hits = 0;
  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    void* first = hz6_malloc(&allocator, size);
    if (!first) {
      fprintf(stderr, "reuse malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 5;
    }
    touch_payload(first, size);
    if (!bench_set_owner_slot(&allocator, 1)) {
      fprintf(stderr, "reuse owner slot switch failed\n");
      hz6_allocator_destroy(&allocator);
      return 6;
    }
    if (!hz6_free_remote(&allocator, first)) {
      fprintf(stderr, "reuse remote free failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 7;
    }
    if (!bench_set_owner_slot(&allocator, 0)) {
      fprintf(stderr, "reuse owner restore failed\n");
      hz6_allocator_destroy(&allocator);
      return 8;
    }
    void* second = hz6_malloc(&allocator, size);
    if (!second) {
      fprintf(stderr, "reuse second malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 9;
    }
    if (second == first) {
      ++reuse_hits;
    }
    touch_payload(second, size);
    hz6_free(&allocator, second);
    ops += 4u;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=%llu\n",
         profile_name(profile), mode_name(HZ6_BENCH_REUSE),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed, (unsigned long long)reuse_hits);
  print_stats(&allocator);
  hz6_allocator_destroy(&allocator);
  return 0;
}

static int run_phase_reuse(Hz6ProfileId profile,
                           uint64_t iters,
                           size_t size) {
  Hz6Allocator origin;
  Hz6Allocator foreign;
  void** first = NULL;
  void** second = NULL;
  uint64_t ops = 0;
  uint64_t reuse_hits = 0;
  uint64_t first_count = 0;
  uint64_t second_count = 0;
  int first_remote_freed = 0;
  int rc = 0;

  hz6_allocator_init_with_profile(&origin, profile);
  hz6_allocator_init_with_profile(&foreign, profile);
  if (!bench_set_owner_slot(&origin, 0) ||
      !bench_set_owner_slot(&foreign, 1)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&foreign);
    hz6_allocator_destroy(&origin);
    return 4;
  }

  first = (void**)calloc((size_t)iters, sizeof(first[0]));
  second = (void**)calloc((size_t)iters, sizeof(second[0]));
  if (!first || !second) {
    fprintf(stderr, "phase-reuse pointer table allocation failed\n");
    rc = 5;
    goto done;
  }

  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    first[i] = hz6_malloc(&origin, size);
    if (!first[i]) {
      fprintf(stderr, "phase-reuse phase-a malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      rc = 6;
      goto done;
    }
    touch_payload(first[i], size);
    ++first_count;
    ++ops;
  }

  for (uint64_t i = 0; i < iters; ++i) {
    hz6_free(&foreign, first[i]);
    ++ops;
  }
  first_remote_freed = 1;

  for (uint64_t i = 0; i < iters; ++i) {
    second[i] = hz6_malloc(&origin, size);
    if (!second[i]) {
      fprintf(stderr, "phase-reuse phase-c malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      rc = 10;
      goto done;
    }
    touch_payload(second[i], size);
    ++second_count;
    ++ops;
  }
  double elapsed = now_sec() - start;
  reuse_hits = count_reused_ptrs(first, second, iters);
  for (uint64_t i = 0; i < iters; ++i) {
    hz6_free(&origin, second[i]);
    ++ops;
  }

  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=%llu\n",
         profile_name(profile), mode_name(HZ6_BENCH_PHASE_REUSE),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed, (unsigned long long)reuse_hits);
  printf("[HZ6_PHASE_REUSE_STATS] owner=origin\n");
  print_stats(&origin);
  printf("[HZ6_PHASE_REUSE_STATS] owner=foreign\n");
  print_stats(&foreign);

done:
  if (rc != 0) {
    for (uint64_t i = 0; i < second_count; ++i) {
      hz6_free(&origin, second[i]);
    }
    if (!first_remote_freed) {
      for (uint64_t i = 0; i < first_count; ++i) {
        hz6_free(&origin, first[i]);
      }
    }
  }
  free(second);
  free(first);
  hz6_allocator_destroy(&foreign);
  hz6_allocator_destroy(&origin);
  return rc;
}

static void usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [mode] [profile] [iters] [size]\n"
          "  mode: local | remote | reuse | phase-reuse\n"
          "  profile: strict | speed | rss | remote\n"
          "  iters: iteration count\n"
          "  size: allocation size in bytes\n",
          argv0);
}

int main(int argc, char** argv) {
  Hz6BenchMode mode = HZ6_BENCH_LOCAL;
  Hz6ProfileId profile = HZ6_PROFILE_STRICT;
  uint64_t iters = 1000000u;
  size_t size = 65536u;

  if (argc > 1 && !parse_mode(argv[1], &mode)) {
    usage(argv[0]);
    return 2;
  }
  if (argc > 2 && !parse_profile(argv[2], &profile)) {
    usage(argv[0]);
    return 2;
  }
  if (argc > 3) {
    iters = strtoull(argv[3], NULL, 10);
  }
  if (argc > 4) {
    size = (size_t)strtoull(argv[4], NULL, 10);
  }
  if (iters == 0 || size == 0) {
    usage(argv[0]);
    return 2;
  }

  switch (mode) {
    case HZ6_BENCH_LOCAL:
      return run_local(profile, iters, size);
    case HZ6_BENCH_REMOTE:
      return run_remote(profile, iters, size);
    case HZ6_BENCH_REUSE:
      return run_reuse(profile, iters, size);
    case HZ6_BENCH_PHASE_REUSE:
      return run_phase_reuse(profile, iters, size);
    default:
      usage(argv[0]);
      return 2;
  }
}
