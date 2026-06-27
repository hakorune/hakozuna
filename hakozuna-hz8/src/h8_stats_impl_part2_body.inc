  out->medium_run_create_class_32k = atomic_load_explicit(
      &h8g.medium_run_create_class_32k, memory_order_acquire);
  out->medium_run_create_class_64k = atomic_load_explicit(
      &h8g.medium_run_create_class_64k, memory_order_acquire);
  out->medium_local_free_class_8k = atomic_load_explicit(
      &h8g.medium_local_free_class_8k, memory_order_acquire);
  out->medium_local_free_class_16k = atomic_load_explicit(
      &h8g.medium_local_free_class_16k, memory_order_acquire);
  out->medium_local_free_class_32k = atomic_load_explicit(
      &h8g.medium_local_free_class_32k, memory_order_acquire);
  out->medium_local_free_class_64k = atomic_load_explicit(
      &h8g.medium_local_free_class_64k, memory_order_acquire);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  out->medium_local_free_class_24k = atomic_load_explicit(
      &h8g.medium_local_free_class_24k, memory_order_acquire);
  out->medium_local_free_class_48k = atomic_load_explicit(
      &h8g.medium_local_free_class_48k, memory_order_acquire);
  out->medium_24k_local_free_decode_attempt = atomic_load_explicit(
      &h8g.medium_24k_local_free_decode_attempt, memory_order_acquire);
  out->medium_24k_local_free_decode_valid_slot0 = atomic_load_explicit(
      &h8g.medium_24k_local_free_decode_valid_slot0, memory_order_acquire);
  out->medium_24k_local_free_decode_valid_slot1 = atomic_load_explicit(
      &h8g.medium_24k_local_free_decode_valid_slot1, memory_order_acquire);
  out->medium_24k_local_free_decode_invalid = atomic_load_explicit(
      &h8g.medium_24k_local_free_decode_invalid, memory_order_acquire);
  out->medium_24k_local_free_decode_equiv_mismatch =
      atomic_load_explicit(&h8g.medium_24k_local_free_decode_equiv_mismatch,
                           memory_order_acquire);
#else
  out->medium_local_free_class_24k = 0;
  out->medium_local_free_class_48k = 0;
  out->medium_24k_local_free_decode_attempt = 0;
  out->medium_24k_local_free_decode_valid_slot0 = 0;
  out->medium_24k_local_free_decode_valid_slot1 = 0;
  out->medium_24k_local_free_decode_invalid = 0;
  out->medium_24k_local_free_decode_equiv_mismatch = 0;
#endif
  out->medium_run_madvise_count =
      atomic_load_explicit(&h8g.medium_run_madvise_count, memory_order_acquire);
  out->medium_chunk_create_count =
      atomic_load_explicit(&h8g.medium_chunk_create_count, memory_order_acquire);
  out->medium_chunk_alloc_count =
      atomic_load_explicit(&h8g.medium_chunk_alloc_count, memory_order_acquire);
  out->medium_chunk_reserved_bytes = atomic_load_explicit(
      &h8g.medium_chunk_reserved_bytes, memory_order_acquire);
  out->medium_chunk_used_bytes =
      atomic_load_explicit(&h8g.medium_chunk_used_bytes, memory_order_acquire);
  out->medium_owner_scan_count =
      atomic_load_explicit(&h8g.medium_owner_scan_count, memory_order_acquire);
  out->medium_owner_scan_step_count = atomic_load_explicit(
      &h8g.medium_owner_scan_step_count, memory_order_acquire);
  out->medium_global_scan_count =
      atomic_load_explicit(&h8g.medium_global_scan_count, memory_order_acquire);
  out->medium_global_scan_step_count = atomic_load_explicit(
      &h8g.medium_global_scan_step_count, memory_order_acquire);
  out->medium_free_lookup_count =
      atomic_load_explicit(&h8g.medium_free_lookup_count, memory_order_acquire);
  out->medium_route_lookup_count =
      atomic_load_explicit(&h8g.medium_route_lookup_count, memory_order_acquire);
  out->medium_invalid_owned_count =
      atomic_load_explicit(&h8g.medium_invalid_owned_count, memory_order_acquire);
  out->medium_empty_transition_count = atomic_load_explicit(
      &h8g.medium_empty_transition_count, memory_order_acquire);
  out->medium_empty_retain_count =
      atomic_load_explicit(&h8g.medium_empty_retain_count, memory_order_acquire);
  out->medium_empty_budget_reject_count = atomic_load_explicit(
      &h8g.medium_empty_budget_reject_count, memory_order_acquire);
  out->medium_empty_reactivate_count = atomic_load_explicit(
      &h8g.medium_empty_reactivate_count, memory_order_acquire);
  out->medium_owner_exit_drain_count = atomic_load_explicit(
      &h8g.medium_owner_exit_drain_count, memory_order_acquire);
  out->medium_collect_active_would_keep = atomic_load_explicit(
      &h8g.medium_collect_active_would_keep, memory_order_acquire);
  out->medium_collect_ctx_missing = atomic_load_explicit(
      &h8g.medium_collect_ctx_missing, memory_order_acquire);
  out->medium_collect_ctx_owner_mismatch = atomic_load_explicit(
      &h8g.medium_collect_ctx_owner_mismatch, memory_order_acquire);
  out->medium_collect_active_hint_mismatch = atomic_load_explicit(
      &h8g.medium_collect_active_hint_mismatch, memory_order_acquire);
  out->medium_collect_active_not_owned = atomic_load_explicit(
      &h8g.medium_collect_active_not_owned, memory_order_acquire);
  out->medium_empty_live_not_current_active = atomic_load_explicit(
      &h8g.medium_empty_live_not_current_active, memory_order_acquire);
  out->medium_active_live_empty_bytes = atomic_load_explicit(
      &h8g.medium_active_live_empty_bytes, memory_order_acquire);
  out->medium_active_live_empty_peak = atomic_load_explicit(
      &h8g.medium_active_live_empty_peak, memory_order_acquire);
  out->medium_owner_exit_active_live_remaining = atomic_load_explicit(
      &h8g.medium_owner_exit_active_live_remaining, memory_order_acquire);
  out->medium_warm1_would_install = atomic_load_explicit(
      &h8g.medium_warm1_would_install, memory_order_acquire);
  out->medium_warm1_would_replace = atomic_load_explicit(
      &h8g.medium_warm1_would_replace, memory_order_acquire);
  out->medium_warm1_reuse_hit =
      atomic_load_explicit(&h8g.medium_warm1_reuse_hit, memory_order_acquire);
  out->medium_warm1_would_avoid_budget_reject = atomic_load_explicit(
      &h8g.medium_warm1_would_avoid_budget_reject, memory_order_acquire);
  out->medium_warm2_would_install = atomic_load_explicit(
      &h8g.medium_warm2_would_install, memory_order_acquire);
  out->medium_warm2_would_replace = atomic_load_explicit(
      &h8g.medium_warm2_would_replace, memory_order_acquire);
  out->medium_warm2_reuse_hit =
      atomic_load_explicit(&h8g.medium_warm2_reuse_hit, memory_order_acquire);
  out->medium_warm2_would_avoid_budget_reject = atomic_load_explicit(
      &h8g.medium_warm2_would_avoid_budget_reject, memory_order_acquire);
  out->medium_warm_reuse_distance_0 = atomic_load_explicit(
      &h8g.medium_warm_reuse_distance_0, memory_order_acquire);
  out->medium_warm_reuse_distance_1 = atomic_load_explicit(
      &h8g.medium_warm_reuse_distance_1, memory_order_acquire);
  out->medium_warm_reuse_distance_2 = atomic_load_explicit(
      &h8g.medium_warm_reuse_distance_2, memory_order_acquire);
  out->medium_warm_reuse_distance_3p = atomic_load_explicit(
      &h8g.medium_warm_reuse_distance_3p, memory_order_acquire);
  out->medium_retention_empty_seen = atomic_load_explicit(
      &h8g.medium_retention_empty_seen, memory_order_acquire);
  out->medium_retention_pre_distance_1 = atomic_load_explicit(
      &h8g.medium_retention_pre_distance_1, memory_order_acquire);
  out->medium_retention_pre_distance_2 = atomic_load_explicit(
      &h8g.medium_retention_pre_distance_2, memory_order_acquire);
  out->medium_retention_pre_distance_3 = atomic_load_explicit(
      &h8g.medium_retention_pre_distance_3, memory_order_acquire);
  out->medium_retention_pre_distance_4 = atomic_load_explicit(
      &h8g.medium_retention_pre_distance_4, memory_order_acquire);
  out->medium_retention_pre_distance_5p = atomic_load_explicit(
      &h8g.medium_retention_pre_distance_5p, memory_order_acquire);
  out->medium_retention_decommit_budget = atomic_load_explicit(
      &h8g.medium_retention_decommit_budget, memory_order_acquire);
  out->medium_retention_decommit_cold = atomic_load_explicit(
      &h8g.medium_retention_decommit_cold, memory_order_acquire);
  out->medium_retention_decommit_owner_exit = atomic_load_explicit(
      &h8g.medium_retention_decommit_owner_exit, memory_order_acquire);
  out->medium_retention_ghost_reuse_budget = atomic_load_explicit(
      &h8g.medium_retention_ghost_reuse_budget, memory_order_acquire);
  out->medium_retention_ghost_reuse_cold = atomic_load_explicit(
      &h8g.medium_retention_ghost_reuse_cold, memory_order_acquire);
  out->medium_retention_ghost_reuse_owner_exit = atomic_load_explicit(
      &h8g.medium_retention_ghost_reuse_owner_exit, memory_order_acquire);
  out->medium_retention_ghost_distance_1 = atomic_load_explicit(
      &h8g.medium_retention_ghost_distance_1, memory_order_acquire);
  out->medium_retention_ghost_distance_2 = atomic_load_explicit(
      &h8g.medium_retention_ghost_distance_2, memory_order_acquire);
  out->medium_retention_ghost_distance_3 = atomic_load_explicit(
      &h8g.medium_retention_ghost_distance_3, memory_order_acquire);
  out->medium_retention_ghost_distance_4 = atomic_load_explicit(
      &h8g.medium_retention_ghost_distance_4, memory_order_acquire);
  out->medium_retention_ghost_distance_5p = atomic_load_explicit(
      &h8g.medium_retention_ghost_distance_5p, memory_order_acquire);
  out->medium_retention_ghost_epoch_0_1 = atomic_load_explicit(
      &h8g.medium_retention_ghost_epoch_0_1, memory_order_acquire);
  out->medium_retention_ghost_epoch_2_3 = atomic_load_explicit(
      &h8g.medium_retention_ghost_epoch_2_3, memory_order_acquire);
  out->medium_retention_ghost_epoch_4_7 = atomic_load_explicit(
      &h8g.medium_retention_ghost_epoch_4_7, memory_order_acquire);
  out->medium_retention_ghost_epoch_8p = atomic_load_explicit(
      &h8g.medium_retention_ghost_epoch_8p, memory_order_acquire);
  out->medium_retention_model_decommit_n0 = atomic_load_explicit(
      &h8g.medium_retention_model_decommit_n0, memory_order_acquire);
  out->medium_retention_model_decommit_n1 = atomic_load_explicit(
      &h8g.medium_retention_model_decommit_n1, memory_order_acquire);
  out->medium_retention_model_decommit_n2 = atomic_load_explicit(
      &h8g.medium_retention_model_decommit_n2, memory_order_acquire);
  out->medium_retention_model_decommit_n3 = atomic_load_explicit(
      &h8g.medium_retention_model_decommit_n3, memory_order_acquire);
  out->medium_retention_model_decommit_n4 = atomic_load_explicit(
      &h8g.medium_retention_model_decommit_n4, memory_order_acquire);
  out->medium_retention_model_refault_n0 = atomic_load_explicit(
      &h8g.medium_retention_model_refault_n0, memory_order_acquire);
  out->medium_retention_model_refault_n1 = atomic_load_explicit(
      &h8g.medium_retention_model_refault_n1, memory_order_acquire);
  out->medium_retention_model_refault_n2 = atomic_load_explicit(
      &h8g.medium_retention_model_refault_n2, memory_order_acquire);
  out->medium_retention_model_refault_n3 = atomic_load_explicit(
      &h8g.medium_retention_model_refault_n3, memory_order_acquire);
  out->medium_retention_model_refault_n4 = atomic_load_explicit(
      &h8g.medium_retention_model_refault_n4, memory_order_acquire);
  out->medium_retention_l3_m0_mismatch = atomic_load_explicit(
      &h8g.medium_retention_l3_m0_mismatch, memory_order_acquire);
  out->medium_retention_l3_m0_decommit = atomic_load_explicit(
      &h8g.medium_retention_l3_m0_decommit, memory_order_acquire);
  out->medium_retention_l3_m0_refault = atomic_load_explicit(
      &h8g.medium_retention_l3_m0_refault, memory_order_acquire);
  out->medium_retention_l3_m1_decommit = atomic_load_explicit(
      &h8g.medium_retention_l3_m1_decommit, memory_order_acquire);
  out->medium_retention_l3_m1_refault = atomic_load_explicit(
      &h8g.medium_retention_l3_m1_refault, memory_order_acquire);
  out->medium_retention_l3_m2_decommit = atomic_load_explicit(
      &h8g.medium_retention_l3_m2_decommit, memory_order_acquire);
  out->medium_retention_l3_m2_refault = atomic_load_explicit(
      &h8g.medium_retention_l3_m2_refault, memory_order_acquire);
  out->medium_retention_l3_clock_decommit = atomic_load_explicit(
      &h8g.medium_retention_l3_clock_decommit, memory_order_acquire);
  out->medium_retention_l3_clock_refault = atomic_load_explicit(
      &h8g.medium_retention_l3_clock_refault, memory_order_acquire);
  out->medium_retention_l3_m0_bytes = atomic_load_explicit(
      &h8g.medium_retention_l3_m0_bytes, memory_order_acquire);
  out->medium_retention_l3_m0_peak = atomic_load_explicit(
      &h8g.medium_retention_l3_m0_peak, memory_order_acquire);
  out->medium_retention_l3_m1_bytes = atomic_load_explicit(
      &h8g.medium_retention_l3_m1_bytes, memory_order_acquire);
  out->medium_retention_l3_m1_peak = atomic_load_explicit(
      &h8g.medium_retention_l3_m1_peak, memory_order_acquire);
  out->medium_retention_l3_m2_bytes = atomic_load_explicit(
      &h8g.medium_retention_l3_m2_bytes, memory_order_acquire);
  out->medium_retention_l3_m2_peak = atomic_load_explicit(
      &h8g.medium_retention_l3_m2_peak, memory_order_acquire);
  out->medium_retention_l3_clock_bytes = atomic_load_explicit(
      &h8g.medium_retention_l3_clock_bytes, memory_order_acquire);
  out->medium_retention_l3_clock_peak = atomic_load_explicit(
      &h8g.medium_retention_l3_clock_peak, memory_order_acquire);
  out->medium_lazy_purge_candidate = atomic_load_explicit(
      &h8g.medium_lazy_purge_candidate, memory_order_acquire);
  out->medium_lazy_purge_reuse = atomic_load_explicit(
      &h8g.medium_lazy_purge_reuse, memory_order_acquire);
  out->medium_lazy_purge_bytes = atomic_load_explicit(
      &h8g.medium_lazy_purge_bytes, memory_order_acquire);
  out->medium_lazy_purge_peak = atomic_load_explicit(
      &h8g.medium_lazy_purge_peak, memory_order_acquire);
  out->medium_lazy_purge_over_16m = atomic_load_explicit(
      &h8g.medium_lazy_purge_over_16m, memory_order_acquire);
  out->medium_lazy_purge_over_32m = atomic_load_explicit(
      &h8g.medium_lazy_purge_over_32m, memory_order_acquire);
  out->medium_lazy_charge_acquire = atomic_load_explicit(
      &h8g.medium_lazy_charge_acquire, memory_order_acquire);
  out->medium_lazy_charge_reuse = atomic_load_explicit(
      &h8g.medium_lazy_charge_reuse, memory_order_acquire);
  out->medium_lazy_charge_keep_live = atomic_load_explicit(
      &h8g.medium_lazy_charge_keep_live, memory_order_acquire);
  out->medium_lazy_charge_empty_fast = atomic_load_explicit(
      &h8g.medium_lazy_charge_empty_fast, memory_order_acquire);
  out->medium_lazy_charge_drop = atomic_load_explicit(
      &h8g.medium_lazy_charge_drop, memory_order_acquire);
  out->medium_lazy_drop_detach = atomic_load_explicit(
      &h8g.medium_lazy_drop_detach, memory_order_acquire);
  out->medium_lazy_drop_destroy = atomic_load_explicit(
      &h8g.medium_lazy_drop_destroy, memory_order_acquire);
  out->medium_lazy_normal_budget_skip = atomic_load_explicit(
      &h8g.medium_lazy_normal_budget_skip, memory_order_acquire);
  out->medium_lazy_cap_reject = atomic_load_explicit(
      &h8g.medium_lazy_cap_reject, memory_order_acquire);
  out->medium_lazy_cas_retry = atomic_load_explicit(
      &h8g.medium_lazy_cas_retry, memory_order_acquire);
  out->medium_madvise_fail_count =
      atomic_load_explicit(&h8g.medium_madvise_fail_count, memory_order_acquire);
  out->medium_resident_empty_bytes =
      atomic_load_explicit(&h8g.medium_resident_empty_bytes, memory_order_acquire);
  out->medium_resident_empty_peak =
      atomic_load_explicit(&h8g.medium_resident_empty_peak, memory_order_acquire);
  out->medium_madvise_ns =
      atomic_load_explicit(&h8g.medium_madvise_ns, memory_order_acquire);
  out->medium_global_lock_wait_ns =
      atomic_load_explicit(&h8g.medium_global_lock_wait_ns, memory_order_acquire);
  out->medium_run_lock_wait_ns =
      atomic_load_explicit(&h8g.medium_run_lock_wait_ns, memory_order_acquire);
  out->medium_alloc_slot_ns =
      atomic_load_explicit(&h8g.medium_alloc_slot_ns, memory_order_acquire);
  out->medium_free_slot_ns =
      atomic_load_explicit(&h8g.medium_free_slot_ns, memory_order_acquire);
  out->medium_alloc_slot_count =
      atomic_load_explicit(&h8g.medium_alloc_slot_count, memory_order_acquire);
  out->medium_free_slot_count =
      atomic_load_explicit(&h8g.medium_free_slot_count, memory_order_acquire);
  out->medium_alloc_mark_live_nonempty = atomic_load_explicit(
      &h8g.medium_alloc_mark_live_nonempty, memory_order_acquire);
  out->medium_alloc_mark_live_live = atomic_load_explicit(
      &h8g.medium_alloc_mark_live_live, memory_order_acquire);
  out->medium_alloc_mark_live_active_empty = atomic_load_explicit(
      &h8g.medium_alloc_mark_live_active_empty, memory_order_acquire);
  out->medium_alloc_mark_live_resident = atomic_load_explicit(
      &h8g.medium_alloc_mark_live_resident, memory_order_acquire);
  out->medium_alloc_mark_live_decommitted = atomic_load_explicit(
      &h8g.medium_alloc_mark_live_decommitted, memory_order_acquire);
  out->medium_alloc_state_check_fail = atomic_load_explicit(
      &h8g.medium_alloc_state_check_fail, memory_order_acquire);
  out->medium_alloc_free_mask_zero = atomic_load_explicit(
      &h8g.medium_alloc_free_mask_zero, memory_order_acquire);
  out->medium_local_free_pending_nonzero = atomic_load_explicit(
      &h8g.medium_local_free_pending_nonzero, memory_order_acquire);
  out->medium_free_cache_attempt = atomic_load_explicit(
      &h8g.medium_free_cache_attempt, memory_order_acquire);
  out->medium_free_cache_range_hit = atomic_load_explicit(
      &h8g.medium_free_cache_range_hit, memory_order_acquire);
  out->medium_free_cache_owner_hit = atomic_load_explicit(
      &h8g.medium_free_cache_owner_hit, memory_order_acquire);
  out->medium_free_cache_slot_hit = atomic_load_explicit(
      &h8g.medium_free_cache_slot_hit, memory_order_acquire);
  out->medium_free_cache_would_succeed = atomic_load_explicit(
      &h8g.medium_free_cache_would_succeed, memory_order_acquire);
  out->medium_free_cache_pending_block = atomic_load_explicit(
      &h8g.medium_free_cache_pending_block, memory_order_acquire);
  out->medium_free_cache_state_block = atomic_load_explicit(
      &h8g.medium_free_cache_state_block, memory_order_acquire);
  out->medium_free_cache_directory_mismatch = atomic_load_explicit(
      &h8g.medium_free_cache_directory_mismatch, memory_order_acquire);
  out->medium_free_cache_fallback = atomic_load_explicit(
      &h8g.medium_free_cache_fallback, memory_order_acquire);
  out->medium_lock_elide_alloc_candidate = atomic_load_explicit(
      &h8g.medium_lock_elide_alloc_candidate, memory_order_acquire);
  out->medium_lock_elide_free_candidate = atomic_load_explicit(
      &h8g.medium_lock_elide_free_candidate, memory_order_acquire);
  out->medium_lock_elide_owner_mismatch = atomic_load_explicit(
      &h8g.medium_lock_elide_owner_mismatch, memory_order_acquire);
  out->medium_active_alloc_owner_mismatch = atomic_load_explicit(
      &h8g.medium_active_alloc_owner_mismatch, memory_order_acquire);
  out->medium_owner_list_owner_mismatch = atomic_load_explicit(
      &h8g.medium_owner_list_owner_mismatch, memory_order_acquire);
  out->medium_global_skip_foreign_attached = atomic_load_explicit(
      &h8g.medium_global_skip_foreign_attached, memory_order_acquire);
  out->medium_local_free_owner_match = atomic_load_explicit(
      &h8g.medium_local_free_owner_match, memory_order_acquire);
  out->medium_remote_free_owner_mismatch = atomic_load_explicit(
      &h8g.medium_remote_free_owner_mismatch, memory_order_acquire);
  out->medium_free_lookup_step_count = atomic_load_explicit(
      &h8g.medium_free_lookup_step_count, memory_order_acquire);
  out->medium_route_lookup_step_count = atomic_load_explicit(
      &h8g.medium_route_lookup_step_count, memory_order_acquire);
  out->medium_route_authority_mismatch = atomic_load_explicit(
      &h8g.medium_route_authority_mismatch, memory_order_acquire);
  out->medium_remote_publish_count = atomic_load_explicit(
      &h8g.medium_remote_publish_count, memory_order_acquire);
  out->medium_remote_owner_lease_ns = atomic_load_explicit(
      &h8g.medium_remote_owner_lease_ns, memory_order_acquire);
  out->medium_remote_owner_lease_enter_ns = atomic_load_explicit(
      &h8g.medium_remote_owner_lease_enter_ns, memory_order_acquire);
  out->medium_remote_owner_lease_exit_ns = atomic_load_explicit(
      &h8g.medium_remote_owner_lease_exit_ns, memory_order_acquire);
  out->medium_remote_run_lock_ns = atomic_load_explicit(
      &h8g.medium_remote_run_lock_ns, memory_order_acquire);
  out->medium_remote_pending_claim_count = atomic_load_explicit(
      &h8g.medium_remote_pending_claim_count, memory_order_acquire);
  out->medium_remote_pending_claim_ns = atomic_load_explicit(
      &h8g.medium_remote_pending_claim_ns, memory_order_acquire);
  out->medium_remote_lockless_claim_count = atomic_load_explicit(
      &h8g.medium_remote_lockless_claim_count, memory_order_acquire);
  out->medium_remote_lockless_claim_collector_accept = atomic_load_explicit(
      &h8g.medium_remote_lockless_claim_collector_accept, memory_order_acquire);
  out->medium_remote_lockless_claim_rollback_invalid = atomic_load_explicit(
      &h8g.medium_remote_lockless_claim_rollback_invalid, memory_order_acquire);
  out->medium_remote_lockless_claim_rollback_accepted = atomic_load_explicit(
      &h8g.medium_remote_lockless_claim_rollback_accepted,
      memory_order_acquire);
  out->medium_attached_writer_overlap = atomic_load_explicit(
      &h8g.medium_attached_writer_overlap, memory_order_acquire);
  out->medium_attached_foreign_mask_writer = atomic_load_explicit(
      &h8g.medium_attached_foreign_mask_writer, memory_order_acquire);
  out->medium_owner_token_changed_during_mutation = atomic_load_explicit(
      &h8g.medium_owner_token_changed_during_mutation, memory_order_acquire);
  out->medium_collect_wrong_owner = atomic_load_explicit(
      &h8g.medium_collect_wrong_owner, memory_order_acquire);
  out->medium_detached_direct_free_while_attached = atomic_load_explicit(
      &h8g.medium_detached_direct_free_while_attached, memory_order_acquire);
  out->medium_remote_lockless_shadow_attempt = atomic_load_explicit(
      &h8g.medium_remote_lockless_shadow_attempt, memory_order_acquire);
  out->medium_remote_lockless_shadow_would_accept = atomic_load_explicit(
      &h8g.medium_remote_lockless_shadow_would_accept, memory_order_acquire);
  out->medium_remote_lockless_shadow_would_reject = atomic_load_explicit(
      &h8g.medium_remote_lockless_shadow_would_reject, memory_order_acquire);
  out->medium_remote_lockless_shadow_match = atomic_load_explicit(
      &h8g.medium_remote_lockless_shadow_match, memory_order_acquire);
  out->medium_remote_lockless_shadow_mismatch = atomic_load_explicit(
      &h8g.medium_remote_lockless_shadow_mismatch, memory_order_acquire);
  out->medium_remote_notify_count = atomic_load_explicit(
      &h8g.medium_remote_notify_count, memory_order_acquire);
  out->medium_remote_queue_push_count = atomic_load_explicit(
      &h8g.medium_remote_queue_push_count, memory_order_acquire);
  out->medium_remote_queue_push_attempt_count = atomic_load_explicit(
      &h8g.medium_remote_queue_push_attempt_count, memory_order_acquire);
  out->medium_remote_queue_push_retry_count = atomic_load_explicit(
      &h8g.medium_remote_queue_push_retry_count, memory_order_acquire);
  out->medium_remote_queue_push_success_count = atomic_load_explicit(
      &h8g.medium_remote_queue_push_success_count, memory_order_acquire);
  out->medium_remote_queue_push_ns = atomic_load_explicit(
      &h8g.medium_remote_queue_push_ns, memory_order_acquire);
  out->medium_remote_collect_call_count = atomic_load_explicit(
      &h8g.medium_remote_collect_call_count, memory_order_acquire);
  out->medium_remote_collect_run_count = atomic_load_explicit(
      &h8g.medium_remote_collect_run_count, memory_order_acquire);
  out->medium_remote_collect_slot_count = atomic_load_explicit(
      &h8g.medium_remote_collect_slot_count, memory_order_acquire);
  out->medium_remote_collect_reject_count = atomic_load_explicit(
      &h8g.medium_remote_collect_reject_count, memory_order_acquire);
  out->medium_remote_collect_ns = atomic_load_explicit(
      &h8g.medium_remote_collect_ns, memory_order_acquire);
  out->medium_collect_state_ns = atomic_load_explicit(
      &h8g.medium_collect_state_ns, memory_order_acquire);
  out->medium_collect_pending_clear_ns = atomic_load_explicit(
      &h8g.medium_collect_pending_clear_ns, memory_order_acquire);
  out->medium_collect_mask_ns = atomic_load_explicit(
      &h8g.medium_collect_mask_ns, memory_order_acquire);
  out->medium_collect_empty_ns = atomic_load_explicit(
      &h8g.medium_collect_empty_ns, memory_order_acquire);
  out->medium_collect_finish_pending_rearm = atomic_load_explicit(
      &h8g.medium_collect_finish_pending_rearm, memory_order_acquire);
  out->medium_empty_with_pending = atomic_load_explicit(
      &h8g.medium_empty_with_pending, memory_order_acquire);
  out->medium_lease_enter_decision_mismatch = atomic_load_explicit(
      &h8g.medium_lease_enter_decision_mismatch, memory_order_acquire);
  out->medium_lease_ref_underflow = atomic_load_explicit(
      &h8g.medium_lease_ref_underflow, memory_order_acquire);
  out->medium_lease_ref_nonzero_at_owner_exit = atomic_load_explicit(
      &h8g.medium_lease_ref_nonzero_at_owner_exit, memory_order_acquire);
  out->medium_lease_enter_after_close = atomic_load_explicit(
      &h8g.medium_lease_enter_after_close, memory_order_acquire);
  out->medium_owner_reuse_with_medium_refs = atomic_load_explicit(
      &h8g.medium_owner_reuse_with_medium_refs, memory_order_acquire);
  out->medium_remote_publish_class_8k = atomic_load_explicit(
      &h8g.medium_remote_publish_class_8k, memory_order_acquire);
  out->medium_remote_publish_class_16k = atomic_load_explicit(
      &h8g.medium_remote_publish_class_16k, memory_order_acquire);
  out->medium_remote_publish_class_32k = atomic_load_explicit(
      &h8g.medium_remote_publish_class_32k, memory_order_acquire);
  out->medium_remote_publish_class_64k = atomic_load_explicit(
      &h8g.medium_remote_publish_class_64k, memory_order_acquire);
  out->medium_remote_qpush_class_8k = atomic_load_explicit(
      &h8g.medium_remote_qpush_class_8k, memory_order_acquire);
  out->medium_remote_qpush_class_16k = atomic_load_explicit(
      &h8g.medium_remote_qpush_class_16k, memory_order_acquire);
  out->medium_remote_qpush_class_32k = atomic_load_explicit(
      &h8g.medium_remote_qpush_class_32k, memory_order_acquire);
  out->medium_remote_qpush_class_64k = atomic_load_explicit(
      &h8g.medium_remote_qpush_class_64k, memory_order_acquire);
  out->medium_remote_collect_run_class_8k = atomic_load_explicit(
      &h8g.medium_remote_collect_run_class_8k, memory_order_acquire);
  out->medium_remote_collect_run_class_16k = atomic_load_explicit(
      &h8g.medium_remote_collect_run_class_16k, memory_order_acquire);
  out->medium_remote_collect_run_class_32k = atomic_load_explicit(
      &h8g.medium_remote_collect_run_class_32k, memory_order_acquire);
  out->medium_remote_collect_run_class_64k = atomic_load_explicit(
      &h8g.medium_remote_collect_run_class_64k, memory_order_acquire);
  out->medium_remote_collect_slot_class_8k = atomic_load_explicit(
      &h8g.medium_remote_collect_slot_class_8k, memory_order_acquire);
  out->medium_remote_collect_slot_class_16k = atomic_load_explicit(
      &h8g.medium_remote_collect_slot_class_16k, memory_order_acquire);
  out->medium_remote_collect_slot_class_32k = atomic_load_explicit(
      &h8g.medium_remote_collect_slot_class_32k, memory_order_acquire);
  out->medium_remote_collect_slot_class_64k = atomic_load_explicit(
      &h8g.medium_remote_collect_slot_class_64k, memory_order_acquire);
  out->pending_collect_word_count =
      atomic_load_explicit(&h8g.pending_collect_word_count, memory_order_acquire);
  out->pending_collect_word_nonzero_count = atomic_load_explicit(
      &h8g.pending_collect_word_nonzero_count, memory_order_acquire);
  out->pending_collect_bit_count =
      atomic_load_explicit(&h8g.pending_collect_bit_count, memory_order_acquire);
  out->pending_mask_notify_without_count = atomic_load_explicit(
      &h8g.pending_mask_notify_without_count, memory_order_acquire);
  out->pending_count_notify_without_mask = atomic_load_explicit(
      &h8g.pending_count_notify_without_mask, memory_order_acquire);
  out->pending_mask_requeue_without_count = atomic_load_explicit(
      &h8g.pending_mask_requeue_without_count, memory_order_acquire);
  out->pending_count_requeue_without_mask = atomic_load_explicit(
      &h8g.pending_count_requeue_without_mask, memory_order_acquire);
  out->pending_finish_count_mask_zero_bitmap_zero = atomic_load_explicit(
      &h8g.pending_finish_count_mask_zero_bitmap_zero, memory_order_acquire);
  out->pending_finish_count_mask_zero_bitmap_nonzero = atomic_load_explicit(
      &h8g.pending_finish_count_mask_zero_bitmap_nonzero, memory_order_acquire);
  out->pending_finish_mask_nonzero_bitmap_zero = atomic_load_explicit(
      &h8g.pending_finish_mask_nonzero_bitmap_zero, memory_order_acquire);
  out->pending_finish_mask_nonzero_bitmap_nonzero = atomic_load_explicit(
      &h8g.pending_finish_mask_nonzero_bitmap_nonzero, memory_order_acquire);
  out->qstate_dirty_set =
      atomic_load_explicit(&h8g.qstate_dirty_set, memory_order_acquire);
  out->qstate_dirty_self_set =
      atomic_load_explicit(&h8g.qstate_dirty_self_set, memory_order_acquire);
  out->qstate_dirty_requeue =
      atomic_load_explicit(&h8g.qstate_dirty_requeue, memory_order_acquire);
  out->quiescent_pending_bitmap_nonzero = atomic_load_explicit(
      &h8g.quiescent_pending_bitmap_nonzero, memory_order_acquire);
  out->quiescent_pending_repair =
      atomic_load_explicit(&h8g.quiescent_pending_repair, memory_order_acquire);
  out->orphan_quiesce_count =
      atomic_load_explicit(&h8g.orphan_quiesce_count, memory_order_acquire);
  out->orphan_ready_count =
      atomic_load_explicit(&h8g.orphan_ready_count, memory_order_acquire);
  out->adoption_dry_run_scan_count =
      atomic_load_explicit(&h8g.adoption_dry_run_scan_count, memory_order_acquire);
  out->adoption_dry_run_candidate_count =
      atomic_load_explicit(&h8g.adoption_dry_run_candidate_count, memory_order_acquire);
  out->adoption_dry_run_block_state_count =
      atomic_load_explicit(&h8g.adoption_dry_run_block_state_count, memory_order_acquire);
  out->adoption_dry_run_block_quiesce_count =
      atomic_load_explicit(&h8g.adoption_dry_run_block_quiesce_count, memory_order_acquire);
  out->adoption_dry_run_empty_count =
      atomic_load_explicit(&h8g.adoption_dry_run_empty_count, memory_order_acquire);
  out->adoption_dry_run_target_closed_count =
      atomic_load_explicit(&h8g.adoption_dry_run_target_closed_count, memory_order_acquire);
  out->adoption_dry_run_would_adopt_count =
      atomic_load_explicit(&h8g.adoption_dry_run_would_adopt_count, memory_order_acquire);
  out->handoff_fail_count = atomic_load_explicit(&h8g.handoff_fail_count, memory_order_acquire);
  out->invalid_count = atomic_load_explicit(&h8g.invalid_count, memory_order_acquire);
  out->miss_count = atomic_load_explicit(&h8g.miss_count, memory_order_acquire);
  out->owner_transition_count =
      atomic_load_explicit(&h8g.owner_transition_count, memory_order_acquire);
  out->adoption_scan_count =
      atomic_load_explicit(&h8g.adoption_scan_count, memory_order_acquire);
  out->adoption_candidate_count =
      atomic_load_explicit(&h8g.adoption_candidate_count, memory_order_acquire);
  out->adoption_block_state_count =
      atomic_load_explicit(&h8g.adoption_block_state_count, memory_order_acquire);
  out->adoption_block_quiesce_count =
      atomic_load_explicit(&h8g.adoption_block_quiesce_count, memory_order_acquire);
  out->adoption_empty_count =
      atomic_load_explicit(&h8g.adoption_empty_count, memory_order_acquire);
  out->adoption_target_closed_count =
      atomic_load_explicit(&h8g.adoption_target_closed_count, memory_order_acquire);
  out->adoption_success_count =
      atomic_load_explicit(&h8g.adoption_success_count, memory_order_acquire);
