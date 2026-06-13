#if HZ5_LOWPAGE64_STATS
typedef struct Hz5Lowpage64P44Current {
  size_t global;
  size_t stash;
  size_t relbuf;
  size_t p43_free;
  size_t total;
} Hz5Lowpage64P44Current;

typedef struct Hz5Lowpage64P43oFreeBuckets {
  size_t hot;
  size_t warm;
  size_t cold;
} Hz5Lowpage64P43oFreeBuckets;

typedef struct Hz5Lowpage64PrintContext {
  Hz5Lowpage64P43StatsSnapshot p43_stats;
  Hz5Lowpage64P44Current p44_current;
  Hz5Lowpage64P43oFreeBuckets p43o_current;
  size_t bridge_cold_current;
} Hz5Lowpage64PrintContext;

static Hz5Lowpage64P44Current hz5_lowpage64_p44_current_candidates(
    const Hz5Lowpage64P43StatsSnapshot* p43_stats) {
  Hz5Lowpage64P44Current current = {0};
  current.global = hz5_lowpage64_p44_candidate_value(
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_relaxed),
      HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP);
  current.stash = hz5_lowpage64_p44_candidate_value(
      g_hz5_lowpage64_stash_count, HZ5_LOWPAGE64_P44_STASH_SOFT_CAP);
  current.relbuf = hz5_lowpage64_p44_candidate_value(
      g_hz5_lowpage64_relbuf_count, HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP);
  current.p43_free = hz5_lowpage64_p44_candidate_value(
      p43_stats->slots_committed_free_current,
      HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP);
  current.total =
      current.global + current.stash + current.relbuf + current.p43_free;
  return current;
}

static Hz5Lowpage64P43oFreeBuckets hz5_lowpage64_p43o_current_buckets(
    const Hz5Lowpage64P43StatsSnapshot* p43_stats) {
  Hz5Lowpage64P43oFreeBuckets buckets = {0};
#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
  hz5_lowpage64_p43o_bucket_free(p43_stats->slots_committed_free_current,
                                 &buckets.hot, &buckets.warm,
                                 &buckets.cold);
#else
  (void)p43_stats;
#endif
  return buckets;
}

static size_t hz5_lowpage64_bridge_cold_current_for_stats(void) {
#if HZ5_LOWPAGE64_STAGE1_ENABLED
  return atomic_load_explicit(&g_hz5_lowpage64_p43p_stage1_count,
                              memory_order_relaxed);
#else
  return 0;
#endif
}

static void hz5_lowpage64_collect_print_context(
    Hz5Lowpage64PrintContext* ctx) {
  *ctx = (Hz5Lowpage64PrintContext){0};
  hz5_lowpage64_p43_stats_snapshot(&ctx->p43_stats);
  ctx->p44_current =
      hz5_lowpage64_p44_current_candidates(&ctx->p43_stats);
  ctx->p43o_current =
      hz5_lowpage64_p43o_current_buckets(&ctx->p43_stats);
  ctx->bridge_cold_current =
      hz5_lowpage64_bridge_cold_current_for_stats();
}

void hz5_lowpage64_print_snapshot(const char* label) {
  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_SNAPSHOT);

  Hz5Lowpage64PrintContext print_ctx;
  hz5_lowpage64_collect_print_context(&print_ctx);
  Hz5Lowpage64P43StatsSnapshot p43_stats = print_ctx.p43_stats;
  Hz5Lowpage64P44Current p44_current = print_ctx.p44_current;
  Hz5Lowpage64P43oFreeBuckets p43o_current =
      print_ctx.p43o_current;
  size_t bridge_cold_current = print_ctx.bridge_cold_current;
  hz5_lowpage64_p43o_note(
      HZ5_LOWPAGE64_P43O_REASON_SNAPSHOT,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      &p43_stats);
  hz5_lowpage64_p43p_note(
      HZ5_LOWPAGE64_P43P_REASON_SNAPSHOT,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      &p43_stats);
  hz5_lowpage64_p45_note(
      HZ5_LOWPAGE64_P45_REASON_SNAPSHOT,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      &p43_stats);
  hz5_lowpage64_p45dr_note(HZ5_LOWPAGE64_P45_REASON_SNAPSHOT);

  fprintf(stderr,
          "[HZ5_LOWPAGE64_SNAPSHOT] label=%s "
          "tls_stash_count=%zu relbuf_count=%zu "
          "global_batch_count=%zu acquired_count_max=%zu "
          "stash_set_nodes_max=%zu p43_segments_current=%zu "
          "p43_slots_total_current=%zu p43_slots_active_current=%zu "
          "p43_slots_committed_current=%zu "
          "p43_slots_committed_free_current=%zu "
          "p43_slots_global_retained_current=%zu "
          "p43_slots_tls_retained_current=%zu "
          "p43_slots_cold_current=%zu "
          "p43_slots_uncommitted_current=%zu "
          "p43_contract_lockless=%u p43_lockless_lookup=%u "
          "p43_slot_decommit=%u "
          "p43_page_noaccess=%u p43_runtime_segment_release=%u "
          "p43g_prepare_calls=%zu p43g_prepare_active=%zu "
          "p43g_prepare_nonactive=%zu p43g_prepare_miss=%zu "
          "p43g_source_p25=%zu p43g_source_other=%zu "
          "p43g_raw_mismatch=%zu "
          "p43g_release_old_path_calls=%zu "
          "p43g_release_prepared_calls=%zu "
          "p43_segments_reserved=%zu p43_segments_released=%zu "
          "p43_slot_commits=%zu p43_slot_decommits=%zu "
          "p43_tls_hits=%zu p43_global_hits=%zu "
          "p43_free_slot_hits=%zu p43_source_alloc_calls=%zu "
          "p43_source_cold_segment_scans_total=%zu "
          "p43_source_cold_segment_scans_max=%zu "
          "p43_source_free_segment_scans_total=%zu "
          "p43_source_free_segment_scans_max=%zu "
          "p43_source_new_segment_scan_total=%zu "
          "p43_source_new_segment_scan_max=%zu p43_lookup_calls=%zu "
          "p43_lookup_active=%zu p43_lookup_nonactive=%zu "
          "p43_lookup_miss=%zu p43_lookup_lockless_hits=%zu "
          "p43_lookup_lockless_misses=%zu "
          "p40_p43_free_before_max=%zu "
          "p40_skip_p43_free_calls=%zu "
          "p40_skip_p43_free_nodes=%zu "
          "p44_checkpoint_calls=%zu p44_candidate_total=%zu "
          "p44_candidate_max=%zu p44_global_candidate_total=%zu "
          "p44_stash_candidate_total=%zu "
          "p44_relbuf_candidate_total=%zu "
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
          "p43p_age_admission_flips=%zu "
          "p45_epoch=%zu p45_state=%zu p45_cooldown=%zu "
          "p45_state_open=%zu p45_state_drain=%zu "
          "p45_state_closed=%zu p45_admission_flips=%zu "
          "p45_reason_publish=%zu p45_reason_acquire_miss=%zu "
          "p45_reason_p40=%zu p45_reason_snapshot=%zu "
          "p45_bridge_current=%zu p45_bridge_current_max=%zu "
          "p45_bridge_pressure_total=%zu "
          "p45_source_free_current=%zu "
          "p45_source_free_current_max=%zu "
          "p45_source_free_pressure_total=%zu "
          "p45_source_committed_current=%zu "
          "p45_source_committed_current_max=%zu "
          "p45_source_committed_pressure_total=%zu "
          "p45_p40_demote_intent=%zu "
          "p45_p40_demote_while_drain_closed=%zu "
          "p45_p40_demote_while_bridge_nonempty=%zu "
          "p45_would_demote_clean_open=%zu "
          "p45_would_stage1_any=%zu "
          "p45_would_stage1_state=%zu "
          "p45_would_stage1_bridge_nonempty=%zu "
          "p45_would_stage1_overlap=%zu "
          "p45_bridge_residual_after_demote_max=%zu "
          "p45_bridge_residual_after_demote_total=%zu "
          "p45_would_demote_open_bridge_soft=%zu "
          "p45_would_stage1_bridge_excess=%zu "
          "p45_would_stage1_refined_any=%zu "
          "p45_source_miss=%zu "
          "p45rg_demote_intent=%zu "
          "p45rg_allowed_open_bridge_soft=%zu "
          "p45rg_stage1_any=%zu "
          "p45rg_stage1_state=%zu "
          "p45rg_stage1_bridge_excess=%zu "
          "p45rg_stage1_overlap=%zu "
          "p45rg_stage1_current=%zu "
          "p45rg_stage1_enqueue_calls=%zu "
          "p45rg_stage1_enqueue_nodes=%zu "
          "p45rg_stage1_acquire_hits=%zu "
          "p45rg_stage1_acquire_nodes=%zu "
          "p45rg_stage1_requeue_calls=%zu "
          "p45rg_stage1_requeue_nodes=%zu "
          "p45rg_stage1_current_max=%zu "
          "p45rg_bridge_residual_max=%zu "
          "p45rg_bridge_residual_total=%zu "
          "p45dr_checkpoint_calls=%zu "
          "p45dr_reason_p40=%zu "
          "p45dr_reason_acquire_miss=%zu "
          "p45dr_reason_snapshot=%zu "
          "p45dr_stage1_current_at_checkpoint=%zu "
          "p45dr_stage1_current_at_checkpoint_max=%zu "
          "p45dr_stage1_current_at_acquire_miss=%zu "
          "p45dr_stage1_current_at_acquire_miss_max=%zu "
          "p45dr_stage1_current_at_snapshot=%zu "
          "p45dr_stage1_current_at_snapshot_max=%zu "
          "p45dr_stage1_age_bucket0=%zu "
          "p45dr_stage1_age_bucket1=%zu "
          "p45dr_stage1_age_bucket2_4=%zu "
          "p45dr_stage1_age_old=%zu "
          "p45dr_stage1_old_current_max=%zu "
          "p45dr_stage1_oldest_age_max=%zu "
          "p45dr_would_drain_to_bridge=%zu "
          "p45dr_would_keep_cold=%zu "
          "p45dr_would_demote_open=%zu "
          "p45dr_would_block_drain_closed=%zu "
          "p45dr_would_block_bridge_excess=%zu "
          "p45dr_bridge_current_hot_max=%zu "
          "p45dr_bridge_cold_current_max=%zu "
          "p45dr_bridge_total_with_cold_max=%zu "
          "p45dr_balance_expected=%zu "
          "p45dr_balance_actual=%zu "
          "p45dr_balance_mismatch=%zu "
          "p45dr_balance_mismatch_max=%zu\n",
          label ? label : "",
          g_hz5_lowpage64_stash_count,
          g_hz5_lowpage64_relbuf_count,
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_max,
                               memory_order_relaxed),
          p43_stats.segments_current,
          p43_stats.slots_total_current,
          p43_stats.slots_active_current,
          p43_stats.slots_committed_current,
          p43_stats.slots_committed_free_current,
          p43_stats.slots_global_retained_current,
          p43_stats.slots_tls_retained_current,
          p43_stats.slots_cold_current,
          p43_stats.slots_uncommitted_current,
          p43_stats.contract_lockless_enabled,
          p43_stats.lockless_lookup_enabled,
          p43_stats.slot_decommit_enabled,
          p43_stats.page_noaccess_enabled,
          p43_stats.runtime_segment_release_enabled,
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
          p43_stats.segments_reserved,
          p43_stats.segments_released,
          p43_stats.slot_commits,
          p43_stats.slot_decommits,
          p43_stats.tls_hits,
          p43_stats.global_hits,
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
          p43_stats.lookup_lockless_hits,
          p43_stats.lookup_lockless_misses,
          atomic_load_explicit(&g_hz5_lowpage64_p40_p43_free_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_global_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_stash_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_relbuf_candidate_total,
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
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_runtime_state,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_runtime_cooldown,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_publish,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed) +
              g_hz5_lowpage64_stash_count + g_hz5_lowpage64_relbuf_count,
          atomic_load_explicit(&g_hz5_lowpage64_p45_bridge_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_bridge_pressure_total,
                               memory_order_relaxed),
          p43_stats.slots_committed_free_current,
          atomic_load_explicit(&g_hz5_lowpage64_p45_source_free_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_source_free_pressure_total,
              memory_order_relaxed),
          p43_stats.slots_committed_current,
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_source_committed_current_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_source_committed_pressure_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_p40_demote_intent,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_p40_demote_while_drain_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_p40_demote_while_bridge_nonempty,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_demote_clean_open,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_any,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_state,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_stage1_bridge_nonempty,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_overlap,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_bridge_residual_after_demote_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_bridge_residual_after_demote_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_demote_open_bridge_soft,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_stage1_bridge_excess,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_refined_any,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_source_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_demote_intent,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45rg_allowed_open_bridge_soft,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_any,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_state,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45rg_stage1_bridge_excess,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_overlap,
                               memory_order_relaxed),
          bridge_cold_current,
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_enqueue_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_enqueue_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_acquire_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_acquire_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_requeue_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_requeue_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_bridge_residual_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_bridge_residual_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_snapshot,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_snapshot_max,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_bucket0,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_bucket1,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_bucket2_4,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_old,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_old_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_oldest_age_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_would_drain_to_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_would_keep_cold,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_would_demote_open,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_would_block_drain_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_would_block_bridge_excess,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_bridge_current_hot_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_bridge_cold_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_bridge_total_with_cold_max,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_expected,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_actual,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_mismatch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_mismatch_max,
                               memory_order_relaxed));
}
#else
void hz5_lowpage64_print_snapshot(const char* label) {
  (void)label;
}
#endif

