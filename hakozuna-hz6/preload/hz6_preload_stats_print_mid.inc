  fprintf(stderr,
          "[HZ6_PRELOAD_STATS] allocators=%zu route_valid=%zu "
          "route_invalid=%zu route_miss=%zu transfer_push=%zu "
          "transfer_pop=%zu source_alloc=%zu toy_source_alloc=%zu "
          "midpage_source_alloc=%zu large_source_alloc=%zu "
          "local2p_source_alloc=%zu alloc_fail=%zu "
          "descriptor_exhausted=%zu "
          "elastic_descriptor_overflow_alloc=%zu "
          "elastic_descriptor_overflow_reset=%zu "
          "elastic_descriptor_overflow_exhausted=%zu "
          "route_register_fail=%zu "
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
          alloc_fail, descriptor_exhausted,
          elastic_descriptor_overflow_alloc,
          elastic_descriptor_overflow_reset,
          elastic_descriptor_overflow_exhausted,
          route_register_fail,
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
          "route_compact_deferred=%zu "
          "route_compact_owner_maintenance=%zu "
          "route_compact_remote_path_attempt=%zu "
          "route_lock_read_contended=%zu "
          "route_lock_write_contended=%zu "
          "route_lock_max_wait=%zu "
          "route_tombstone_cond_probe=%zu "
          "route_tombstone_cond_would_compact=%zu "
          "route_tombstone_cond_ratio25=%zu "
          "route_tombstone_cond_occupancy75=%zu "
          "route_tombstone_cond_cooldown_blocked=%zu "
          "route_tombstone_cond_highwater=%zu "
          "shared_dir_tombstone_current=%zu "
          "shared_dir_tombstone_max=%zu "
          "shared_dir_register_used_tombstone=%zu "
          "shared_dir_maintenance_attempt=%zu "
          "shared_dir_maintenance_success=%zu "
          "shared_dir_maintenance_cleared=%zu\n",
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
          route_tombstone_compact_moved, route_compact_deferred,
          route_compact_owner_maintenance, route_compact_remote_path_attempt,
          route_lock_read_contended, route_lock_write_contended,
          route_lock_max_wait,
          route_tombstone_cond_probe,
          route_tombstone_cond_would_compact,
          route_tombstone_cond_ratio25, route_tombstone_cond_occupancy75,
          route_tombstone_cond_cooldown_blocked,
          route_tombstone_cond_highwater,
          shared_dir_tombstone_current, shared_dir_tombstone_max,
          shared_dir_register_used_tombstone,
          shared_dir_maintenance_attempt, shared_dir_maintenance_success,
          shared_dir_maintenance_cleared);

  fprintf(stderr,
          "[HZ6_PRELOAD_RESOLVE_DETAIL] "
          "free_resolve_attempt=%zu "
          "free_resolve_shared_probe=%zu "
          "free_resolve_shared_valid=%zu "
          "free_resolve_shared_miss=%zu "
          "free_resolve_shared_retry=%zu "
          "free_resolve_shared_stale=%zu "
          "free_resolve_local_lookup=%zu "
          "free_resolve_visible_shadow=%zu "
          "free_resolve_visible_hit=%zu "
          "free_resolve_result_local_valid=%zu "
          "free_resolve_result_foreign_valid=%zu "
          "free_resolve_result_owned_invalid=%zu "
          "free_resolve_result_proven_external=%zu "
          "free_resolve_result_retry=%zu "
          "free_resolve_result_unresolved_integrity=%zu\n",
          free_resolve_attempt, free_resolve_shared_probe,
          free_resolve_shared_valid, free_resolve_shared_miss,
          free_resolve_shared_retry, free_resolve_shared_stale,
          free_resolve_local_lookup, free_resolve_visible_shadow,
          free_resolve_visible_hit, free_resolve_result_local_valid,
          free_resolve_result_foreign_valid,
          free_resolve_result_owned_invalid,
          free_resolve_result_proven_external, free_resolve_result_retry,
          free_resolve_result_unresolved_integrity);

  fprintf(stderr,
          "[HZ6_PRELOAD_FRONT_DETAIL] descriptor_live_max=%zu "
          "source_block_active_max=%zu frontcache_total_max=%zu "
          "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
          "frontcache_spill_success=%zu "
          "frontcache_spill_retry_success=%zu "
          "frontcache_borrow_success=%zu "
          "toy_small_malloc_frontcache_pop=%zu "
          "toy_small_malloc_activate_success=%zu "
          "toy_class4_malloc_fast_attempt=%zu "
          "toy_class4_malloc_fast_hit=%zu "
          "toy_class4_malloc_front_dispatch=%zu "
          "toy_class4_malloc_frontcache_pop=%zu "
          "toy_class4_malloc_activate_success=%zu "
          "toy_small_free_cache_attempt=%zu "
          "toy_small_free_cache_success=%zu "
          "toy_class4_free_cache_attempt=%zu "
          "toy_class4_free_cache_success=%zu "
          "toy_small_active_map_register=%zu "
          "toy_small_active_map_register_collision=%zu "
          "toy_class4_active_map_register=%zu "
          "toy_class4_active_map_register_collision=%zu "
          "toy_small_active_map_free_attempt=%zu "
          "toy_small_active_map_free_hit=%zu "
          "toy_small_active_map_free_hit_base_slot=%zu "
          "toy_small_active_map_free_hit_probe_total=%zu "
          "toy_small_active_map_free_hit_probe_max=%zu "
          "toy_small_active_map_free_miss=%zu "
          "toy_small_active_map_free_stale=%zu "
          "toy_small_active_map_free_cache_fail=%zu "
          "toy_small_active_map_route_bypass=%zu "
          "toy_class4_active_map_free_hit=%zu "
          "toy_class4_active_map_free_hit_base_slot=%zu "
          "toy_class4_active_map_free_hit_probe_total=%zu "
          "toy_class4_active_map_free_hit_probe_max=%zu "
          "midpage_active_map_register=%zu "
          "midpage_active_map_register_direct=%zu "
          "midpage_active_map_register_front_alloc=%zu "
          "midpage_active_map_register_route_fallback=%zu "
          "midpage_active_map_register_collision=%zu "
          "midpage_active_map_register_empty_slot=%zu "
          "midpage_active_map_register_same_ptr=%zu "
          "midpage_active_map_register_base_slot=%zu "
          "midpage_active_map_register_probe_total=%zu "
          "midpage_active_map_register_probe_max=%zu "
          "midpage_active_map_register_overwrite=%zu "
          "midpage_8k_active_map_register_overwrite=%zu "
          "midpage_32k_active_map_register_overwrite=%zu "
          "midpage_active_map_register_overwrite_same_class_alt=%zu "
          "midpage_active_map_register_overwrite_stale_alt=%zu "
          "midpage_active_map_free_attempt=%zu "
          "midpage_active_map_free_hit=%zu "
          "midpage_active_map_free_hit_base_slot=%zu "
          "midpage_active_map_free_hit_probe_total=%zu "
          "midpage_active_map_free_hit_probe_max=%zu "
          "midpage_active_map_free_miss=%zu "
          "midpage_active_map_free_miss_probe_empty=%zu "
          "midpage_active_map_free_miss_probe_occupied=%zu "
          "midpage_active_map_free_miss_found_elsewhere=%zu "
          "midpage_8k_active_map_free_miss_found_elsewhere=%zu "
          "midpage_32k_active_map_free_miss_found_elsewhere=%zu "
          "midpage_active_map_free_stale=%zu "
          "midpage_active_map_free_cache_attempt=%zu "
          "midpage_active_map_free_cache_success=%zu "
          "midpage_active_map_free_cache_fail=%zu "
          "midpage_active_map_alignment_skip=%zu "
          "midpage_active_map_addr_envelope_skip=%zu "
          "midpage_active_map_route_bypass=%zu\n",
          descriptor_live_max, source_block_active_max,
          frontcache_total_max, frontcache_reuse_hit,
          frontcache_reuse_invalid, frontcache_spill_success,
          frontcache_spill_retry_success, frontcache_borrow_success,
          toy_small_malloc_frontcache_pop,
          toy_small_malloc_activate_success,
          toy_class4_malloc_fast_attempt,
          toy_class4_malloc_fast_hit,
          toy_class4_malloc_front_dispatch,
          toy_class4_malloc_frontcache_pop,
          toy_class4_malloc_activate_success,
          toy_small_free_cache_attempt, toy_small_free_cache_success,
          toy_class4_free_cache_attempt, toy_class4_free_cache_success,
          toy_small_active_map_register,
          toy_small_active_map_register_collision,
          toy_class4_active_map_register,
          toy_class4_active_map_register_collision,
          toy_small_active_map_free_attempt,
          toy_small_active_map_free_hit,
          toy_small_active_map_free_hit_base_slot,
          toy_small_active_map_free_hit_probe_total,
          toy_small_active_map_free_hit_probe_max,
          toy_small_active_map_free_miss,
          toy_small_active_map_free_stale,
          toy_small_active_map_free_cache_fail,
          toy_small_active_map_route_bypass,
          toy_class4_active_map_free_hit,
          toy_class4_active_map_free_hit_base_slot,
          toy_class4_active_map_free_hit_probe_total,
          toy_class4_active_map_free_hit_probe_max,
          midpage_active_map_register,
          midpage_active_map_register_direct,
          midpage_active_map_register_front_alloc,
          midpage_active_map_register_route_fallback,
          midpage_active_map_register_collision,
          midpage_active_map_register_empty_slot,
          midpage_active_map_register_same_ptr,
          midpage_active_map_register_base_slot,
          midpage_active_map_register_probe_total,
          midpage_active_map_register_probe_max,
          midpage_active_map_register_overwrite,
          midpage_8k_active_map_register_overwrite,
          midpage_32k_active_map_register_overwrite,
          midpage_active_map_register_overwrite_same_class_alt,
          midpage_active_map_register_overwrite_stale_alt,
          midpage_active_map_free_attempt,
          midpage_active_map_free_hit,
          midpage_active_map_free_hit_base_slot,
          midpage_active_map_free_hit_probe_total,
          midpage_active_map_free_hit_probe_max,
          midpage_active_map_free_miss,
          midpage_active_map_free_miss_probe_empty,
          midpage_active_map_free_miss_probe_occupied,
          midpage_active_map_free_miss_found_elsewhere,
          midpage_8k_active_map_free_miss_found_elsewhere,
          midpage_32k_active_map_free_miss_found_elsewhere,
          midpage_active_map_free_stale,
          midpage_active_map_free_cache_attempt,
          midpage_active_map_free_cache_success,
          midpage_active_map_free_cache_fail,
          midpage_active_map_alignment_skip,
          midpage_active_map_addr_envelope_skip,
          midpage_active_map_route_bypass);

  fprintf(stderr,
          "[HZ6_PRELOAD_PAGE_KIND_DETAIL] "
          "free_page_kind_selector_probe=%zu "
          "free_page_kind_selector_unknown=%zu "
          "free_page_kind_selector_toy=%zu "
          "free_page_kind_selector_midpage=%zu "
          "free_page_kind_selector_mixed=%zu "
          "free_page_kind_selector_toy_hit=%zu "
          "free_page_kind_selector_midpage_hit=%zu "
          "free_page_kind_selector_wrong_toy_page_mid_hit=%zu "
          "free_page_kind_selector_wrong_midpage_page_toy_hit=%zu\n",
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_probe),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_unknown),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_toy),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_midpage),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_mixed),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_toy_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_midpage_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_page_kind_selector_wrong_toy_page_mid_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_page_kind_selector_wrong_midpage_page_toy_hit));

  fprintf(stderr, "[HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL]");
  for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
    fprintf(stderr,
            " c%zu_push=%zu c%zu_empty=%zu c%zu_max=%zu",
            class_id,
            frontcache_push_by_class[class_id],
            class_id,
            frontcache_pop_empty_by_class[class_id],
            class_id,
            frontcache_bin_max_by_class[class_id]);
  }
  fprintf(stderr, "\n");

  fprintf(stderr,
          "[HZ6_PRELOAD_MIDPAGE_CLASS_DETAIL] "
          "midpage_8k_alloc_call=%zu midpage_32k_alloc_call=%zu "
          "midpage_8k_prefill_run_call=%zu "
          "midpage_32k_prefill_run_call=%zu "
          "midpage_8k_prefill_run_filled=%zu "
          "midpage_32k_prefill_run_filled=%zu "
          "midpage_8k_active_map_register=%zu "
          "midpage_32k_active_map_register=%zu "
          "midpage_8k_active_map_free_hit=%zu "
          "midpage_32k_active_map_free_hit=%zu "
          "midpage_8k_frontcache_push=%zu "
          "midpage_32k_frontcache_push=%zu "
          "midpage_8k_frontcache_pop_empty=%zu "
          "midpage_32k_frontcache_pop_empty=%zu "
          "midpage_direct_transfer_probe_attempt=%zu "
          "midpage_direct_transfer_probe_hit=%zu "
          "midpage_direct_transfer_probe_empty=%zu "
          "midpage_8k_direct_transfer_probe_attempt=%zu "
          "midpage_32k_direct_transfer_probe_attempt=%zu "
          "midpage_8k_preload_local_route_valid=%zu "
          "midpage_32k_preload_local_route_valid=%zu "
          "midpage_8k_source_run_slot_route_register=%zu "
          "midpage_32k_source_run_slot_route_register=%zu "
          "midpage_32k_cold_retire_attempt=%zu "
          "midpage_32k_cold_retire_scan_blocks=%zu "
          "midpage_32k_cold_retire_candidate_blocks=%zu "
          "midpage_32k_cold_retire_retired_blocks=%zu "
          "midpage_32k_cold_retire_retired_descriptors=%zu "
          "midpage_32k_cold_retire_retired_bytes=%zu "
          "midpage_32k_cold_retire_frontcache_remove_fail=%zu "
          "midpage_32k_cold_retire_blocked=%zu "
          "midpage_32k_cold_purge_attempt=%zu "
          "midpage_32k_cold_purge_scan_blocks=%zu "
          "midpage_32k_cold_purge_candidate_blocks=%zu "
          "midpage_32k_cold_purge_purged_blocks=%zu "
          "midpage_32k_cold_purge_purged_bytes=%zu "
          "midpage_32k_cold_purge_fail=%zu "
          "midpage_32k_cold_purge_blocked=%zu "
          "midpage_32k_cold_purge_recommit=%zu "
          "midpage_32k_cold_purge_recommit_fail=%zu\n",
          midpage_8k_alloc_call, midpage_32k_alloc_call,
          midpage_8k_prefill_run_call, midpage_32k_prefill_run_call,
          midpage_8k_prefill_run_filled, midpage_32k_prefill_run_filled,
          midpage_8k_active_map_register, midpage_32k_active_map_register,
          midpage_8k_active_map_free_hit, midpage_32k_active_map_free_hit,
          midpage_8k_frontcache_push, midpage_32k_frontcache_push,
          midpage_8k_frontcache_pop_empty, midpage_32k_frontcache_pop_empty,
          midpage_direct_transfer_probe_attempt,
          midpage_direct_transfer_probe_hit,
          midpage_direct_transfer_probe_empty,
          midpage_8k_direct_transfer_probe_attempt,
          midpage_32k_direct_transfer_probe_attempt,
          midpage_8k_preload_local_route_valid,
          midpage_32k_preload_local_route_valid,
          midpage_8k_source_run_slot_route_register,
          midpage_32k_source_run_slot_route_register,
          midpage_32k_cold_retire_attempt,
          midpage_32k_cold_retire_scan_blocks,
          midpage_32k_cold_retire_candidate_blocks,
          midpage_32k_cold_retire_retired_blocks,
          midpage_32k_cold_retire_retired_descriptors,
          midpage_32k_cold_retire_retired_bytes,
          midpage_32k_cold_retire_frontcache_remove_fail,
          midpage_32k_cold_retire_blocked,
          midpage_32k_cold_purge_attempt,
          midpage_32k_cold_purge_scan_blocks,
          midpage_32k_cold_purge_candidate_blocks,
          midpage_32k_cold_purge_purged_blocks,
          midpage_32k_cold_purge_purged_bytes,
          midpage_32k_cold_purge_fail,
          midpage_32k_cold_purge_blocked,
          midpage_32k_cold_purge_recommit,
          midpage_32k_cold_purge_recommit_fail);

  fprintf(stderr,
          "[HZ6_PRELOAD_RUNMETA_DETAIL] "
          "smallrun_route_attempt=%zu "
          "smallrun_range_hit=%zu "
          "smallrun_active_slot_hit=%zu "
          "smallrun_descriptor_match=%zu "
          "smallrun_generation_match=%zu "
          "smallrun_would_valid=%zu "
          "smallrun_would_invalid=%zu "
          "smallrun_exact_fallback_needed=%zu "
          "smallrun_false_positive=%zu "
          "source_block_route_range_index_lookup=%zu "
          "source_block_route_range_index_hit=%zu "
          "source_block_route_range_index_miss=%zu "
          "source_block_route_range_index_stale=%zu "
          "source_block_route_range_index_probe_total=%zu "
          "source_block_route_range_index_probe_max=%zu "
          "source_block_route_behavior_attempt=%zu "
          "source_block_route_behavior_valid=%zu "
          "source_block_route_behavior_fallback=%zu "
          "source_block_route_behavior_invalid_front=%zu "
          "source_block_route_behavior_invalid_descriptor=%zu\n",
          smallrun_route_attempt, smallrun_range_hit,
          smallrun_active_slot_hit, smallrun_descriptor_match,
          smallrun_generation_match, smallrun_would_valid,
          smallrun_would_invalid, smallrun_exact_fallback_needed,
          smallrun_false_positive, source_block_route_range_index_lookup,
          source_block_route_range_index_hit,
          source_block_route_range_index_miss,
          source_block_route_range_index_stale,
          source_block_route_range_index_probe_total,
          source_block_route_range_index_probe_max,
          source_block_route_behavior_attempt,
          source_block_route_behavior_valid,
          source_block_route_behavior_fallback,
          source_block_route_behavior_invalid_front,
          source_block_route_behavior_invalid_descriptor);
