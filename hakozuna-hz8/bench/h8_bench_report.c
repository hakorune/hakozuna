#include "h8_bench_support.h"

#include "../include/h8.h"

#include <inttypes.h>
#include <stdio.h>

void h8_bench_print_final_report(const H8BenchReportInput* input) {
  const H8BenchOptions* opt = input->opt;
#if defined(H8_BENCH_ATTRIBUTION)
  double frag_ratio = input->frag_requested_total
                          ? (double)input->frag_rounded_total /
                                (double)input->frag_requested_total
                          : 0.0;
  printf("fragmentation requested_bytes=%" PRIu64 " rounded_bytes=%" PRIu64 " rounding_ratio=%.6f\n",
         input->frag_requested_total, input->frag_rounded_total, frag_ratio);
  printf("fragmentation_candidates upper1536_rounded_bytes=%" PRIu64 " upper1536_ratio=%.6f upper1p5_rounded_bytes=%" PRIu64 " upper1p5_ratio=%.6f\n",
         input->frag_upper1536_total,
         input->frag_requested_total ? (double)input->frag_upper1536_total /
                                           (double)input->frag_requested_total
                                     : 0.0,
         input->frag_upper1p5_total,
         input->frag_requested_total ? (double)input->frag_upper1p5_total /
                                           (double)input->frag_requested_total
                                     : 0.0);
  printf("fragmentation_by_class class_count=%u allocs=[", H8_CLASS_COUNT);
  for (uint32_t c = 0; c < H8_CLASS_COUNT; ++c) {
    printf("%s%zu", c == 0 ? "" : ",", input->frag_allocs_by_class[c]);
  }
  printf("] rounded_bytes=[");
  for (uint32_t c = 0; c < H8_CLASS_COUNT; ++c) {
    printf("%s%" PRIu64, c == 0 ? "" : ",", input->frag_rounded_by_class[c]);
  }
  printf("]\n");
  h8_bench_print_medium_totals(input->medium_totals);
#else
  printf("fragmentation attribution=disabled\n");
#endif

  H8Stats stats = h8_stats();
  H8DebugStats debug = h8_debug_stats();
  printf("stats_snapshot owner_exit=%zu pending_enqueue=%zu pending_dequeue=%zu orphan_handoff=%zu handoff_ok=%zu local=%zu remote=%zu\n",
         stats.owner_exit_count, stats.pending_enqueue_count,
         stats.pending_dequeue_count, stats.orphan_handoff_count,
         stats.handoff_success_count, stats.local_alloc_count,
         stats.remote_collect_count);
  printf("counters_dbg publish_enter=%zu publish_exit=%zu lifecycle_enter=%zu lifecycle_exit=%zu span_publish_enter=%zu span_publish_exit=%zu remote_regular=%zu remote_orphan=%zu pending_notify=%zu pending_calls=%zu pending_carry_hit=%zu pending_requeue=%zu pending_word_set=%zu pending_word_shadow_hit=%zu pending_word_false_pos=%zu pending_word_false_neg=%zu pending_word_rearm=%zu pending_words=%zu pending_words_nonzero=%zu pending_bits=%zu orphan_quiesce=%zu orphan_ready=%zu dry_scan=%zu dry_candidate=%zu dry_block_state=%zu dry_block_quiesce=%zu dry_empty=%zu dry_target_closed=%zu dry_would_adopt=%zu handoff_fail=%zu invalid=%zu miss=%zu owner_transition=%zu adopt_scan=%zu adopt_candidate=%zu adopt_block_state=%zu adopt_block_quiesce=%zu adopt_empty=%zu adopt_target_closed=%zu adopt_ok=%zu\n",
         debug.owner_publish_enter_count, debug.owner_publish_exit_count,
         debug.owner_lifecycle_enter_count, debug.owner_lifecycle_exit_count,
         debug.span_publish_enter_count, debug.span_publish_exit_count,
         debug.remote_regular_admission_count, debug.remote_orphan_admission_count,
         debug.pending_notify_count, debug.pending_collect_call_count,
         debug.pending_collect_carry_hit_count, debug.pending_collect_requeue_count,
         debug.pending_word_summary_set, debug.pending_word_summary_shadow_hit,
         debug.pending_word_summary_false_positive,
         debug.pending_word_summary_false_negative, debug.pending_word_summary_rearm,
         debug.pending_collect_word_count, debug.pending_collect_word_nonzero_count,
         debug.pending_collect_bit_count, debug.orphan_quiesce_count,
         debug.orphan_ready_count, debug.adoption_dry_run_scan_count,
         debug.adoption_dry_run_candidate_count,
         debug.adoption_dry_run_block_state_count,
         debug.adoption_dry_run_block_quiesce_count,
         debug.adoption_dry_run_empty_count,
         debug.adoption_dry_run_target_closed_count,
         debug.adoption_dry_run_would_adopt_count, debug.handoff_fail_count,
         debug.invalid_count, debug.miss_count, debug.owner_transition_count,
         debug.adoption_scan_count, debug.adoption_candidate_count,
         debug.adoption_block_state_count, debug.adoption_block_quiesce_count,
         debug.adoption_empty_count, debug.adoption_target_closed_count,
         debug.adoption_success_count);
  printf("remote_stage enter=%zu span_miss=%zu owner_missing=%zu regular_lease_ok=%zu regular_lease_fail=%zu regular_lease_elided=%zu orphan_lease_ok=%zu orphan_lease_fail=%zu pending_claim_ok=%zu validate_fail=%zu notify_first=%zu publish_ok=%zu pending_publish_elided=%zu\n",
         debug.remote_stage_enter, debug.remote_stage_span_miss,
         debug.remote_stage_owner_missing, debug.remote_stage_regular_lease_ok,
         debug.remote_stage_regular_lease_fail,
         debug.remote_stage_regular_lease_elided,
         debug.remote_stage_orphan_lease_ok, debug.remote_stage_orphan_lease_fail,
         debug.remote_stage_pending_claim_ok, debug.remote_stage_validate_fail,
         debug.remote_stage_notify_first, debug.remote_stage_publish_ok,
         debug.remote_stage_pending_publish_elided);
  printf("remote_lookup enter=%zu arena_miss=%zu span_miss=%zu retired=%zu slot_oob=%zu ok=%zu owner_word=%zu\n",
         debug.remote_lookup_enter, debug.remote_lookup_arena_miss,
         debug.remote_lookup_span_miss, debug.remote_lookup_retired,
         debug.remote_lookup_slot_oob, debug.remote_lookup_ok,
         debug.remote_owner_word_load);

  double slots_per_nonzero_word = 0.0;
  double singleton_ratio = 0.0;
  double multi_ratio = 0.0;
  if (debug.pending_word_drain_count != 0) {
    slots_per_nonzero_word =
        (double)debug.pending_slots_drained / (double)debug.pending_word_drain_count;
    singleton_ratio =
        (double)debug.pending_word_popcount_1 / (double)debug.pending_word_drain_count;
    multi_ratio = 1.0 - singleton_ratio;
  }
  printf("pending_word_density drain=%zu pop1=%zu pop2=%zu pop3_4=%zu pop5_8=%zu pop9_16=%zu pop17p=%zu slots=%zu rearmed=%zu new_publish=%zu slots_per_nonzero_word=%.3f singleton_ratio=%.3f multi_ratio=%.3f\n",
         debug.pending_word_drain_count, debug.pending_word_popcount_1,
         debug.pending_word_popcount_2, debug.pending_word_popcount_3_4,
         debug.pending_word_popcount_5_8, debug.pending_word_popcount_9_16,
         debug.pending_word_popcount_17_plus, debug.pending_slots_drained,
         debug.pending_words_rearmed, debug.pending_word_new_publish_during_drain,
         slots_per_nonzero_word, singleton_ratio, multi_ratio);
  printf("pending_count_shadow mask_notify_without_count=%zu count_notify_without_mask=%zu mask_requeue_without_count=%zu count_requeue_without_mask=%zu\n",
         debug.pending_mask_notify_without_count,
         debug.pending_count_notify_without_mask,
         debug.pending_mask_requeue_without_count,
         debug.pending_count_requeue_without_mask);
  printf("pending_finish_shadow count_mask0_bitmap0=%zu count_mask0_bitmap1=%zu mask1_bitmap0=%zu mask1_bitmap1=%zu\n",
         debug.pending_finish_count_mask_zero_bitmap_zero,
         debug.pending_finish_count_mask_zero_bitmap_nonzero,
         debug.pending_finish_mask_nonzero_bitmap_zero,
         debug.pending_finish_mask_nonzero_bitmap_nonzero);
  printf("qstate_dirty set=%zu self_set=%zu requeue=%zu\n",
         debug.qstate_dirty_set, debug.qstate_dirty_self_set,
         debug.qstate_dirty_requeue);
  printf("quiescent_pending bitmap_nonzero=%zu repair=%zu\n",
         debug.quiescent_pending_bitmap_nonzero, debug.quiescent_pending_repair);
  printf("queue_contention qstate_attempt=%zu qstate_success=%zu qstate_skip=%zu pending_push_attempt=%zu pending_push_retry=%zu pending_push_success=%zu owner_enter_retry=%zu owner_exit_retry=%zu duplicate_claim=%zu\n",
         debug.qstate_notify_attempt_count, debug.qstate_notify_success_count,
         debug.qstate_notify_skip_nonidle_count,
         debug.pending_head_push_attempt_count,
         debug.pending_head_push_retry_count,
         debug.pending_head_push_success_count,
         debug.owner_lifecycle_enter_cas_retry_count,
         debug.owner_lifecycle_exit_cas_retry_count,
         debug.remote_publish_pending_claim_duplicate_count);
  printf("local_zero_gates alloc_pending=%zu free_pending=%zu live_set_already=%zu live_clear_free=%zu\n",
         debug.local_alloc_pending_nonzero, debug.local_free_pending_nonzero,
         debug.owner_live_set_already_live,
         debug.owner_live_clear_already_free);
  printf("local_path active_hit=%zu active_miss=%zu freelist=%zu bump=%zu slow_collect=%zu span_commit=%zu find_scan=%zu scan_span=%zu free_hit=%zu reject_owner=%zu reject_state=%zu reject_live=%zu\n",
         debug.local_active_hit, debug.local_active_miss,
         debug.local_freelist_pop, debug.local_bump_alloc,
         debug.local_slow_collect, debug.local_span_commit,
         debug.local_find_scan, debug.local_find_scan_span, debug.local_free_hit,
         debug.local_free_reject_owner, debug.local_free_reject_state,
         debug.local_free_reject_live);
  printf("medium_stats malloc=%zu create=%zu active_reuse=%zu owner_reuse=%zu global_reuse=%zu refill_reuse=%zu madvise=%zu owner_scan=%zu owner_steps=%zu global_scan=%zu global_steps=%zu free_lookup=%zu route_lookup=%zu invalid_owned=%zu empty=%zu retain=%zu budget_reject=%zu reactivate=%zu exit_drain=%zu collect_active_would_keep=%zu collect_ctx_missing=%zu collect_ctx_owner_mismatch=%zu collect_active_hint_mismatch=%zu collect_active_not_owned=%zu empty_live_not_active=%zu active_live_bytes=%zu active_live_peak=%zu exit_active_live=%zu madvise_fail=%zu resident_bytes=%zu resident_peak=%zu madvise_ms=%.3f global_lock_ms=%.3f run_lock_ms=%.3f alloc_slot_ms=%.3f free_slot_ms=%.3f alloc_slot=%zu free_slot=%zu alloc_live_nonempty=%zu alloc_live_live=%zu alloc_live_active_empty=%zu alloc_live_resident=%zu alloc_live_decommitted=%zu alloc_state_fail=%zu alloc_free_zero=%zu local_free_pending=%zu lock_elide_alloc=%zu lock_elide_free=%zu lock_elide_mismatch=%zu active_owner_mismatch=%zu owner_list_mismatch=%zu global_skip_foreign=%zu local_free_owner=%zu remote_free_owner=%zu free_steps=%zu route_steps=%zu route_authority_mismatch=%zu remote_pub=%zu remote_lease_ms=%.3f remote_lease_enter_ms=%.3f remote_lease_exit_ms=%.3f remote_run_lock_ms=%.3f remote_claim=%zu remote_claim_ms=%.3f remote_lockless_claim=%zu remote_lockless_accept=%zu remote_lockless_rb_invalid=%zu remote_lockless_rb_accept=%zu writer_overlap=%zu writer_foreign=%zu writer_token_change=%zu collect_wrong_owner=%zu detached_while_attached=%zu remote_shadow_attempt=%zu remote_shadow_accept=%zu remote_shadow_reject=%zu remote_shadow_match=%zu remote_shadow_mismatch=%zu remote_notify=%zu remote_qpush=%zu remote_qpush_ms=%.3f remote_collect_call=%zu remote_collect_run=%zu remote_collect_slot=%zu remote_collect_ms=%.3f collect_finish_pending_rearm=%zu empty_with_pending=%zu\n",
         debug.medium_malloc_count, debug.medium_run_create_count,
         debug.medium_run_reuse_active_count,
         debug.medium_run_reuse_owner_list_count,
         debug.medium_run_reuse_global_count,
         debug.medium_run_reuse_refill_candidate_count,
         debug.medium_run_madvise_count,
         debug.medium_owner_scan_count, debug.medium_owner_scan_step_count,
         debug.medium_global_scan_count, debug.medium_global_scan_step_count,
         debug.medium_free_lookup_count, debug.medium_route_lookup_count,
         debug.medium_invalid_owned_count, debug.medium_empty_transition_count,
         debug.medium_empty_retain_count, debug.medium_empty_budget_reject_count,
         debug.medium_empty_reactivate_count, debug.medium_owner_exit_drain_count,
         debug.medium_collect_active_would_keep, debug.medium_collect_ctx_missing,
         debug.medium_collect_ctx_owner_mismatch,
         debug.medium_collect_active_hint_mismatch,
         debug.medium_collect_active_not_owned,
         debug.medium_empty_live_not_current_active,
         debug.medium_active_live_empty_bytes,
         debug.medium_active_live_empty_peak,
         debug.medium_owner_exit_active_live_remaining,
         debug.medium_madvise_fail_count, debug.medium_resident_empty_bytes,
         debug.medium_resident_empty_peak, (double)debug.medium_madvise_ns / 1e6,
         (double)debug.medium_global_lock_wait_ns / 1e6,
         (double)debug.medium_run_lock_wait_ns / 1e6,
         (double)debug.medium_alloc_slot_ns / 1e6,
         (double)debug.medium_free_slot_ns / 1e6,
         debug.medium_alloc_slot_count, debug.medium_free_slot_count,
         debug.medium_alloc_mark_live_nonempty,
         debug.medium_alloc_mark_live_live,
         debug.medium_alloc_mark_live_active_empty,
         debug.medium_alloc_mark_live_resident,
         debug.medium_alloc_mark_live_decommitted,
         debug.medium_alloc_state_check_fail, debug.medium_alloc_free_mask_zero,
         debug.medium_local_free_pending_nonzero,
         debug.medium_lock_elide_alloc_candidate,
         debug.medium_lock_elide_free_candidate,
         debug.medium_lock_elide_owner_mismatch,
         debug.medium_active_alloc_owner_mismatch,
         debug.medium_owner_list_owner_mismatch,
         debug.medium_global_skip_foreign_attached,
         debug.medium_local_free_owner_match,
         debug.medium_remote_free_owner_mismatch,
         debug.medium_free_lookup_step_count, debug.medium_route_lookup_step_count,
         debug.medium_route_authority_mismatch, debug.medium_remote_publish_count,
         (double)debug.medium_remote_owner_lease_ns / 1e6,
         (double)debug.medium_remote_owner_lease_enter_ns / 1e6,
         (double)debug.medium_remote_owner_lease_exit_ns / 1e6,
         (double)debug.medium_remote_run_lock_ns / 1e6,
         debug.medium_remote_pending_claim_count,
         (double)debug.medium_remote_pending_claim_ns / 1e6,
         debug.medium_remote_lockless_claim_count,
         debug.medium_remote_lockless_claim_collector_accept,
         debug.medium_remote_lockless_claim_rollback_invalid,
         debug.medium_remote_lockless_claim_rollback_accepted,
         debug.medium_attached_writer_overlap,
         debug.medium_attached_foreign_mask_writer,
         debug.medium_owner_token_changed_during_mutation,
         debug.medium_collect_wrong_owner,
         debug.medium_detached_direct_free_while_attached,
         debug.medium_remote_lockless_shadow_attempt,
         debug.medium_remote_lockless_shadow_would_accept,
         debug.medium_remote_lockless_shadow_would_reject,
         debug.medium_remote_lockless_shadow_match,
         debug.medium_remote_lockless_shadow_mismatch,
         debug.medium_remote_notify_count, debug.medium_remote_queue_push_count,
         (double)debug.medium_remote_queue_push_ns / 1e6,
         debug.medium_remote_collect_call_count,
         debug.medium_remote_collect_run_count,
         debug.medium_remote_collect_slot_count,
         (double)debug.medium_remote_collect_ns / 1e6,
         debug.medium_collect_finish_pending_rearm,
         debug.medium_empty_with_pending);
  printf("medium_local_class malloc=[%zu,%zu,%zu,%zu] active=[%zu,%zu,%zu,%zu] owner=[%zu,%zu,%zu,%zu] global=[%zu,%zu,%zu,%zu] create=[%zu,%zu,%zu,%zu] local_free=[%zu,%zu,%zu,%zu] active_miss_null=%zu active_miss_owner=%zu active_miss_unusable=%zu\n",
         debug.medium_malloc_class_8k, debug.medium_malloc_class_16k,
         debug.medium_malloc_class_32k, debug.medium_malloc_class_64k,
         debug.medium_run_reuse_active_class_8k,
         debug.medium_run_reuse_active_class_16k,
         debug.medium_run_reuse_active_class_32k,
         debug.medium_run_reuse_active_class_64k,
         debug.medium_run_reuse_owner_class_8k,
         debug.medium_run_reuse_owner_class_16k,
         debug.medium_run_reuse_owner_class_32k,
         debug.medium_run_reuse_owner_class_64k,
         debug.medium_run_reuse_global_class_8k,
         debug.medium_run_reuse_global_class_16k,
         debug.medium_run_reuse_global_class_32k,
         debug.medium_run_reuse_global_class_64k,
         debug.medium_run_create_class_8k, debug.medium_run_create_class_16k,
         debug.medium_run_create_class_32k, debug.medium_run_create_class_64k,
         debug.medium_local_free_class_8k, debug.medium_local_free_class_16k,
         debug.medium_local_free_class_32k, debug.medium_local_free_class_64k,
         debug.medium_active_miss_null, debug.medium_active_miss_owner,
         debug.medium_active_miss_unusable);

  printf("medium_free_cache_shadow attempt=%zu range_hit=%zu owner_hit=%zu slot_hit=%zu would_succeed=%zu pending_block=%zu state_block=%zu directory_mismatch=%zu fallback=%zu\n",
         debug.medium_free_cache_attempt,
         debug.medium_free_cache_range_hit,
         debug.medium_free_cache_owner_hit,
         debug.medium_free_cache_slot_hit,
         debug.medium_free_cache_would_succeed,
         debug.medium_free_cache_pending_block,
         debug.medium_free_cache_state_block,
         debug.medium_free_cache_directory_mismatch,
         debug.medium_free_cache_fallback);

  printf("medium_warm_shadow warm1_install=%zu warm1_replace=%zu warm1_hit=%zu warm1_avoid_reject=%zu warm2_install=%zu warm2_replace=%zu warm2_hit=%zu warm2_avoid_reject=%zu reuse_distance=[%zu,%zu,%zu,%zu]\n",
         debug.medium_warm1_would_install,
         debug.medium_warm1_would_replace,
         debug.medium_warm1_reuse_hit,
         debug.medium_warm1_would_avoid_budget_reject,
         debug.medium_warm2_would_install,
         debug.medium_warm2_would_replace,
         debug.medium_warm2_reuse_hit,
         debug.medium_warm2_would_avoid_budget_reject,
         debug.medium_warm_reuse_distance_0,
         debug.medium_warm_reuse_distance_1,
         debug.medium_warm_reuse_distance_2,
         debug.medium_warm_reuse_distance_3p);

  printf("medium_retention_causal empty=%zu pre_distance=[%zu,%zu,%zu,%zu,%zu] decommit=[budget:%zu,cold:%zu,exit:%zu] ghost_reuse=[budget:%zu,cold:%zu,exit:%zu] ghost_distance=[%zu,%zu,%zu,%zu,%zu] ghost_epoch=[%zu,%zu,%zu,%zu] model_decommit=[%zu,%zu,%zu,%zu,%zu] model_refault=[%zu,%zu,%zu,%zu,%zu]\n",
         debug.medium_retention_empty_seen,
         debug.medium_retention_pre_distance_1,
         debug.medium_retention_pre_distance_2,
         debug.medium_retention_pre_distance_3,
         debug.medium_retention_pre_distance_4,
         debug.medium_retention_pre_distance_5p,
         debug.medium_retention_decommit_budget,
         debug.medium_retention_decommit_cold,
         debug.medium_retention_decommit_owner_exit,
         debug.medium_retention_ghost_reuse_budget,
         debug.medium_retention_ghost_reuse_cold,
         debug.medium_retention_ghost_reuse_owner_exit,
         debug.medium_retention_ghost_distance_1,
         debug.medium_retention_ghost_distance_2,
         debug.medium_retention_ghost_distance_3,
         debug.medium_retention_ghost_distance_4,
         debug.medium_retention_ghost_distance_5p,
         debug.medium_retention_ghost_epoch_0_1,
         debug.medium_retention_ghost_epoch_2_3,
         debug.medium_retention_ghost_epoch_4_7,
         debug.medium_retention_ghost_epoch_8p,
         debug.medium_retention_model_decommit_n0,
         debug.medium_retention_model_decommit_n1,
         debug.medium_retention_model_decommit_n2,
         debug.medium_retention_model_decommit_n3,
         debug.medium_retention_model_decommit_n4,
         debug.medium_retention_model_refault_n0,
         debug.medium_retention_model_refault_n1,
         debug.medium_retention_model_refault_n2,
         debug.medium_retention_model_refault_n3,
         debug.medium_retention_model_refault_n4);
  printf("medium_retention_l3 mismatch=%zu decommit=[%zu,%zu,%zu,%zu] refault=[%zu,%zu,%zu,%zu] bytes=[%zu,%zu,%zu,%zu] peak=[%zu,%zu,%zu,%zu]\n",
         debug.medium_retention_l3_m0_mismatch,
         debug.medium_retention_l3_m0_decommit,
         debug.medium_retention_l3_m1_decommit,
         debug.medium_retention_l3_m2_decommit,
         debug.medium_retention_l3_clock_decommit,
         debug.medium_retention_l3_m0_refault,
         debug.medium_retention_l3_m1_refault,
         debug.medium_retention_l3_m2_refault,
         debug.medium_retention_l3_clock_refault,
         debug.medium_retention_l3_m0_bytes,
         debug.medium_retention_l3_m1_bytes,
         debug.medium_retention_l3_m2_bytes,
         debug.medium_retention_l3_clock_bytes,
         debug.medium_retention_l3_m0_peak,
         debug.medium_retention_l3_m1_peak,
         debug.medium_retention_l3_m2_peak,
         debug.medium_retention_l3_clock_peak);
  printf("medium_lazy_purge_shadow candidate=%zu reuse=%zu bytes=%zu peak=%zu over16m=%zu over32m=%zu\n",
         debug.medium_lazy_purge_candidate, debug.medium_lazy_purge_reuse,
         debug.medium_lazy_purge_bytes, debug.medium_lazy_purge_peak,
         debug.medium_lazy_purge_over_16m,
         debug.medium_lazy_purge_over_32m);
  printf("medium_lazy_purge_cost acquire=%zu reuse=%zu keep_live=%zu empty_fast=%zu drop=%zu drop_detach=%zu drop_destroy=%zu normal_skip=%zu cap_reject=%zu cas_retry=%zu\n",
         debug.medium_lazy_charge_acquire, debug.medium_lazy_charge_reuse,
         debug.medium_lazy_charge_keep_live,
         debug.medium_lazy_charge_empty_fast, debug.medium_lazy_charge_drop,
         debug.medium_lazy_drop_detach, debug.medium_lazy_drop_destroy,
         debug.medium_lazy_normal_budget_skip, debug.medium_lazy_cap_reject,
         debug.medium_lazy_cas_retry);

  size_t medium_collect_seen =
      debug.medium_remote_collect_slot_count +
      debug.medium_remote_collect_reject_count;
  printf("medium_collect_detail accepted=%zu rejected=%zu reject_ratio=%.6f\n",
         debug.medium_remote_collect_slot_count,
         debug.medium_remote_collect_reject_count,
         medium_collect_seen
             ? (double)debug.medium_remote_collect_reject_count /
                   (double)medium_collect_seen
             : 0.0);
  printf("medium_collect_breakdown state_ms=%.3f pending_clear_ms=%.3f mask_ms=%.3f empty_ms=%.3f\n",
         (double)debug.medium_collect_state_ns / 1e6,
         (double)debug.medium_collect_pending_clear_ns / 1e6,
         (double)debug.medium_collect_mask_ns / 1e6,
         (double)debug.medium_collect_empty_ns / 1e6);
  printf("medium_remote_class pub=[%zu,%zu,%zu,%zu] qpush=[%zu,%zu,%zu,%zu] collect_run=[%zu,%zu,%zu,%zu] collect_slot=[%zu,%zu,%zu,%zu]\n",
         debug.medium_remote_publish_class_8k,
         debug.medium_remote_publish_class_16k,
         debug.medium_remote_publish_class_32k,
         debug.medium_remote_publish_class_64k,
         debug.medium_remote_qpush_class_8k, debug.medium_remote_qpush_class_16k,
         debug.medium_remote_qpush_class_32k,
         debug.medium_remote_qpush_class_64k,
         debug.medium_remote_collect_run_class_8k,
         debug.medium_remote_collect_run_class_16k,
         debug.medium_remote_collect_run_class_32k,
         debug.medium_remote_collect_run_class_64k,
         debug.medium_remote_collect_slot_class_8k,
         debug.medium_remote_collect_slot_class_16k,
         debug.medium_remote_collect_slot_class_32k,
         debug.medium_remote_collect_slot_class_64k);
  printf("medium_lease_shadow decision_mismatch=%zu ref_underflow=%zu refs_at_exit=%zu enter_after_close=%zu reuse_with_refs=%zu\n",
         debug.medium_lease_enter_decision_mismatch,
         debug.medium_lease_ref_underflow,
         debug.medium_lease_ref_nonzero_at_owner_exit,
         debug.medium_lease_enter_after_close,
         debug.medium_owner_reuse_with_medium_refs);

  double medium_slots_per_run_8k =
      debug.medium_remote_collect_run_class_8k
          ? (double)debug.medium_remote_collect_slot_class_8k /
                (double)debug.medium_remote_collect_run_class_8k
          : 0.0;
  double medium_slots_per_run_16k =
      debug.medium_remote_collect_run_class_16k
          ? (double)debug.medium_remote_collect_slot_class_16k /
                (double)debug.medium_remote_collect_run_class_16k
          : 0.0;
  double medium_slots_per_run_32k =
      debug.medium_remote_collect_run_class_32k
          ? (double)debug.medium_remote_collect_slot_class_32k /
                (double)debug.medium_remote_collect_run_class_32k
          : 0.0;
  double medium_slots_per_run_64k =
      debug.medium_remote_collect_run_class_64k
          ? (double)debug.medium_remote_collect_slot_class_64k /
                (double)debug.medium_remote_collect_run_class_64k
          : 0.0;
  printf("medium_remote_class_density slots_per_run=[%.3f,%.3f,%.3f,%.3f]\n",
         medium_slots_per_run_8k, medium_slots_per_run_16k,
         medium_slots_per_run_32k, medium_slots_per_run_64k);
  double medium_qpush_per_pub_8k =
      debug.medium_remote_publish_class_8k
          ? (double)debug.medium_remote_qpush_class_8k /
                (double)debug.medium_remote_publish_class_8k
          : 0.0;
  double medium_qpush_per_pub_16k =
      debug.medium_remote_publish_class_16k
          ? (double)debug.medium_remote_qpush_class_16k /
                (double)debug.medium_remote_publish_class_16k
          : 0.0;
  double medium_qpush_per_pub_32k =
      debug.medium_remote_publish_class_32k
          ? (double)debug.medium_remote_qpush_class_32k /
                (double)debug.medium_remote_publish_class_32k
          : 0.0;
  double medium_qpush_per_pub_64k =
      debug.medium_remote_publish_class_64k
          ? (double)debug.medium_remote_qpush_class_64k /
                (double)debug.medium_remote_publish_class_64k
          : 0.0;
  printf("medium_remote_class_episode qpush_per_pub=[%.3f,%.3f,%.3f,%.3f]\n",
         medium_qpush_per_pub_8k, medium_qpush_per_pub_16k,
         medium_qpush_per_pub_32k, medium_qpush_per_pub_64k);
  printf("medium_remote_queue push_attempt=%zu push_retry=%zu push_success=%zu\n",
         debug.medium_remote_queue_push_attempt_count,
         debug.medium_remote_queue_push_retry_count,
         debug.medium_remote_queue_push_success_count);
  printf("medium_refill_candidate_shadow install=%zu attempt=%zu hit=%zu owner_mismatch=%zu unusable=%zu\n",
         debug.medium_refill_candidate_install,
         debug.medium_refill_candidate_attempt,
         debug.medium_refill_candidate_hit,
         debug.medium_refill_candidate_owner_mismatch,
         debug.medium_refill_candidate_unusable);
  printf("medium_available_shadow add=%zu remove=%zu head_attempt=%zu head_hit=%zu head_unusable=%zu owner_hit_without_available=%zu duplicate=%zu owner_mismatch=%zu class_mismatch=%zu indexed_full=%zu indexed_detached=%zu indexed_nonactive=%zu exit_nonempty=%zu\n",
         debug.medium_available_add, debug.medium_available_remove,
         debug.medium_available_head_attempt,
         debug.medium_available_head_hit,
         debug.medium_available_head_unusable,
         debug.medium_available_owner_list_hit_without_available,
         debug.medium_available_duplicate_membership,
         debug.medium_available_owner_mismatch,
         debug.medium_available_class_mismatch,
         debug.medium_available_indexed_full,
         debug.medium_available_indexed_detached,
         debug.medium_available_indexed_nonactive,
         debug.medium_available_exit_nonempty);
  printf("medium_chunk create=%zu alloc=%zu reserved_bytes=%zu used_bytes=%zu\n",
         debug.medium_chunk_create_count, debug.medium_chunk_alloc_count,
         debug.medium_chunk_reserved_bytes, debug.medium_chunk_used_bytes);

  double medium_remote_pub = (double)debug.medium_remote_publish_count;
  double medium_collect_run = (double)debug.medium_remote_collect_run_count;
  double medium_collect_call = (double)debug.medium_remote_collect_call_count;
  double medium_lease_ns_per_pub =
      medium_remote_pub > 0.0
          ? (double)debug.medium_remote_owner_lease_ns / medium_remote_pub
          : 0.0;
  double medium_claim_ns_per_pub =
      medium_remote_pub > 0.0
          ? (double)debug.medium_remote_pending_claim_ns / medium_remote_pub
          : 0.0;
  double medium_qpush_ns_per_push =
      debug.medium_remote_queue_push_count
          ? (double)debug.medium_remote_queue_push_ns /
                (double)debug.medium_remote_queue_push_count
          : 0.0;
  double medium_collect_ns_per_run =
      medium_collect_run > 0.0
          ? (double)debug.medium_remote_collect_ns / medium_collect_run
          : 0.0;
  double medium_collect_runs_per_call =
      medium_collect_call > 0.0 ? medium_collect_run / medium_collect_call : 0.0;
  double medium_collect_slots_per_run =
      medium_collect_run > 0.0
          ? (double)debug.medium_remote_collect_slot_count / medium_collect_run
          : 0.0;
  printf("medium_residual lease_ns_per_pub=%.1f claim_ns_per_pub=%.1f qpush_ns_per_push=%.1f collect_ns_per_run=%.1f collect_runs_per_call=%.3f collect_slots_per_run=%.3f periodic_fast=%zu periodic_slow=%zu periodic_hit=%zu periodic_miss=%zu periodic_active=%zu periodic_owner=%zu active_refill_hint=%zu\n",
         medium_lease_ns_per_pub, medium_claim_ns_per_pub,
         medium_qpush_ns_per_push, medium_collect_ns_per_run,
         medium_collect_runs_per_call, medium_collect_slots_per_run,
         debug.medium_collect_periodic_fast_skip,
         debug.medium_collect_periodic_slow_enter,
         debug.medium_collect_periodic_pending_hit,
         debug.medium_collect_periodic_pending_miss,
         debug.medium_collect_periodic_from_active,
         debug.medium_collect_periodic_from_owner_list,
         debug.medium_collect_active_refill_hint);
  double medium_attributed_ms =
      (double)(debug.medium_remote_owner_lease_ns +
               debug.medium_remote_pending_claim_ns +
               debug.medium_remote_queue_push_ns + debug.medium_remote_collect_ns +
               debug.medium_global_lock_wait_ns + debug.medium_run_lock_wait_ns +
               debug.medium_alloc_slot_ns + debug.medium_free_slot_ns +
               debug.medium_madvise_ns) /
      1e6;
  double medium_lock_ms =
      (double)(debug.medium_global_lock_wait_ns +
               debug.medium_run_lock_wait_ns +
               debug.medium_remote_run_lock_ns) /
      1e6;
  double medium_slot_ms =
      (double)(debug.medium_alloc_slot_ns + debug.medium_free_slot_ns) / 1e6;
  size_t minor_median =
      h8_percentile_size_t(input->minor_faults, (size_t)opt->runs, 0.50);
  double bench_ops = (double)opt->threads * (double)opt->iters_per_thread;
  double minor_per_op = bench_ops > 0.0 ? (double)minor_median / bench_ops : 0.0;
  printf("medium_residual_budget attributed_ms=%.3f lock_ms=%.3f slot_ms=%.3f qpush_ms=%.3f collect_ms=%.3f lease_ms=%.3f claim_ms=%.3f madvise_ms=%.3f minor_faults_median=%zu minor_faults_per_op=%.6f\n",
         medium_attributed_ms, medium_lock_ms, medium_slot_ms,
         (double)debug.medium_remote_queue_push_ns / 1e6,
         (double)debug.medium_remote_collect_ns / 1e6,
         (double)debug.medium_remote_owner_lease_ns / 1e6,
         (double)debug.medium_remote_pending_claim_ns / 1e6,
         (double)debug.medium_madvise_ns / 1e6, minor_median, minor_per_op);
  size_t lower_median =
      h8_percentile_size_t(input->span_lower_bound, (size_t)opt->runs, 0.50);
  double actual_per_run =
      opt->runs > 0 ? (double)debug.local_span_commit / (double)opt->runs : 0.0;
  double span_excess_ratio =
      lower_median && debug.local_span_commit
          ? actual_per_run / (double)lower_median
          : 0.0;
  printf("span_commit_lower_bound actual_total=%zu actual_per_run=%.1f lower_bound_median=%zu excess_ratio=%.3f\n",
         debug.local_span_commit, actual_per_run, lower_median, span_excess_ratio);
  printf("local_scan_detail hint_null=%zu hint_full=%zu hint_state_blocked=%zu hint_trusted=%zu hint_class_mismatch=%zu hint_owner_mismatch=%zu hint_generation_mismatch=%zu hint_state_mismatch=%zu scan_usable=%zu scan_full=%zu scan_state_blocked=%zu skip_no_pending=%zu\n",
         debug.local_active_hint_null, debug.local_active_hint_full,
         debug.local_active_hint_state_blocked, debug.local_active_hint_trusted,
         debug.local_active_hint_class_mismatch,
         debug.local_active_hint_owner_mismatch,
         debug.local_active_hint_generation_mismatch,
         debug.local_active_hint_state_mismatch,
         debug.local_find_scan_span_usable, debug.local_find_scan_span_full,
         debug.local_find_scan_span_state_blocked,
         debug.local_find_skip_scan_no_pending);
  printf("local_hot live_alloc=%zu live_free=%zu word0=%zu word1=%zu word2_7=%zu word8_31=%zu word32_63=%zu free_head_alloc=%zu free_head_free=%zu pending_alloc=%zu pending_free=%zu used_alloc=%zu used_free=%zu\n",
         debug.local_live_touch_alloc, debug.local_live_touch_free,
         debug.local_live_word_0, debug.local_live_word_1,
         debug.local_live_word_2_7, debug.local_live_word_8_31,
         debug.local_live_word_32_63, debug.local_free_head_touch_alloc,
         debug.local_free_head_touch_free, debug.local_pending_check_alloc,
         debug.local_pending_check_free, debug.local_used_touch_alloc,
         debug.local_used_touch_free);
  printf("used_count_detail load_alloc=%zu store_alloc=%zu load_free=%zu store_free=%zu full_check=%zu underflow=%zu mirror_mismatch=%zu mirror_underflow=%zu\n",
         debug.local_used_count_load_alloc, debug.local_used_count_store_alloc,
         debug.local_used_count_load_free, debug.local_used_count_store_free,
         debug.local_used_count_full_check, debug.local_used_count_underflow,
         debug.local_used_mirror_mismatch, debug.local_used_mirror_underflow);
  printf("used_count_cold active_hint=%zu owner_scan_locked=%zu adoption_locked=%zu owner_exit=%zu verify_quiescent=%zu derived_mismatch=%zu derived_scan=%zu\n",
         debug.local_used_cold_active_hint,
         debug.local_used_cold_owner_scan_locked,
         debug.local_used_cold_adoption_locked, debug.local_used_cold_owner_exit,
         debug.local_used_cold_verify_quiescent, debug.local_used_derived_mismatch,
         debug.local_used_derived_quiescent_scan);
  printf("span_commit_timing total_ms=%.3f meta_ms=%.3f mprotect_ms=%.3f\n",
         (double)debug.span_commit_total_ns / 1e6,
         (double)debug.span_commit_meta_ns / 1e6,
         (double)debug.span_commit_mprotect_ns / 1e6);
  printf("lifecycle_timing owner_exit_total_ms=%.3f owner_exit_collect_ms=%.3f owner_exit_span_walk_ms=%.3f span_retire_count=%zu span_retire_total_ms=%.3f span_retire_lock_wait_ms=%.3f span_retire_madvise_ms=%.3f span_retire_meta_free_ms=%.3f\n",
         (double)debug.owner_exit_total_ns / 1e6,
         (double)debug.owner_exit_collect_ns / 1e6,
         (double)debug.owner_exit_span_walk_ns / 1e6, debug.span_retire_count,
         (double)debug.span_retire_total_ns / 1e6,
         (double)debug.span_retire_lock_wait_ns / 1e6,
         (double)debug.span_retire_madvise_ns / 1e6,
         (double)debug.span_retire_meta_free_ns / 1e6);
  double purge_avg =
      debug.span_purge_run_count
          ? (double)debug.span_purge_run_spans_total /
                (double)debug.span_purge_run_count
          : 0.0;
  printf("span_purge runs=%zu spans=%zu avg_spans_per_run=%.3f max_run=%zu singleton_runs=%zu madvise_calls=%zu madvise_bytes=%zu madvise_ms=%.3f\n",
         debug.span_purge_run_count, debug.span_purge_run_spans_total,
         purge_avg, debug.span_purge_run_max, debug.span_purge_singleton_runs,
         debug.span_purge_madvise_calls, debug.span_purge_madvise_bytes,
         (double)debug.span_purge_madvise_ns / 1e6);
  printf("slot_shadow valid_mismatch=%zu invalid_mismatch=%zu pending_nonallocated=%zu free_unreachable=%zu free_duplicate=%zu free_cycle=%zu bad_next=%zu never_used_below_bump=%zu nonvirgin_above_bump=%zu used_mismatch=%zu reserved_quiescent=%zu\n",
         debug.slot_shadow_valid_mismatch, debug.slot_shadow_invalid_mismatch,
         debug.slot_shadow_pending_nonallocated, debug.slot_shadow_free_unreachable,
         debug.slot_shadow_free_duplicate, debug.slot_shadow_free_cycle,
         debug.slot_shadow_bad_next, debug.slot_shadow_never_used_below_bump,
         debug.slot_shadow_nonvirgin_above_bump, debug.slot_shadow_used_mismatch,
         debug.slot_shadow_reserved_quiescent);
}
