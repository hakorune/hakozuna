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
  size_t elastic_descriptor_overflow_alloc = 0;
  size_t elastic_descriptor_overflow_reset = 0;
  size_t elastic_descriptor_overflow_exhausted = 0;
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
#include "hz6_preload_stats_print_rehome_vars.inc"
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
  size_t route_compact_deferred = 0;
  size_t route_compact_owner_maintenance = 0;
  size_t route_compact_remote_path_attempt = 0;
  size_t route_lock_read_contended = 0;
  size_t route_lock_write_contended = 0;
  size_t route_lock_max_wait = 0;
  size_t route_tombstone_cond_probe = 0;
  size_t route_tombstone_cond_would_compact = 0;
  size_t route_tombstone_cond_ratio25 = 0;
  size_t route_tombstone_cond_occupancy75 = 0;
  size_t route_tombstone_cond_cooldown_blocked = 0;
  size_t route_tombstone_cond_highwater = 0;
#include "hz6_preload_stats_print_shared_route_vars.inc"
#include "hz6_preload_stats_print_resolve_vars.inc"
  size_t descriptor_live_max = 0;
  size_t source_block_active_max = 0;
  size_t frontcache_total_max = 0;
  size_t frontcache_reuse_hit = 0;
  size_t frontcache_reuse_invalid = 0;
  size_t frontcache_push_by_class[HZ6_STATS_CLASS_COUNT] = {0};
  size_t frontcache_pop_empty_by_class[HZ6_STATS_CLASS_COUNT] = {0};
  size_t frontcache_bin_max_by_class[HZ6_STATS_CLASS_COUNT] = {0};
  size_t frontcache_spill_success = 0;
  size_t frontcache_spill_retry_success = 0;
  size_t frontcache_borrow_success = 0;
  size_t toy_small_malloc_frontcache_pop = 0;
  size_t toy_small_malloc_activate_success = 0;
  size_t toy_class4_malloc_fast_attempt = 0;
  size_t toy_class4_malloc_fast_hit = 0;
  size_t toy_class4_malloc_front_dispatch = 0;
  size_t toy_class4_malloc_frontcache_pop = 0;
  size_t toy_class4_malloc_activate_success = 0;
  size_t toy_small_free_cache_attempt = 0;
  size_t toy_small_free_cache_success = 0;
  size_t toy_class4_free_cache_attempt = 0;
  size_t toy_class4_free_cache_success = 0;
  size_t toy_small_active_map_register = 0;
  size_t toy_small_active_map_register_collision = 0;
  size_t toy_class4_active_map_register = 0;
  size_t toy_class4_active_map_register_collision = 0;
  size_t toy_small_active_map_free_attempt = 0;
  size_t toy_small_active_map_free_hit = 0;
  size_t toy_small_active_map_free_hit_base_slot = 0;
  size_t toy_small_active_map_free_hit_probe_total = 0;
  size_t toy_small_active_map_free_hit_probe_max = 0;
  size_t toy_small_active_map_free_miss = 0;
  size_t toy_small_active_map_free_stale = 0;
  size_t toy_small_active_map_free_cache_fail = 0;
  size_t toy_small_active_map_route_bypass = 0;
  size_t toy_class4_active_map_free_hit = 0;
  size_t toy_class4_active_map_free_hit_base_slot = 0;
  size_t toy_class4_active_map_free_hit_probe_total = 0;
  size_t toy_class4_active_map_free_hit_probe_max = 0;
  size_t midpage_active_map_register = 0;
  size_t midpage_active_map_register_direct = 0;
  size_t midpage_active_map_register_front_alloc = 0;
  size_t midpage_active_map_register_route_fallback = 0;
  size_t midpage_active_map_register_collision = 0;
  size_t midpage_active_map_register_empty_slot = 0;
  size_t midpage_active_map_register_same_ptr = 0;
  size_t midpage_active_map_register_base_slot = 0;
  size_t midpage_active_map_register_probe_total = 0;
  size_t midpage_active_map_register_probe_max = 0;
  size_t midpage_active_map_register_overwrite = 0;
  size_t midpage_8k_active_map_register_overwrite = 0;
  size_t midpage_32k_active_map_register_overwrite = 0;
  size_t midpage_active_map_register_overwrite_same_class_alt = 0;
  size_t midpage_active_map_register_overwrite_stale_alt = 0;
  size_t midpage_active_map_free_attempt = 0;
  size_t midpage_active_map_free_hit = 0;
  size_t midpage_active_map_free_hit_base_slot = 0;
  size_t midpage_active_map_free_hit_probe_total = 0;
  size_t midpage_active_map_free_hit_probe_max = 0;
  size_t midpage_active_map_free_miss = 0;
  size_t midpage_active_map_free_miss_probe_empty = 0;
  size_t midpage_active_map_free_miss_probe_occupied = 0;
  size_t midpage_active_map_free_miss_found_elsewhere = 0;
  size_t midpage_8k_active_map_free_miss_found_elsewhere = 0;
  size_t midpage_32k_active_map_free_miss_found_elsewhere = 0;
  size_t midpage_active_map_free_stale = 0;
  size_t midpage_active_map_free_cache_attempt = 0;
  size_t midpage_active_map_free_cache_success = 0;
  size_t midpage_active_map_free_cache_fail = 0;
  size_t midpage_active_map_alignment_skip = 0;
  size_t midpage_active_map_addr_envelope_skip = 0;
  size_t midpage_active_map_route_bypass = 0;
  size_t midpage_8k_alloc_call = 0;
  size_t midpage_32k_alloc_call = 0;
  size_t midpage_8k_prefill_run_call = 0;
  size_t midpage_32k_prefill_run_call = 0;
  size_t midpage_8k_prefill_run_filled = 0;
  size_t midpage_32k_prefill_run_filled = 0;
  size_t midpage_8k_active_map_register = 0;
  size_t midpage_32k_active_map_register = 0;
  size_t midpage_8k_active_map_free_hit = 0;
  size_t midpage_32k_active_map_free_hit = 0;
  size_t midpage_8k_frontcache_push = 0;
  size_t midpage_32k_frontcache_push = 0;
  size_t midpage_8k_frontcache_pop_empty = 0;
  size_t midpage_32k_frontcache_pop_empty = 0;
  size_t midpage_direct_transfer_probe_attempt = 0;
  size_t midpage_direct_transfer_probe_hit = 0;
  size_t midpage_direct_transfer_probe_empty = 0;
  size_t midpage_8k_direct_transfer_probe_attempt = 0;
  size_t midpage_32k_direct_transfer_probe_attempt = 0;
  size_t midpage_8k_preload_local_route_valid = 0;
  size_t midpage_32k_preload_local_route_valid = 0;
  size_t midpage_8k_source_run_slot_route_register = 0;
  size_t midpage_32k_source_run_slot_route_register = 0;
  size_t midpage_32k_cold_retire_attempt = 0;
  size_t midpage_32k_cold_retire_scan_blocks = 0;
  size_t midpage_32k_cold_retire_candidate_blocks = 0;
  size_t midpage_32k_cold_retire_retired_blocks = 0;
  size_t midpage_32k_cold_retire_retired_descriptors = 0;
  size_t midpage_32k_cold_retire_retired_bytes = 0;
  size_t midpage_32k_cold_retire_frontcache_remove_fail = 0;
  size_t midpage_32k_cold_retire_blocked = 0;
  size_t midpage_32k_cold_purge_attempt = 0;
  size_t midpage_32k_cold_purge_scan_blocks = 0;
  size_t midpage_32k_cold_purge_candidate_blocks = 0;
  size_t midpage_32k_cold_purge_purged_blocks = 0;
  size_t midpage_32k_cold_purge_purged_bytes = 0;
  size_t midpage_32k_cold_purge_fail = 0;
  size_t midpage_32k_cold_purge_blocked = 0;
  size_t midpage_32k_cold_purge_recommit = 0;
  size_t midpage_32k_cold_purge_recommit_fail = 0;
  size_t smallrun_route_attempt = 0;
  size_t smallrun_range_hit = 0;
  size_t smallrun_active_slot_hit = 0;
  size_t smallrun_descriptor_match = 0;
  size_t smallrun_generation_match = 0;
  size_t smallrun_would_valid = 0;
  size_t smallrun_would_invalid = 0;
  size_t smallrun_exact_fallback_needed = 0;
  size_t smallrun_false_positive = 0;
  size_t source_block_route_range_index_lookup = 0;
  size_t source_block_route_range_index_hit = 0;
  size_t source_block_route_range_index_miss = 0;
  size_t source_block_route_range_index_stale = 0;
  size_t source_block_route_range_index_probe_total = 0;
  size_t source_block_route_range_index_probe_max = 0;
  size_t source_block_route_behavior_attempt = 0;
  size_t source_block_route_behavior_valid = 0;
  size_t source_block_route_behavior_fallback = 0;
  size_t source_block_route_behavior_invalid_front = 0;
  size_t source_block_route_behavior_invalid_descriptor = 0;
#if HZ6_DIAGNOSTIC_PROBES
  size_t memory_descriptor_table_bytes = 0;
  size_t memory_route_table_bytes = 0;
  size_t memory_source_block_table_bytes = 0;
  size_t memory_frontcache_table_bytes = 0;
  size_t memory_transfer_table_bytes = 0;
  size_t memory_ownerlocality_index_bytes = 0;
  size_t memory_static_table_bytes = 0;
  size_t memory_static_plus_payload_bytes = 0;
  size_t memory_source_block_payload_bytes = 0;
  size_t memory_frontcache_total = 0;
  size_t memory_frontcache_largest_bin = 0;
  size_t memory_active_source_blocks = 0;
  size_t memory_registered_source_blocks = 0;
  size_t memory_ref_nonzero_source_blocks = 0;
  size_t memory_ref_zero_source_blocks = 0;
  size_t memory_source_run_active_blocks = 0;
  size_t memory_toy_source_run_blocks = 0;
  size_t memory_midpage_source_run_blocks = 0;
  size_t memory_source_run_slot_count_max = 0;
  size_t memory_toy_source_run_slot_count_max = 0;
  size_t memory_midpage_8k_source_run_slot_count_max = 0;
  size_t memory_midpage_32k_source_run_slot_count_max = 0;
  size_t memory_midpage_8k_source_blocks = 0;
  size_t memory_midpage_32k_source_blocks = 0;
  size_t memory_midpage_8k_payload_bytes = 0;
  size_t memory_midpage_32k_payload_bytes = 0;
  size_t memory_midpage_8k_active_descriptors = 0;
  size_t memory_midpage_32k_active_descriptors = 0;
  size_t memory_midpage_8k_local_free_descriptors = 0;
  size_t memory_midpage_32k_local_free_descriptors = 0;
  size_t memory_midpage_8k_transfer_free_descriptors = 0;
  size_t memory_midpage_32k_transfer_free_descriptors = 0;
  size_t memory_midpage_8k_remote_pending_descriptors = 0;
  size_t memory_midpage_32k_remote_pending_descriptors = 0;
  size_t memory_midpage_8k_all_local_free_blocks = 0;
  size_t memory_midpage_32k_all_local_free_blocks = 0;
  size_t memory_midpage_8k_all_local_free_payload_bytes = 0;
  size_t memory_midpage_32k_all_local_free_payload_bytes = 0;
  size_t memory_midpage_8k_active_zero_local_free_nonzero_blocks = 0;
  size_t memory_midpage_32k_active_zero_local_free_nonzero_blocks = 0;
  size_t memory_midpage_8k_low_active_1_4_blocks = 0;
  size_t memory_midpage_32k_low_active_1_4_blocks = 0;
  size_t memory_midpage_8k_low_active_1_4_payload_bytes = 0;
  size_t memory_midpage_32k_low_active_1_4_payload_bytes = 0;
  size_t memory_midpage_ref_mismatch_blocks = 0;
  size_t memory_midpage_8k_retire_candidate_blocks = 0;
  size_t memory_midpage_32k_retire_candidate_blocks = 0;
  size_t memory_midpage_8k_retire_candidate_payload_bytes = 0;
  size_t memory_midpage_32k_retire_candidate_payload_bytes = 0;
  size_t memory_midpage_8k_retire_candidate_descriptors = 0;
  size_t memory_midpage_32k_retire_candidate_descriptors = 0;
  size_t memory_midpage_8k_retire_candidate_frontcache_entries = 0;
  size_t memory_midpage_32k_retire_candidate_frontcache_entries = 0;
#endif

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
    elastic_descriptor_overflow_alloc += stats.elastic_descriptor_overflow_alloc;
    elastic_descriptor_overflow_reset += stats.elastic_descriptor_overflow_reset;
    elastic_descriptor_overflow_exhausted += stats.elastic_descriptor_overflow_exhausted;
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
    route_register_reason_source_run_slot += stats.route_register_reason_source_run_slot;
    route_register_reason_direct_source += stats.route_register_reason_direct_source;
    route_register_reason_materialize += stats.route_register_reason_materialize;
    route_register_reason_rehome += stats.route_register_reason_rehome;
    route_unregister_reason_unknown += stats.route_unregister_reason_unknown;
    route_unregister_reason_frontcache_overflow += stats.route_unregister_reason_frontcache_overflow;
    route_unregister_reason_cap_release += stats.route_unregister_reason_cap_release;
    route_unregister_reason_descriptorless_detach += stats.route_unregister_reason_descriptorless_detach;
    route_unregister_reason_source_slot_release += stats.route_unregister_reason_source_slot_release;
    route_unregister_reason_rehome += stats.route_unregister_reason_rehome;
#include "hz6_preload_stats_print_rehome_accumulate.inc"
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
    route_compact_deferred += stats.route_compact_deferred;
    route_compact_owner_maintenance +=
        stats.route_compact_owner_maintenance;
    route_compact_remote_path_attempt +=
        stats.route_compact_remote_path_attempt;
    route_lock_read_contended += stats.route_lock_read_contended;
    route_lock_write_contended += stats.route_lock_write_contended;
    if (stats.route_lock_max_wait > route_lock_max_wait) {
      route_lock_max_wait = stats.route_lock_max_wait;
    }
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
#include "hz6_preload_stats_print_shared_route_accumulate.inc"
#include "hz6_preload_stats_print_resolve_accumulate.inc"
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
    for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
      frontcache_push_by_class[class_id] +=
          stats.frontcache_push_by_class[class_id];
      frontcache_pop_empty_by_class[class_id] +=
          stats.frontcache_pop_empty_by_class[class_id];
      if (stats.frontcache_bin_max_by_class[class_id] >
          frontcache_bin_max_by_class[class_id]) {
        frontcache_bin_max_by_class[class_id] =
            stats.frontcache_bin_max_by_class[class_id];
      }
    }
    frontcache_spill_success += stats.frontcache_spill_success;
    frontcache_spill_retry_success += stats.frontcache_spill_retry_success;
    frontcache_borrow_success += stats.frontcache_borrow_success;
    toy_small_malloc_frontcache_pop += stats.toy_small_malloc_frontcache_pop;
    toy_small_malloc_activate_success +=
        stats.toy_small_malloc_activate_success;
    toy_class4_malloc_fast_attempt += stats.toy_class4_malloc_fast_attempt;
    toy_class4_malloc_fast_hit += stats.toy_class4_malloc_fast_hit;
    toy_class4_malloc_front_dispatch +=
        stats.toy_class4_malloc_front_dispatch;
    toy_class4_malloc_frontcache_pop +=
        stats.toy_class4_malloc_frontcache_pop;
    toy_class4_malloc_activate_success +=
        stats.toy_class4_malloc_activate_success;
    toy_small_free_cache_attempt += stats.toy_small_free_cache_attempt;
    toy_small_free_cache_success += stats.toy_small_free_cache_success;
    toy_class4_free_cache_attempt += stats.toy_class4_free_cache_attempt;
    toy_class4_free_cache_success += stats.toy_class4_free_cache_success;
    toy_small_active_map_register += stats.toy_small_active_map_register;
    toy_small_active_map_register_collision +=
        stats.toy_small_active_map_register_collision;
    toy_class4_active_map_register += stats.toy_class4_active_map_register;
    toy_class4_active_map_register_collision +=
        stats.toy_class4_active_map_register_collision;
    toy_small_active_map_free_attempt +=
        stats.toy_small_active_map_free_attempt;
    toy_small_active_map_free_hit += stats.toy_small_active_map_free_hit;
    toy_small_active_map_free_hit_base_slot +=
        stats.toy_small_active_map_free_hit_base_slot;
    toy_small_active_map_free_hit_probe_total +=
        stats.toy_small_active_map_free_hit_probe_total;
    if (stats.toy_small_active_map_free_hit_probe_max >
        toy_small_active_map_free_hit_probe_max) {
      toy_small_active_map_free_hit_probe_max =
          stats.toy_small_active_map_free_hit_probe_max;
    }
    toy_small_active_map_free_miss += stats.toy_small_active_map_free_miss;
    toy_small_active_map_free_stale += stats.toy_small_active_map_free_stale;
    toy_small_active_map_free_cache_fail +=
        stats.toy_small_active_map_free_cache_fail;
    toy_small_active_map_route_bypass +=
        stats.toy_small_active_map_route_bypass;
    toy_class4_active_map_free_hit += stats.toy_class4_active_map_free_hit;
    toy_class4_active_map_free_hit_base_slot +=
        stats.toy_class4_active_map_free_hit_base_slot;
    toy_class4_active_map_free_hit_probe_total +=
        stats.toy_class4_active_map_free_hit_probe_total;
    if (stats.toy_class4_active_map_free_hit_probe_max >
        toy_class4_active_map_free_hit_probe_max) {
      toy_class4_active_map_free_hit_probe_max =
          stats.toy_class4_active_map_free_hit_probe_max;
    }
    midpage_active_map_register += stats.midpage_active_map_register;
    midpage_active_map_register_direct +=
        stats.midpage_active_map_register_direct;
    midpage_active_map_register_front_alloc +=
        stats.midpage_active_map_register_front_alloc;
    midpage_active_map_register_route_fallback +=
        stats.midpage_active_map_register_route_fallback;
    midpage_active_map_register_collision +=
        stats.midpage_active_map_register_collision;
    midpage_active_map_register_empty_slot +=
        stats.midpage_active_map_register_empty_slot;
    midpage_active_map_register_same_ptr +=
        stats.midpage_active_map_register_same_ptr;
    midpage_active_map_register_base_slot +=
        stats.midpage_active_map_register_base_slot;
    midpage_active_map_register_probe_total +=
        stats.midpage_active_map_register_probe_total;
    if (stats.midpage_active_map_register_probe_max >
        midpage_active_map_register_probe_max) {
      midpage_active_map_register_probe_max =
          stats.midpage_active_map_register_probe_max;
    }
    midpage_active_map_register_overwrite +=
        stats.midpage_active_map_register_overwrite;
    midpage_8k_active_map_register_overwrite +=
        stats.midpage_8k_active_map_register_overwrite;
    midpage_32k_active_map_register_overwrite +=
        stats.midpage_32k_active_map_register_overwrite;
    midpage_active_map_register_overwrite_same_class_alt +=
        stats.midpage_active_map_register_overwrite_same_class_alt;
    midpage_active_map_register_overwrite_stale_alt +=
        stats.midpage_active_map_register_overwrite_stale_alt;
    midpage_active_map_free_attempt +=
        stats.midpage_active_map_free_attempt;
    midpage_active_map_free_hit += stats.midpage_active_map_free_hit;
    midpage_active_map_free_hit_base_slot +=
        stats.midpage_active_map_free_hit_base_slot;
    midpage_active_map_free_hit_probe_total +=
        stats.midpage_active_map_free_hit_probe_total;
    if (stats.midpage_active_map_free_hit_probe_max >
        midpage_active_map_free_hit_probe_max) {
      midpage_active_map_free_hit_probe_max =
          stats.midpage_active_map_free_hit_probe_max;
    }
    midpage_active_map_free_miss += stats.midpage_active_map_free_miss;
    midpage_active_map_free_miss_probe_empty +=
        stats.midpage_active_map_free_miss_probe_empty;
    midpage_active_map_free_miss_probe_occupied +=
        stats.midpage_active_map_free_miss_probe_occupied;
    midpage_active_map_free_miss_found_elsewhere +=
        stats.midpage_active_map_free_miss_found_elsewhere;
    midpage_8k_active_map_free_miss_found_elsewhere +=
        stats.midpage_8k_active_map_free_miss_found_elsewhere;
    midpage_32k_active_map_free_miss_found_elsewhere +=
        stats.midpage_32k_active_map_free_miss_found_elsewhere;
    midpage_active_map_free_stale += stats.midpage_active_map_free_stale;
    midpage_active_map_free_cache_attempt +=
        stats.midpage_active_map_free_cache_attempt;
    midpage_active_map_free_cache_success +=
        stats.midpage_active_map_free_cache_success;
    midpage_active_map_free_cache_fail +=
        stats.midpage_active_map_free_cache_fail;
    midpage_active_map_alignment_skip +=
        stats.midpage_active_map_alignment_skip;
    midpage_active_map_addr_envelope_skip +=
        stats.midpage_active_map_addr_envelope_skip;
    midpage_active_map_route_bypass +=
        stats.midpage_active_map_route_bypass;
    midpage_8k_alloc_call += stats.midpage_8k_alloc_call;
    midpage_32k_alloc_call += stats.midpage_32k_alloc_call;
    midpage_8k_prefill_run_call += stats.midpage_8k_prefill_run_call;
    midpage_32k_prefill_run_call += stats.midpage_32k_prefill_run_call;
    midpage_8k_prefill_run_filled += stats.midpage_8k_prefill_run_filled;
    midpage_32k_prefill_run_filled += stats.midpage_32k_prefill_run_filled;
    midpage_8k_active_map_register += stats.midpage_8k_active_map_register;
    midpage_32k_active_map_register += stats.midpage_32k_active_map_register;
    midpage_8k_active_map_free_hit += stats.midpage_8k_active_map_free_hit;
    midpage_32k_active_map_free_hit += stats.midpage_32k_active_map_free_hit;
    midpage_8k_frontcache_push +=
        stats.frontcache_push_by_class[HZ6_MIDPAGE_8K_CLASS_ID];
    midpage_32k_frontcache_push +=
        stats.frontcache_push_by_class[HZ6_MIDPAGE_32K_CLASS_ID];
    midpage_8k_frontcache_pop_empty +=
        stats.frontcache_pop_empty_by_class[HZ6_MIDPAGE_8K_CLASS_ID];
    midpage_32k_frontcache_pop_empty +=
        stats.frontcache_pop_empty_by_class[HZ6_MIDPAGE_32K_CLASS_ID];
    midpage_direct_transfer_probe_attempt +=
        stats.midpage_direct_transfer_probe_attempt;
    midpage_direct_transfer_probe_hit +=
        stats.midpage_direct_transfer_probe_hit;
    midpage_direct_transfer_probe_empty +=
        stats.midpage_direct_transfer_probe_empty;
    midpage_8k_direct_transfer_probe_attempt +=
        stats.midpage_8k_direct_transfer_probe_attempt;
    midpage_32k_direct_transfer_probe_attempt +=
        stats.midpage_32k_direct_transfer_probe_attempt;
    midpage_8k_preload_local_route_valid +=
        stats.midpage_8k_preload_local_route_valid;
    midpage_32k_preload_local_route_valid +=
        stats.midpage_32k_preload_local_route_valid;
    midpage_8k_source_run_slot_route_register +=
        stats.midpage_8k_source_run_slot_route_register;
    midpage_32k_source_run_slot_route_register +=
        stats.midpage_32k_source_run_slot_route_register;
    midpage_32k_cold_retire_attempt +=
        stats.midpage_32k_cold_retire_attempt;
    midpage_32k_cold_retire_scan_blocks +=
        stats.midpage_32k_cold_retire_scan_blocks;
    midpage_32k_cold_retire_candidate_blocks +=
        stats.midpage_32k_cold_retire_candidate_blocks;
    midpage_32k_cold_retire_retired_blocks +=
        stats.midpage_32k_cold_retire_retired_blocks;
    midpage_32k_cold_retire_retired_descriptors +=
        stats.midpage_32k_cold_retire_retired_descriptors;
    midpage_32k_cold_retire_retired_bytes +=
        stats.midpage_32k_cold_retire_retired_bytes;
    midpage_32k_cold_retire_frontcache_remove_fail +=
        stats.midpage_32k_cold_retire_frontcache_remove_fail;
    midpage_32k_cold_retire_blocked +=
        stats.midpage_32k_cold_retire_blocked;
    midpage_32k_cold_purge_attempt +=
        stats.midpage_32k_cold_purge_attempt;
    midpage_32k_cold_purge_scan_blocks +=
        stats.midpage_32k_cold_purge_scan_blocks;
    midpage_32k_cold_purge_candidate_blocks +=
        stats.midpage_32k_cold_purge_candidate_blocks;
    midpage_32k_cold_purge_purged_blocks +=
        stats.midpage_32k_cold_purge_purged_blocks;
    midpage_32k_cold_purge_purged_bytes +=
        stats.midpage_32k_cold_purge_purged_bytes;
    midpage_32k_cold_purge_fail += stats.midpage_32k_cold_purge_fail;
    midpage_32k_cold_purge_blocked +=
        stats.midpage_32k_cold_purge_blocked;
    midpage_32k_cold_purge_recommit +=
        stats.midpage_32k_cold_purge_recommit;
    midpage_32k_cold_purge_recommit_fail +=
        stats.midpage_32k_cold_purge_recommit_fail;
    smallrun_route_attempt += stats.smallrun_route_attempt;
    smallrun_range_hit += stats.smallrun_range_hit;
    smallrun_active_slot_hit += stats.smallrun_active_slot_hit;
    smallrun_descriptor_match += stats.smallrun_descriptor_match;
    smallrun_generation_match += stats.smallrun_generation_match;
    smallrun_would_valid += stats.smallrun_would_valid;
    smallrun_would_invalid += stats.smallrun_would_invalid;
    smallrun_exact_fallback_needed += stats.smallrun_exact_fallback_needed;
    smallrun_false_positive += stats.smallrun_false_positive;
    source_block_route_range_index_lookup +=
        stats.source_block_route_range_index_lookup;
    source_block_route_range_index_hit +=
        stats.source_block_route_range_index_hit;
    source_block_route_range_index_miss +=
        stats.source_block_route_range_index_miss;
    source_block_route_range_index_stale +=
        stats.source_block_route_range_index_stale;
    source_block_route_range_index_probe_total +=
        stats.source_block_route_range_index_probe_total;
    if (stats.source_block_route_range_index_probe_max >
        source_block_route_range_index_probe_max) {
      source_block_route_range_index_probe_max =
          stats.source_block_route_range_index_probe_max;
    }
    source_block_route_behavior_attempt +=
        stats.source_block_route_behavior_attempt;
    source_block_route_behavior_valid += stats.source_block_route_behavior_valid;
    source_block_route_behavior_fallback +=
        stats.source_block_route_behavior_fallback;
    source_block_route_behavior_invalid_front +=
        stats.source_block_route_behavior_invalid_front;
    source_block_route_behavior_invalid_descriptor +=
        stats.source_block_route_behavior_invalid_descriptor;
#if HZ6_DIAGNOSTIC_PROBES
    memory_descriptor_table_bytes += stats.memory_descriptor_table_bytes;
    memory_route_table_bytes += stats.memory_route_table_bytes;
    memory_source_block_table_bytes += stats.memory_source_block_table_bytes;
    memory_frontcache_table_bytes += stats.memory_frontcache_table_bytes;
    memory_transfer_table_bytes += stats.memory_transfer_table_bytes;
    memory_ownerlocality_index_bytes += stats.memory_ownerlocality_index_bytes;
    memory_static_table_bytes += stats.memory_static_table_bytes;
    memory_static_plus_payload_bytes += stats.memory_static_plus_payload_bytes;
    memory_source_block_payload_bytes += stats.memory_source_block_payload_bytes;
    memory_frontcache_total += stats.memory_frontcache_total;
    if (stats.memory_frontcache_largest_bin > memory_frontcache_largest_bin) {
      memory_frontcache_largest_bin = stats.memory_frontcache_largest_bin;
    }
    memory_active_source_blocks += stats.memory_active_source_blocks;
    memory_registered_source_blocks += stats.memory_registered_source_blocks;
    memory_ref_nonzero_source_blocks += stats.memory_ref_nonzero_source_blocks;
    memory_ref_zero_source_blocks += stats.memory_ref_zero_source_blocks;
    memory_source_run_active_blocks +=
        stats.memory_source_run_active_blocks;
    memory_toy_source_run_blocks += stats.memory_toy_source_run_blocks;
    memory_midpage_source_run_blocks += stats.memory_midpage_source_run_blocks;
    if (stats.memory_source_run_slot_count_max >
        memory_source_run_slot_count_max) {
      memory_source_run_slot_count_max =
          stats.memory_source_run_slot_count_max;
    }
    if (stats.memory_toy_source_run_slot_count_max >
        memory_toy_source_run_slot_count_max) {
      memory_toy_source_run_slot_count_max =
          stats.memory_toy_source_run_slot_count_max;
    }
    if (stats.memory_midpage_8k_source_run_slot_count_max >
        memory_midpage_8k_source_run_slot_count_max) {
      memory_midpage_8k_source_run_slot_count_max =
          stats.memory_midpage_8k_source_run_slot_count_max;
    }
    if (stats.memory_midpage_32k_source_run_slot_count_max >
        memory_midpage_32k_source_run_slot_count_max) {
      memory_midpage_32k_source_run_slot_count_max =
          stats.memory_midpage_32k_source_run_slot_count_max;
    }
    memory_midpage_8k_source_blocks +=
        stats.memory_midpage_8k_source_blocks;
    memory_midpage_32k_source_blocks +=
        stats.memory_midpage_32k_source_blocks;
    memory_midpage_8k_payload_bytes +=
        stats.memory_midpage_8k_payload_bytes;
    memory_midpage_32k_payload_bytes +=
        stats.memory_midpage_32k_payload_bytes;
    memory_midpage_8k_active_descriptors +=
        stats.memory_midpage_8k_active_descriptors;
    memory_midpage_32k_active_descriptors +=
        stats.memory_midpage_32k_active_descriptors;
    memory_midpage_8k_local_free_descriptors +=
        stats.memory_midpage_8k_local_free_descriptors;
    memory_midpage_32k_local_free_descriptors +=
        stats.memory_midpage_32k_local_free_descriptors;
    memory_midpage_8k_transfer_free_descriptors +=
        stats.memory_midpage_8k_transfer_free_descriptors;
    memory_midpage_32k_transfer_free_descriptors +=
        stats.memory_midpage_32k_transfer_free_descriptors;
    memory_midpage_8k_remote_pending_descriptors +=
        stats.memory_midpage_8k_remote_pending_descriptors;
    memory_midpage_32k_remote_pending_descriptors +=
        stats.memory_midpage_32k_remote_pending_descriptors;
    memory_midpage_8k_all_local_free_blocks +=
        stats.memory_midpage_8k_all_local_free_blocks;
    memory_midpage_32k_all_local_free_blocks +=
        stats.memory_midpage_32k_all_local_free_blocks;
    memory_midpage_8k_all_local_free_payload_bytes +=
        stats.memory_midpage_8k_all_local_free_payload_bytes;
    memory_midpage_32k_all_local_free_payload_bytes +=
        stats.memory_midpage_32k_all_local_free_payload_bytes;
    memory_midpage_8k_active_zero_local_free_nonzero_blocks +=
        stats.memory_midpage_8k_active_zero_local_free_nonzero_blocks;
    memory_midpage_32k_active_zero_local_free_nonzero_blocks +=
        stats.memory_midpage_32k_active_zero_local_free_nonzero_blocks;
    memory_midpage_8k_low_active_1_4_blocks +=
        stats.memory_midpage_8k_low_active_1_4_blocks;
    memory_midpage_32k_low_active_1_4_blocks +=
        stats.memory_midpage_32k_low_active_1_4_blocks;
    memory_midpage_8k_low_active_1_4_payload_bytes +=
        stats.memory_midpage_8k_low_active_1_4_payload_bytes;
    memory_midpage_32k_low_active_1_4_payload_bytes +=
        stats.memory_midpage_32k_low_active_1_4_payload_bytes;
    memory_midpage_ref_mismatch_blocks +=
        stats.memory_midpage_ref_mismatch_blocks;
    memory_midpage_8k_retire_candidate_blocks +=
        stats.memory_midpage_8k_retire_candidate_blocks;
    memory_midpage_32k_retire_candidate_blocks +=
        stats.memory_midpage_32k_retire_candidate_blocks;
    memory_midpage_8k_retire_candidate_payload_bytes +=
        stats.memory_midpage_8k_retire_candidate_payload_bytes;
    memory_midpage_32k_retire_candidate_payload_bytes +=
        stats.memory_midpage_32k_retire_candidate_payload_bytes;
    memory_midpage_8k_retire_candidate_descriptors +=
        stats.memory_midpage_8k_retire_candidate_descriptors;
    memory_midpage_32k_retire_candidate_descriptors +=
        stats.memory_midpage_32k_retire_candidate_descriptors;
    memory_midpage_8k_retire_candidate_frontcache_entries +=
        stats.memory_midpage_8k_retire_candidate_frontcache_entries;
    memory_midpage_32k_retire_candidate_frontcache_entries +=
        stats.memory_midpage_32k_retire_candidate_frontcache_entries;
#endif
  }
  pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);

  Hz6LinuxMmapRetainStats retain_stats =
      hz6_linux_mmap_retain_stats_snapshot();
#if HZ6_DIAGNOSTIC_PROBES
  size_t toy_active_map_table_bytes = 0;
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  toy_active_map_table_bytes =
      allocator_count * sizeof(Hz6ToySmallActiveMapEntry) *
      HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY;
#endif
  size_t midpage_active_map_table_bytes = 0;
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  midpage_active_map_table_bytes =
      allocator_count * sizeof(Hz6MidPageActiveMapEntry) *
      HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY;
#endif
  size_t preload_attributed_bytes =
      memory_static_plus_payload_bytes + toy_active_map_table_bytes +
      midpage_active_map_table_bytes + retain_stats.retained_bytes;
#endif
