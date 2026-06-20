#if HZ6_DIAGNOSTIC_PROBES
  fprintf(stderr,
          "[HZ6_PRELOAD_MEMORY_ATTR] "
          "allocator_count=%zu "
          "descriptor_table_bytes=%zu "
          "route_table_bytes=%zu "
          "source_block_table_bytes=%zu "
          "frontcache_table_bytes=%zu "
          "transfer_table_bytes=%zu "
          "ownerlocality_index_bytes=%zu "
          "toy_active_map_table_bytes=%zu "
          "midpage_active_map_table_bytes=%zu "
          "static_table_bytes=%zu "
          "source_block_payload_bytes=%zu "
          "static_plus_payload_bytes=%zu "
          "retain_retained_bytes=%zu "
          "preload_attributed_bytes=%zu "
          "frontcache_total=%zu "
          "frontcache_largest_bin=%zu "
          "active_source_blocks=%zu "
          "registered_source_blocks=%zu "
          "ref_nonzero_source_blocks=%zu "
          "ref_zero_source_blocks=%zu "
          "source_run_active_blocks=%zu "
          "toy_source_run_blocks=%zu "
          "midpage_source_run_blocks=%zu "
          "source_run_slot_count_max=%zu "
          "toy_source_run_slot_count_max=%zu "
          "midpage_8k_source_run_slot_count_max=%zu "
          "midpage_32k_source_run_slot_count_max=%zu "
          "midpage_8k_source_blocks=%zu "
          "midpage_32k_source_blocks=%zu "
          "midpage_8k_payload_bytes=%zu "
          "midpage_32k_payload_bytes=%zu "
          "midpage_8k_active=%zu "
          "midpage_32k_active=%zu "
          "midpage_8k_local_free=%zu "
          "midpage_32k_local_free=%zu "
          "midpage_8k_transfer_free=%zu "
          "midpage_32k_transfer_free=%zu "
          "midpage_8k_remote_pending=%zu "
          "midpage_32k_remote_pending=%zu "
          "midpage_8k_all_local_free_blocks=%zu "
          "midpage_32k_all_local_free_blocks=%zu "
          "midpage_8k_all_local_free_payload_bytes=%zu "
          "midpage_32k_all_local_free_payload_bytes=%zu "
          "midpage_8k_active_zero_local_free_nonzero_blocks=%zu "
          "midpage_32k_active_zero_local_free_nonzero_blocks=%zu "
          "midpage_8k_low_active_1_4_blocks=%zu "
          "midpage_32k_low_active_1_4_blocks=%zu "
          "midpage_8k_low_active_1_4_payload_bytes=%zu "
          "midpage_32k_low_active_1_4_payload_bytes=%zu "
          "midpage_ref_mismatch_blocks=%zu "
          "midpage_8k_retire_candidate_blocks=%zu "
          "midpage_32k_retire_candidate_blocks=%zu "
          "midpage_8k_retire_candidate_payload_bytes=%zu "
          "midpage_32k_retire_candidate_payload_bytes=%zu "
          "midpage_8k_retire_candidate_descriptors=%zu "
          "midpage_32k_retire_candidate_descriptors=%zu "
          "midpage_8k_retire_candidate_frontcache_entries=%zu "
          "midpage_32k_retire_candidate_frontcache_entries=%zu\n",
          allocator_count, memory_descriptor_table_bytes,
          memory_route_table_bytes, memory_source_block_table_bytes,
          memory_frontcache_table_bytes, memory_transfer_table_bytes,
          memory_ownerlocality_index_bytes, toy_active_map_table_bytes,
          midpage_active_map_table_bytes, memory_static_table_bytes,
          memory_source_block_payload_bytes,
          memory_static_plus_payload_bytes, retain_stats.retained_bytes,
          preload_attributed_bytes, memory_frontcache_total,
          memory_frontcache_largest_bin, memory_active_source_blocks,
          memory_registered_source_blocks, memory_ref_nonzero_source_blocks,
          memory_ref_zero_source_blocks, memory_source_run_active_blocks,
          memory_toy_source_run_blocks, memory_midpage_source_run_blocks,
          memory_source_run_slot_count_max,
          memory_toy_source_run_slot_count_max,
          memory_midpage_8k_source_run_slot_count_max,
          memory_midpage_32k_source_run_slot_count_max,
          memory_midpage_8k_source_blocks,
          memory_midpage_32k_source_blocks, memory_midpage_8k_payload_bytes,
          memory_midpage_32k_payload_bytes,
          memory_midpage_8k_active_descriptors,
          memory_midpage_32k_active_descriptors,
          memory_midpage_8k_local_free_descriptors,
          memory_midpage_32k_local_free_descriptors,
          memory_midpage_8k_transfer_free_descriptors,
          memory_midpage_32k_transfer_free_descriptors,
          memory_midpage_8k_remote_pending_descriptors,
          memory_midpage_32k_remote_pending_descriptors,
          memory_midpage_8k_all_local_free_blocks,
          memory_midpage_32k_all_local_free_blocks,
          memory_midpage_8k_all_local_free_payload_bytes,
          memory_midpage_32k_all_local_free_payload_bytes,
          memory_midpage_8k_active_zero_local_free_nonzero_blocks,
          memory_midpage_32k_active_zero_local_free_nonzero_blocks,
          memory_midpage_8k_low_active_1_4_blocks,
          memory_midpage_32k_low_active_1_4_blocks,
          memory_midpage_8k_low_active_1_4_payload_bytes,
          memory_midpage_32k_low_active_1_4_payload_bytes,
          memory_midpage_ref_mismatch_blocks,
          memory_midpage_8k_retire_candidate_blocks,
          memory_midpage_32k_retire_candidate_blocks,
          memory_midpage_8k_retire_candidate_payload_bytes,
          memory_midpage_32k_retire_candidate_payload_bytes,
          memory_midpage_8k_retire_candidate_descriptors,
          memory_midpage_32k_retire_candidate_descriptors,
          memory_midpage_8k_retire_candidate_frontcache_entries,
          memory_midpage_32k_retire_candidate_frontcache_entries);
#endif

  fprintf(stderr,
          "[HZ6_PRELOAD_PHASE_STATS] malloc_calls=%zu "
          "malloc_hz6_success=%zu malloc_real_fallback=%zu "
          "malloc_midpage_boundary_attempt=%zu "
          "malloc_midpage_boundary_hit=%zu "
          "malloc_midpage_boundary_fallback=%zu "
          "malloc_realloc_boundary_slack_4k=%zu "
          "malloc_realloc_boundary_slack_8k=%zu "
          "calloc_calls=%zu calloc_zero_bytes=%zu "
          "free_calls=%zu free_null=%zu "
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
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_midpage_boundary_attempt),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_midpage_boundary_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_midpage_boundary_fallback),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_realloc_boundary_slack_4k),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_realloc_boundary_slack_8k),
          hz6_preload_phase_load(&g_hz6_preload_phase_stats.calloc_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_zero_bytes),
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
          "[HZ6_PRELOAD_WRAPPER_DETAIL] "
          "posix_memalign_calls=%zu posix_memalign_hz6_path=%zu "
          "posix_memalign_real_fallback=%zu "
          "posix_memalign_align_le16=%zu "
          "posix_memalign_align_17_64=%zu "
          "posix_memalign_align_65_4096=%zu "
          "posix_memalign_align_gt4096=%zu "
          "aligned_alloc_calls=%zu aligned_alloc_hz6_path=%zu "
          "aligned_alloc_real_fallback=%zu "
          "aligned_alloc_align_le16=%zu "
          "aligned_alloc_align_17_64=%zu "
          "aligned_alloc_align_65_4096=%zu "
          "aligned_alloc_align_gt4096=%zu "
          "real_aligned_record_set=%zu "
          "real_aligned_record_fail=%zu "
          "real_aligned_free_skip_hit=%zu "
          "real_aligned_free_skip_miss=%zu "
          "calloc_real_record_set=%zu "
          "calloc_real_record_fail=%zu "
          "calloc_real_free_skip_hit=%zu "
          "calloc_real_free_skip_miss=%zu\n",
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_hz6_path),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_real_fallback),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_align_le16),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_align_17_64),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_align_65_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_align_gt4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_hz6_path),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_real_fallback),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_align_le16),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_align_17_64),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_align_65_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_align_gt4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.real_aligned_record_set),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.real_aligned_record_fail),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.real_aligned_free_skip_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.real_aligned_free_skip_miss),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_real_record_set),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_real_record_fail),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_real_free_skip_hit),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_real_free_skip_miss));

  fprintf(stderr,
          "[HZ6_PRELOAD_HOOK_DETAIL] free_toy_active_map_attempt=%zu free_toy_active_map_hit=%zu "
          "free_toy_active_map_miss=%zu free_midpage_active_map_attempt=%zu "
          "free_midpage_active_map_hit=%zu free_midpage_active_map_miss=%zu "
          "free_source_run_route_attempt=%zu "
          "free_source_run_route_hit=%zu "
          "free_source_run_route_midpage8_hit=%zu "
          "free_source_run_route_midpage32_hit=%zu "
          "free_source_run_route_invalid=%zu "
          "free_source_run_route_fallback=%zu "
          "free_source_run_route_prechecked=%zu "
          "free_route_lookup_after_maps=%zu free_route_valid_owned=%zu "
          "free_route_valid_foreign_visible=%zu free_route_invalid=%zu "
          "free_route_miss_real=%zu free_route_retry_abort=%zu "
          "free_route_integrity_abort=%zu "
          "free_route_real_free_unproven=%zu "
          "free_route_before_maps_attempt=%zu "
          "free_route_before_maps_arm_skip=%zu "
          "free_route_before_maps_foreign_dispatch=%zu "
          "free_route_before_maps_local_dispatch=%zu "
          "free_route_before_maps_external_dispatch=%zu "
          "free_route_before_maps_fallback=%zu "
          "free_route_before_maps_fallback_local_valid=%zu "
          "free_route_before_maps_fallback_owned_invalid=%zu "
          "free_route_before_maps_fallback_proven_external=%zu "
          "free_route_before_maps_fallback_other=%zu "
          "free_resolved_foreign_direct_dispatch=%zu "
          "free_resolved_duplicate_route_lookup_avoided=%zu "
          "mh_probe=%zu mh_would=%zu "
          "mh_true=%zu mh_false=%zu mh_miss=%zu\n",
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_toy_active_map_attempt),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_toy_active_map_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_toy_active_map_miss),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_active_map_attempt),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_active_map_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_active_map_miss),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_attempt),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_midpage8_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_midpage32_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_invalid),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_fallback),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_source_run_route_prechecked),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_lookup_after_maps),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_valid_owned),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_valid_foreign_visible),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_invalid),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_miss_real),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_retry_abort),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_integrity_abort),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_real_free_unproven),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_before_maps_attempt),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_before_maps_arm_skip),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_foreign_dispatch),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_local_dispatch),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_external_dispatch),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_route_before_maps_fallback),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_fallback_local_valid),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_fallback_owned_invalid),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_fallback_proven_external),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_route_before_maps_fallback_other),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_resolved_foreign_direct_dispatch),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_resolved_duplicate_route_lookup_avoided),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_probe),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_would_first), HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_true_midpage),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_false_positive), HZ6_PRELOAD_PHASE_LOAD_FIELD(free_midpage_hint_missed_midpage));
  fprintf(stderr,
          "[HZ6_PRELOAD_SIZE_STATS] "
          "malloc_size_zero=%zu malloc_size_le1024=%zu "
          "malloc_size_1025_4096=%zu malloc_size_4097_16384=%zu "
          "malloc_size_gt16384=%zu "
          "malloc_toy_direct_class_eligible=%zu "
          "malloc_toy_direct_class_eligible_le1024=%zu "
          "malloc_toy_direct_class_eligible_1025_4096=%zu "
          "malloc_toy_direct_class_enter=%zu "
          "malloc_toy_direct_class_enter_le1024=%zu "
          "malloc_toy_direct_class_enter_1025_4096=%zu "
          "calloc_size_zero=%zu calloc_size_le1024=%zu "
          "calloc_size_1025_4096=%zu calloc_size_4097_16384=%zu "
          "calloc_size_gt16384=%zu "
          "realloc_request_zero=%zu realloc_request_le1024=%zu "
          "realloc_request_1025_4096=%zu "
          "realloc_request_4097_16384=%zu "
          "realloc_request_gt16384=%zu "
          "realloc_owned_old_zero=%zu "
          "realloc_owned_old_le1024=%zu "
          "realloc_owned_old_1025_4096=%zu "
          "realloc_owned_old_4097_16384=%zu "
          "realloc_owned_old_gt16384=%zu "
          "realloc_copy_calls=%zu "
          "realloc_copy_same_class=%zu "
          "realloc_copy_cross_class=%zu "
          "realloc_copy_boundary_toy_to_midpage=%zu "
          "realloc_copy_boundary_mid8_to_mid32=%zu "
          "realloc_copy_boundary_midpage_to_large=%zu\n",
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
              &g_hz6_preload_phase_stats.malloc_toy_direct_class_eligible),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats
                   .malloc_toy_direct_class_eligible_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats
                   .malloc_toy_direct_class_eligible_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.malloc_toy_direct_class_enter),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats
                   .malloc_toy_direct_class_enter_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats
                   .malloc_toy_direct_class_enter_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_size_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_size_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_size_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_size_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.calloc_size_gt16384),
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
              &g_hz6_preload_phase_stats.realloc_copy_calls),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_copy_same_class),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_copy_cross_class),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats
                   .realloc_copy_boundary_toy_to_midpage),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.realloc_copy_boundary_mid8_to_mid32),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats
                   .realloc_copy_boundary_midpage_to_large));

  fprintf(stderr,
          "[HZ6_PRELOAD_WRAPPER_SIZE_DETAIL] "
          "posix_memalign_size_zero=%zu "
          "posix_memalign_size_le1024=%zu "
          "posix_memalign_size_1025_4096=%zu "
          "posix_memalign_size_4097_16384=%zu "
          "posix_memalign_size_gt16384=%zu "
          "aligned_alloc_size_zero=%zu "
          "aligned_alloc_size_le1024=%zu "
          "aligned_alloc_size_1025_4096=%zu "
          "aligned_alloc_size_4097_16384=%zu "
          "aligned_alloc_size_gt16384=%zu\n",
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_size_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_size_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_size_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_size_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.posix_memalign_size_gt16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_size_zero),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_size_le1024),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_size_1025_4096),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_size_4097_16384),
          hz6_preload_phase_load(
              &g_hz6_preload_phase_stats.aligned_alloc_size_gt16384));

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
              "route_register_fail=%zu source_block_exhausted=%zu "
              "toy2_construct_abort_attempt=%zu "
              "toy2_construct_abort_success=%zu "
              "toy2_construct_abort_ref_nonzero=%zu "
              "toy2_construct_abort_live_descriptor=%zu "
              "toy2_construct_abort_live_route=%zu "
              "toy2_construct_abort_pending_reference=%zu "
              "toy2_construct_abort_double_release=%zu "
              "toy2_construct_abort_release_fail=%zu "
              "toy2_unretained_active_block_after_abort=%zu\n",
              i, (void*)allocator, stats.route_valid, stats.route_invalid,
              stats.route_miss, stats.transfer_push, stats.transfer_pop,
              stats.source_alloc, stats.toy_source_alloc,
              stats.midpage_source_alloc, stats.large_source_alloc,
              stats.local2p_source_alloc, stats.alloc_fail,
              stats.descriptor_exhausted, stats.route_register_fail,
              stats.source_block_exhausted,
              stats.toy2_construct_abort_attempt,
              stats.toy2_construct_abort_success,
              stats.toy2_construct_abort_ref_nonzero,
              stats.toy2_construct_abort_live_descriptor,
              stats.toy2_construct_abort_live_route,
              stats.toy2_construct_abort_pending_reference,
              stats.toy2_construct_abort_double_release,
              stats.toy2_construct_abort_release_fail,
              stats.toy2_unretained_active_block_after_abort);
    }
    pthread_mutex_unlock(&g_hz6_preload_allocator_registry_mutex);
  }

  g_hz6_preload_reentry = saved_reentry;
