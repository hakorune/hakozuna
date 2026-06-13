#if HZ5_LOWPAGE64_STATS
static void hz5_lowpage64_print_once(void) {
  static atomic_flag printed = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set_explicit(&printed, memory_order_acq_rel)) {
    return;
  }

  Hz5Lowpage64PrintContext print_ctx;
  hz5_lowpage64_collect_print_context(&print_ctx);
  Hz5Lowpage64P43StatsSnapshot p43_stats = print_ctx.p43_stats;
  Hz5Lowpage64P44Current p44_current = print_ctx.p44_current;
  Hz5Lowpage64P43oFreeBuckets p43o_current =
      print_ctx.p43o_current;
  size_t bridge_cold_current = print_ctx.bridge_cold_current;

  fprintf(stderr,
          "[HZ5_LOWPAGE64] alloc_calls=%zu span_hits=%zu "
          "span_pushes=%zu span_full=%zu stash_hits=%zu "
          "global_batch_hits=%zu os_allocs=%zu free_calls=%zu "
          "relbuf_pushes=%zu batch_publishes=%zu batch_publish_nodes=%zu "
          "global_overflow_frees=%zu global_batch_count=%zu "
          "relbuf_count_max=%zu global_count_before_max=%zu "
          "global_count_after_max=%zu acquired_count_total=%zu "
          "acquired_count_max=%zu stash_set_calls=%zu "
          "stash_set_nodes_total=%zu stash_set_nodes_max=%zu "
          "trim_calls=%zu trim_freed_nodes=%zu "
          "adaptive_trigger_calls=%zu acquired_le64=%zu "
          "acquired_le128=%zu acquired_le192=%zu acquired_gt192=%zu "
          "stash_nodes_le64=%zu stash_nodes_le128=%zu "
          "stash_nodes_le192=%zu stash_nodes_gt192=%zu "
          "p37_overflow_hits=%zu p37_overflow_set_nodes_total=%zu "
          "p37_overflow_set_nodes_max=%zu "
          "p37_overflow_release_calls=%zu "
          "p37_overflow_release_nodes=%zu "
          "p37_high_event_releases=%zu p37_lifetime_releases=%zu "
          "p38_hot_hits=%zu p38_global_first_hits=%zu "
          "p38_set_hot_nodes_total=%zu p38_set_hot_nodes_max=%zu "
          "p38_global_cap_release_calls=%zu "
          "p38_global_cap_release_nodes=%zu "
          "p38_stash_trim_release_calls=%zu "
          "p38_stash_trim_release_nodes=%zu "
          "p38_p37_release_calls=%zu p38_p37_release_nodes=%zu "
          "p39_probation_hits=%zu "
          "p39_probation_set_nodes_total=%zu "
          "p39_probation_set_nodes_max=%zu "
          "p39_probation_release_calls=%zu "
          "p39_probation_release_nodes=%zu "
          "p39_probation_lifetime_releases=%zu "
          "p39_probation_replace_releases=%zu "
          "p40_checkpoint_calls=%zu p40_release_calls=%zu "
          "p40_release_nodes=%zu p40_release_age_nodes=%zu "
          "p40_release_hard_nodes=%zu p40_keep_nodes=%zu "
          "p40_p43_free_before_total=%zu p40_p43_free_before_max=%zu "
          "p40_p43_committed_before_total=%zu "
          "p40_p43_committed_before_max=%zu "
          "p40_skip_p43_free_calls=%zu p40_skip_p43_free_nodes=%zu "
          "p42_va_allocs=%zu p42_va_alloc_failures=%zu "
          "p42_va_releases=%zu p42_decommit_calls=%zu "
          "p42_decommit_failures=%zu p42_recommit_calls=%zu "
          "p42_recommit_failures=%zu p42_cold_hits=%zu "
          "p42_cold_count=%zu p42_cold_count_max=%zu "
          "p43_segments_reserved=%zu p43_segments_released=%zu "
          "p43_slot_commits=%zu p43_slot_commit_failures=%zu "
          "p43_slot_decommits=%zu p43_slot_decommit_failures=%zu "
          "p43_tls_hits=%zu p43_tls_pushes=%zu "
          "p43_tls_overflow_pushes=%zu p43_global_hits=%zu "
          "p43_global_pushes=%zu p43_cold_hits=%zu "
          "p43_free_slot_hits=%zu p43_source_alloc_calls=%zu "
          "p43_source_cold_segment_scans_total=%zu "
          "p43_source_cold_segment_scans_max=%zu "
          "p43_source_free_segment_scans_total=%zu "
          "p43_source_free_segment_scans_max=%zu "
          "p43_source_new_segment_scan_total=%zu "
          "p43_source_new_segment_scan_max=%zu p43_lookup_calls=%zu "
          "p43_lookup_active=%zu p43_lookup_nonactive=%zu "
          "p43_lookup_miss=%zu "
          "p43_lookup_segments_scanned_total=%zu "
          "p43_lookup_segments_scanned_max=%zu "
          "p43_lookup_fast_hits=%zu p43_lookup_fast_misses=%zu "
          "p43_lookup_lockless_hits=%zu "
          "p43_lookup_lockless_misses=%zu "
          "p43_find_fast_hits=%zu "
          "p43_find_segments_scanned_total=%zu "
          "p43_find_segments_scanned_max=%zu "
          "p43g_prepare_calls=%zu p43g_prepare_active=%zu "
          "p43g_prepare_nonactive=%zu p43g_prepare_miss=%zu "
          "p43g_source_p25=%zu p43g_source_other=%zu "
          "p43g_raw_mismatch=%zu "
          "p43g_release_old_path_calls=%zu "
          "p43g_release_prepared_calls=%zu "
          "p44_checkpoint_calls=%zu p44_candidate_total=%zu "
          "p44_candidate_max=%zu p44_global_observed_max=%zu "
          "p44_global_candidate_total=%zu "
          "p44_stash_observed_max=%zu "
          "p44_stash_candidate_total=%zu "
          "p44_relbuf_observed_max=%zu "
          "p44_relbuf_candidate_total=%zu "
          "p44_p43_free_observed_max=%zu "
          "p44_p43_free_candidate_total=%zu "
          "p44_reason_publish=%zu p44_reason_acquire_miss=%zu "
          "p44_reason_p40=%zu p44_reason_snapshot=%zu "
          "p44_candidate_current=%zu "
          "p44_global_candidate_current=%zu "
          "p44_stash_candidate_current=%zu "
          "p44_relbuf_candidate_current=%zu "
          "p44_p43_free_candidate_current=%zu "
          "p43o_epoch=%zu p43o_state=%zu "
          "p43o_cooldown=%zu p43o_state_open=%zu "
          "p43o_state_drain=%zu p43o_state_closed=%zu "
          "p43o_admission_flips=%zu "
          "p43o_reason_acquire_miss=%zu p43o_reason_p40=%zu "
          "p43o_reason_snapshot=%zu p43o_p43_free_max=%zu "
          "p43o_p43_free_hot_current=%zu "
          "p43o_p43_free_warm_current=%zu "
          "p43o_p43_free_cold_current=%zu "
          "p43o_p43_free_hot_total=%zu "
          "p43o_p43_free_warm_total=%zu "
          "p43o_p43_free_cold_total=%zu "
          "p43o_p40_release_candidates=%zu "
          "p43o_p40_would_stage1=%zu "
          "p43o_p40_would_demote_to_source=%zu "
          "p43o_p40_would_keep_bridge=%zu "
          "p43o_source_miss=%zu "
          "p43o_projected_free_max=%zu "
          "p43o_projected_p40_candidates=%zu "
          "p43o_projected_would_stage1=%zu "
          "p43o_projected_would_demote_to_source=%zu "
          "p43o_projected_would_keep_bridge=%zu "
          "p43o_projected_blocks=%zu "
          "p43p_epoch=%zu p43p_state_open=%zu "
          "p43p_state_drain=%zu p43p_state_closed=%zu "
          "p43p_admission_flips=%zu "
          "p43p_reason_p40=%zu p43p_reason_snapshot=%zu "
          "p43p_p40_release_candidates=%zu "
          "p43p_would_stage1_bridge_cold=%zu "
          "p43p_would_demote_to_source=%zu "
          "p43p_would_keep_bridge=%zu "
          "p43p_bridge_cold_projected_current=%zu "
          "p43p_bridge_cold_projected_max=%zu "
          "p43p_p43_committed_free_current=%zu "
          "p43p_p43_committed_free_max=%zu "
          "p43p_stage1_current=%zu "
          "p43p_stage1_enqueue_calls=%zu "
          "p43p_stage1_enqueue_nodes=%zu "
          "p43p_stage1_acquire_hits=%zu "
          "p43p_stage1_acquire_nodes=%zu "
          "p43p_stage1_requeue_calls=%zu "
          "p43p_stage1_requeue_nodes=%zu "
          "p43p_stage1_current_max=%zu "
          "p43p_age_stage1_items_total=%zu "
          "p43p_age_young_current=%zu "
          "p43p_age_old_current=%zu "
          "p43p_age_would_reuse_young=%zu "
          "p43p_age_would_keep_old=%zu "
          "p43p_age_would_demote_old_open=%zu "
          "p43p_age_would_block_old_drain=%zu "
          "p43p_age_would_block_old_closed=%zu "
          "p43p_age_hot_reinject_avoided=%zu "
          "p43p_age_pop_all_pollution_projection=%zu "
          "p43p_age_admission_flips=%zu\n",
          atomic_load_explicit(&g_hz5_lowpage64_alloc_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_pushes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_full,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_os_allocs,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_relbuf_pushes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_batch_publishes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_batch_publish_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_overflow_frees,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_relbuf_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_count_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_count_after_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_trim_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_trim_freed_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_adaptive_trigger_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le64,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le128,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_gt192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le64,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le128,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_gt192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_hits,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p37_overflow_set_nodes_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_high_event_releases,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_lifetime_releases,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_hot_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_first_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_set_hot_nodes_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_set_hot_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_cap_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_cap_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_stash_trim_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_stash_trim_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_p37_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_p37_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_hits,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_set_nodes_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_lifetime_releases,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_replace_releases,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_age_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_hard_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_keep_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_p43_free_before_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_p43_free_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p40_p43_committed_before_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p40_p43_committed_before_max,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_allocs,
                               memory_order_relaxed) +
              p43_stats.va_allocs,
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_alloc_failures,
                               memory_order_relaxed) +
              p43_stats.va_alloc_failures,
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_releases,
                               memory_order_relaxed) +
              p43_stats.va_releases,
          atomic_load_explicit(&g_hz5_lowpage64_p42_decommit_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_decommit_failures,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_recommit_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_recommit_failures,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_cold_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_cold_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_cold_count_max,
                               memory_order_relaxed),
          p43_stats.segments_reserved,
          p43_stats.segments_released,
          p43_stats.slot_commits,
          p43_stats.slot_commit_failures,
          p43_stats.slot_decommits,
          p43_stats.slot_decommit_failures,
          p43_stats.tls_hits,
          p43_stats.tls_pushes,
          p43_stats.tls_overflow_pushes,
          p43_stats.global_hits,
          p43_stats.global_pushes,
          p43_stats.cold_hits,
          p43_stats.free_slot_hits,
          p43_stats.source_alloc_calls,
          p43_stats.source_cold_segment_scans_total,
          p43_stats.source_cold_segment_scans_max,
          p43_stats.source_free_segment_scans_total,
          p43_stats.source_free_segment_scans_max,
          p43_stats.source_new_segment_scan_total,
          p43_stats.source_new_segment_scan_max,
          p43_stats.lookup_calls,
          p43_stats.lookup_active,
          p43_stats.lookup_nonactive,
          p43_stats.lookup_miss,
          p43_stats.lookup_segments_scanned_total,
          p43_stats.lookup_segments_scanned_max,
          p43_stats.lookup_fast_hits,
          p43_stats.lookup_fast_misses,
          p43_stats.lookup_lockless_hits,
          p43_stats.lookup_lockless_misses,
          p43_stats.find_fast_hits,
          p43_stats.find_segments_scanned_total,
          p43_stats.find_segments_scanned_max,
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_active,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_nonactive,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_source_p25,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_source_other,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_raw_mismatch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_release_old_path_calls,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43g_release_prepared_calls,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_global_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_global_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_stash_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_stash_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_relbuf_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_relbuf_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_p43_free_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_p43_free_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_publish,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_snapshot,
                               memory_order_relaxed),
          p44_current.total,
          p44_current.global,
          p44_current.stash,
          p44_current.relbuf,
          p44_current.p43_free,
          atomic_load_explicit(&g_hz5_lowpage64_p43o_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_runtime_state,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_runtime_cooldown,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_max,
                               memory_order_relaxed),
          p43o_current.hot,
          p43o_current.warm,
          p43o_current.cold,
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_hot_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_warm_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_cold_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_release_candidates,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_would_stage1,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_p40_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_would_keep_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_source_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_projected_free_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_p40_candidates,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_stage1,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_keep_bridge,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_projected_blocks,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p40_release_candidates,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_would_stage1_bridge_cold,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_would_keep_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_bridge_cold_projected_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p43_committed_free_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p43_committed_free_max,
              memory_order_relaxed),
          bridge_cold_current,
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_enqueue_calls,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_enqueue_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_acquire_hits,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_acquire_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_requeue_calls,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_requeue_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_current_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_stage1_items_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_young_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_old_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_reuse_young,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_keep_old,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_demote_old_open,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_block_old_drain,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_block_old_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_hot_reinject_avoided,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_pop_all_pollution_projection,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_admission_flips,
              memory_order_relaxed));
}
#endif

void hz5_lowpage64_register_stats_once(void) {
#if HZ5_LOWPAGE64_STATS
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_lowpage64_print_once);
  }
#endif
}
