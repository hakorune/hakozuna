  out->owner_lifecycle_enter_count =
      atomic_load_explicit(&h8g.owner_lifecycle_enter_count, memory_order_acquire);
  out->owner_lifecycle_exit_count =
      atomic_load_explicit(&h8g.owner_lifecycle_exit_count, memory_order_acquire);
  out->owner_publish_enter_count =
      atomic_load_explicit(&h8g.owner_publish_enter_count, memory_order_acquire);
  out->owner_publish_exit_count =
      atomic_load_explicit(&h8g.owner_publish_exit_count, memory_order_acquire);
  out->span_publish_enter_count =
      atomic_load_explicit(&h8g.span_publish_enter_count, memory_order_acquire);
  out->span_publish_exit_count =
      atomic_load_explicit(&h8g.span_publish_exit_count, memory_order_acquire);
  out->remote_regular_admission_count =
      atomic_load_explicit(&h8g.remote_regular_admission_count, memory_order_acquire);
  out->remote_orphan_admission_count =
      atomic_load_explicit(&h8g.remote_orphan_admission_count, memory_order_acquire);
  out->remote_stage_enter =
      atomic_load_explicit(&h8g.remote_stage_enter, memory_order_acquire);
  out->remote_stage_span_miss =
      atomic_load_explicit(&h8g.remote_stage_span_miss, memory_order_acquire);
  out->remote_stage_owner_missing =
      atomic_load_explicit(&h8g.remote_stage_owner_missing, memory_order_acquire);
  out->remote_stage_regular_lease_ok =
      atomic_load_explicit(&h8g.remote_stage_regular_lease_ok, memory_order_acquire);
  out->remote_stage_regular_lease_fail = atomic_load_explicit(
      &h8g.remote_stage_regular_lease_fail, memory_order_acquire);
  out->remote_stage_regular_lease_elided = atomic_load_explicit(
      &h8g.remote_stage_regular_lease_elided, memory_order_acquire);
  out->remote_stage_orphan_lease_ok =
      atomic_load_explicit(&h8g.remote_stage_orphan_lease_ok, memory_order_acquire);
  out->remote_stage_orphan_lease_fail = atomic_load_explicit(
      &h8g.remote_stage_orphan_lease_fail, memory_order_acquire);
  out->remote_stage_pending_claim_ok =
      atomic_load_explicit(&h8g.remote_stage_pending_claim_ok, memory_order_acquire);
  out->remote_stage_validate_fail =
      atomic_load_explicit(&h8g.remote_stage_validate_fail, memory_order_acquire);
  out->remote_stage_notify_first =
      atomic_load_explicit(&h8g.remote_stage_notify_first, memory_order_acquire);
  out->remote_stage_publish_ok =
      atomic_load_explicit(&h8g.remote_stage_publish_ok, memory_order_acquire);
  out->remote_stage_pending_publish_elided = atomic_load_explicit(
      &h8g.remote_stage_pending_publish_elided, memory_order_acquire);
  out->remote_lookup_enter =
      atomic_load_explicit(&h8g.remote_lookup_enter, memory_order_acquire);
  out->remote_lookup_arena_miss =
      atomic_load_explicit(&h8g.remote_lookup_arena_miss, memory_order_acquire);
  out->remote_lookup_span_miss =
      atomic_load_explicit(&h8g.remote_lookup_span_miss, memory_order_acquire);
  out->remote_lookup_retired =
      atomic_load_explicit(&h8g.remote_lookup_retired, memory_order_acquire);
  out->remote_lookup_slot_oob =
      atomic_load_explicit(&h8g.remote_lookup_slot_oob, memory_order_acquire);
  out->remote_lookup_ok =
      atomic_load_explicit(&h8g.remote_lookup_ok, memory_order_acquire);
  out->remote_owner_word_load =
      atomic_load_explicit(&h8g.remote_owner_word_load, memory_order_acquire);
  out->pending_notify_count =
      atomic_load_explicit(&h8g.pending_notify_count, memory_order_acquire);
  out->qstate_notify_attempt_count =
      atomic_load_explicit(&h8g.qstate_notify_attempt_count, memory_order_acquire);
  out->qstate_notify_success_count =
      atomic_load_explicit(&h8g.qstate_notify_success_count, memory_order_acquire);
  out->qstate_notify_skip_nonidle_count = atomic_load_explicit(
      &h8g.qstate_notify_skip_nonidle_count, memory_order_acquire);
  out->pending_head_push_attempt_count = atomic_load_explicit(
      &h8g.pending_head_push_attempt_count, memory_order_acquire);
  out->pending_head_push_retry_count =
      atomic_load_explicit(&h8g.pending_head_push_retry_count, memory_order_acquire);
  out->pending_head_push_success_count = atomic_load_explicit(
      &h8g.pending_head_push_success_count, memory_order_acquire);
  out->owner_lifecycle_enter_cas_retry_count = atomic_load_explicit(
      &h8g.owner_lifecycle_enter_cas_retry_count, memory_order_acquire);
  out->owner_lifecycle_exit_cas_retry_count = atomic_load_explicit(
      &h8g.owner_lifecycle_exit_cas_retry_count, memory_order_acquire);
  out->remote_publish_pending_claim_duplicate_count = atomic_load_explicit(
      &h8g.remote_publish_pending_claim_duplicate_count, memory_order_acquire);
  out->pending_collect_call_count =
      atomic_load_explicit(&h8g.pending_collect_call_count, memory_order_acquire);
  out->pending_collect_carry_hit_count = atomic_load_explicit(
      &h8g.pending_collect_carry_hit_count, memory_order_acquire);
  out->pending_collect_requeue_count =
      atomic_load_explicit(&h8g.pending_collect_requeue_count, memory_order_acquire);
  out->pending_word_summary_set =
      atomic_load_explicit(&h8g.pending_word_summary_set, memory_order_acquire);
  out->pending_word_summary_shadow_hit = atomic_load_explicit(
      &h8g.pending_word_summary_shadow_hit, memory_order_acquire);
  out->pending_word_summary_false_positive = atomic_load_explicit(
      &h8g.pending_word_summary_false_positive, memory_order_acquire);
  out->pending_word_summary_false_negative = atomic_load_explicit(
      &h8g.pending_word_summary_false_negative, memory_order_acquire);
  out->pending_word_summary_rearm =
      atomic_load_explicit(&h8g.pending_word_summary_rearm, memory_order_acquire);
  out->pending_word_drain_count =
      atomic_load_explicit(&h8g.pending_word_drain_count, memory_order_acquire);
  out->pending_word_popcount_1 =
      atomic_load_explicit(&h8g.pending_word_popcount_1, memory_order_acquire);
  out->pending_word_popcount_2 =
      atomic_load_explicit(&h8g.pending_word_popcount_2, memory_order_acquire);
  out->pending_word_popcount_3_4 =
      atomic_load_explicit(&h8g.pending_word_popcount_3_4, memory_order_acquire);
  out->pending_word_popcount_5_8 =
      atomic_load_explicit(&h8g.pending_word_popcount_5_8, memory_order_acquire);
  out->pending_word_popcount_9_16 =
      atomic_load_explicit(&h8g.pending_word_popcount_9_16, memory_order_acquire);
  out->pending_word_popcount_17_plus =
      atomic_load_explicit(&h8g.pending_word_popcount_17_plus, memory_order_acquire);
  out->pending_slots_drained =
      atomic_load_explicit(&h8g.pending_slots_drained, memory_order_acquire);
  out->pending_words_rearmed =
      atomic_load_explicit(&h8g.pending_words_rearmed, memory_order_acquire);
  out->pending_word_new_publish_during_drain = atomic_load_explicit(
      &h8g.pending_word_new_publish_during_drain, memory_order_acquire);
  out->local_alloc_pending_nonzero =
      atomic_load_explicit(&h8g.local_alloc_pending_nonzero, memory_order_acquire);
  out->local_free_pending_nonzero =
      atomic_load_explicit(&h8g.local_free_pending_nonzero, memory_order_acquire);
  out->owner_live_set_already_live =
      atomic_load_explicit(&h8g.owner_live_set_already_live, memory_order_acquire);
  out->owner_live_clear_already_free =
      atomic_load_explicit(&h8g.owner_live_clear_already_free, memory_order_acquire);
  out->local_active_hit =
      atomic_load_explicit(&h8g.local_active_hit, memory_order_acquire);
  out->local_active_miss =
      atomic_load_explicit(&h8g.local_active_miss, memory_order_acquire);
  out->local_freelist_pop =
      atomic_load_explicit(&h8g.local_freelist_pop, memory_order_acquire);
  out->local_bump_alloc =
      atomic_load_explicit(&h8g.local_bump_alloc, memory_order_acquire);
  out->local_slow_collect =
      atomic_load_explicit(&h8g.local_slow_collect, memory_order_acquire);
  out->local_span_commit =
      atomic_load_explicit(&h8g.local_span_commit, memory_order_acquire);
  out->local_find_scan =
      atomic_load_explicit(&h8g.local_find_scan, memory_order_acquire);
  out->local_find_scan_span =
      atomic_load_explicit(&h8g.local_find_scan_span, memory_order_acquire);
  out->local_active_hint_null =
      atomic_load_explicit(&h8g.local_active_hint_null, memory_order_acquire);
  out->local_active_hint_full =
      atomic_load_explicit(&h8g.local_active_hint_full, memory_order_acquire);
  out->local_active_hint_state_blocked = atomic_load_explicit(
      &h8g.local_active_hint_state_blocked, memory_order_acquire);
  out->local_active_hint_trusted =
      atomic_load_explicit(&h8g.local_active_hint_trusted, memory_order_acquire);
  out->local_active_hint_class_mismatch = atomic_load_explicit(
      &h8g.local_active_hint_class_mismatch, memory_order_acquire);
  out->local_active_hint_owner_mismatch = atomic_load_explicit(
      &h8g.local_active_hint_owner_mismatch, memory_order_acquire);
  out->local_active_hint_generation_mismatch = atomic_load_explicit(
      &h8g.local_active_hint_generation_mismatch, memory_order_acquire);
  out->local_active_hint_state_mismatch = atomic_load_explicit(
      &h8g.local_active_hint_state_mismatch, memory_order_acquire);
  out->local_find_scan_span_usable = atomic_load_explicit(
      &h8g.local_find_scan_span_usable, memory_order_acquire);
  out->local_find_scan_span_full =
      atomic_load_explicit(&h8g.local_find_scan_span_full, memory_order_acquire);
  out->local_find_scan_span_state_blocked = atomic_load_explicit(
      &h8g.local_find_scan_span_state_blocked, memory_order_acquire);
  out->local_find_skip_scan_no_pending = atomic_load_explicit(
      &h8g.local_find_skip_scan_no_pending, memory_order_acquire);
  out->local_free_hit =
      atomic_load_explicit(&h8g.local_free_hit, memory_order_acquire);
  out->local_free_reject_owner =
      atomic_load_explicit(&h8g.local_free_reject_owner, memory_order_acquire);
  out->local_free_reject_state =
      atomic_load_explicit(&h8g.local_free_reject_state, memory_order_acquire);
  out->local_free_reject_live =
      atomic_load_explicit(&h8g.local_free_reject_live, memory_order_acquire);
  out->local_live_touch_alloc =
      atomic_load_explicit(&h8g.local_live_touch_alloc, memory_order_acquire);
  out->local_live_touch_free =
      atomic_load_explicit(&h8g.local_live_touch_free, memory_order_acquire);
  out->local_live_word_0 =
      atomic_load_explicit(&h8g.local_live_word_0, memory_order_acquire);
  out->local_live_word_1 =
      atomic_load_explicit(&h8g.local_live_word_1, memory_order_acquire);
  out->local_live_word_2_7 =
      atomic_load_explicit(&h8g.local_live_word_2_7, memory_order_acquire);
  out->local_live_word_8_31 =
      atomic_load_explicit(&h8g.local_live_word_8_31, memory_order_acquire);
  out->local_live_word_32_63 =
      atomic_load_explicit(&h8g.local_live_word_32_63, memory_order_acquire);
  out->local_free_head_touch_alloc = atomic_load_explicit(
      &h8g.local_free_head_touch_alloc, memory_order_acquire);
  out->local_free_head_touch_free = atomic_load_explicit(
      &h8g.local_free_head_touch_free, memory_order_acquire);
  out->local_pending_check_alloc =
      atomic_load_explicit(&h8g.local_pending_check_alloc, memory_order_acquire);
  out->local_pending_check_free =
      atomic_load_explicit(&h8g.local_pending_check_free, memory_order_acquire);
  out->local_used_touch_alloc =
      atomic_load_explicit(&h8g.local_used_touch_alloc, memory_order_acquire);
  out->local_used_touch_free =
      atomic_load_explicit(&h8g.local_used_touch_free, memory_order_acquire);
  out->local_used_count_load_alloc = atomic_load_explicit(
      &h8g.local_used_count_load_alloc, memory_order_acquire);
  out->local_used_count_store_alloc = atomic_load_explicit(
      &h8g.local_used_count_store_alloc, memory_order_acquire);
  out->local_used_count_load_free =
      atomic_load_explicit(&h8g.local_used_count_load_free, memory_order_acquire);
  out->local_used_count_store_free = atomic_load_explicit(
      &h8g.local_used_count_store_free, memory_order_acquire);
  out->local_used_count_full_check = atomic_load_explicit(
      &h8g.local_used_count_full_check, memory_order_acquire);
  out->local_used_count_underflow = atomic_load_explicit(
      &h8g.local_used_count_underflow, memory_order_acquire);
  out->local_used_mirror_mismatch = atomic_load_explicit(
      &h8g.local_used_mirror_mismatch, memory_order_acquire);
  out->local_used_mirror_underflow = atomic_load_explicit(
      &h8g.local_used_mirror_underflow, memory_order_acquire);
  out->local_used_cold_active_hint = atomic_load_explicit(
      &h8g.local_used_cold_active_hint, memory_order_acquire);
  out->local_used_cold_owner_scan_locked = atomic_load_explicit(
      &h8g.local_used_cold_owner_scan_locked, memory_order_acquire);
  out->local_used_cold_adoption_locked = atomic_load_explicit(
      &h8g.local_used_cold_adoption_locked, memory_order_acquire);
  out->local_used_cold_owner_exit = atomic_load_explicit(
      &h8g.local_used_cold_owner_exit, memory_order_acquire);
  out->local_used_cold_verify_quiescent = atomic_load_explicit(
      &h8g.local_used_cold_verify_quiescent, memory_order_acquire);
  out->local_used_derived_mismatch = atomic_load_explicit(
      &h8g.local_used_derived_mismatch, memory_order_acquire);
  out->local_used_derived_quiescent_scan = atomic_load_explicit(
      &h8g.local_used_derived_quiescent_scan, memory_order_acquire);
  out->span_commit_total_ns =
      atomic_load_explicit(&h8g.span_commit_total_ns, memory_order_acquire);
  out->span_commit_meta_ns =
      atomic_load_explicit(&h8g.span_commit_meta_ns, memory_order_acquire);
  out->span_commit_mprotect_ns =
      atomic_load_explicit(&h8g.span_commit_mprotect_ns, memory_order_acquire);
  out->owner_exit_total_ns =
      atomic_load_explicit(&h8g.owner_exit_total_ns, memory_order_acquire);
  out->owner_exit_collect_ns =
      atomic_load_explicit(&h8g.owner_exit_collect_ns, memory_order_acquire);
  out->owner_exit_span_walk_ns =
      atomic_load_explicit(&h8g.owner_exit_span_walk_ns, memory_order_acquire);
  out->span_retire_count =
      atomic_load_explicit(&h8g.span_retire_count, memory_order_acquire);
  out->span_retire_total_ns =
      atomic_load_explicit(&h8g.span_retire_total_ns, memory_order_acquire);
  out->span_retire_lock_wait_ns =
      atomic_load_explicit(&h8g.span_retire_lock_wait_ns, memory_order_acquire);
  out->span_retire_madvise_ns =
      atomic_load_explicit(&h8g.span_retire_madvise_ns, memory_order_acquire);
  out->span_retire_meta_free_ns =
      atomic_load_explicit(&h8g.span_retire_meta_free_ns, memory_order_acquire);
  out->span_purge_run_count =
      atomic_load_explicit(&h8g.span_purge_run_count, memory_order_acquire);
  out->span_purge_run_spans_total =
      atomic_load_explicit(&h8g.span_purge_run_spans_total, memory_order_acquire);
  out->span_purge_run_max =
      atomic_load_explicit(&h8g.span_purge_run_max, memory_order_acquire);
  out->span_purge_singleton_runs =
      atomic_load_explicit(&h8g.span_purge_singleton_runs, memory_order_acquire);
  out->span_purge_madvise_calls =
      atomic_load_explicit(&h8g.span_purge_madvise_calls, memory_order_acquire);
  out->span_purge_madvise_bytes =
      atomic_load_explicit(&h8g.span_purge_madvise_bytes, memory_order_acquire);
  out->span_purge_madvise_ns =
      atomic_load_explicit(&h8g.span_purge_madvise_ns, memory_order_acquire);
  out->slot_shadow_valid_mismatch =
      atomic_load_explicit(&h8g.slot_shadow_valid_mismatch, memory_order_acquire);
  out->slot_shadow_invalid_mismatch =
      atomic_load_explicit(&h8g.slot_shadow_invalid_mismatch, memory_order_acquire);
  out->slot_shadow_pending_nonallocated = atomic_load_explicit(
      &h8g.slot_shadow_pending_nonallocated, memory_order_acquire);
  out->slot_shadow_free_unreachable =
      atomic_load_explicit(&h8g.slot_shadow_free_unreachable, memory_order_acquire);
  out->slot_shadow_free_duplicate =
      atomic_load_explicit(&h8g.slot_shadow_free_duplicate, memory_order_acquire);
  out->slot_shadow_free_cycle =
      atomic_load_explicit(&h8g.slot_shadow_free_cycle, memory_order_acquire);
  out->slot_shadow_bad_next =
      atomic_load_explicit(&h8g.slot_shadow_bad_next, memory_order_acquire);
  out->slot_shadow_never_used_below_bump = atomic_load_explicit(
      &h8g.slot_shadow_never_used_below_bump, memory_order_acquire);
  out->slot_shadow_nonvirgin_above_bump = atomic_load_explicit(
      &h8g.slot_shadow_nonvirgin_above_bump, memory_order_acquire);
  out->slot_shadow_used_mismatch =
      atomic_load_explicit(&h8g.slot_shadow_used_mismatch, memory_order_acquire);
  out->slot_shadow_reserved_quiescent = atomic_load_explicit(
      &h8g.slot_shadow_reserved_quiescent, memory_order_acquire);
  out->medium_malloc_count =
      atomic_load_explicit(&h8g.medium_malloc_count, memory_order_acquire);
  out->medium_malloc_class_8k =
      atomic_load_explicit(&h8g.medium_malloc_class_8k, memory_order_acquire);
  out->medium_malloc_class_16k =
      atomic_load_explicit(&h8g.medium_malloc_class_16k, memory_order_acquire);
  out->medium_malloc_class_32k =
      atomic_load_explicit(&h8g.medium_malloc_class_32k, memory_order_acquire);
  out->medium_malloc_class_64k =
      atomic_load_explicit(&h8g.medium_malloc_class_64k, memory_order_acquire);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  out->medium_malloc_class_24k =
      atomic_load_explicit(&h8g.medium_malloc_class_24k, memory_order_acquire);
  out->medium_malloc_class_48k =
      atomic_load_explicit(&h8g.medium_malloc_class_48k, memory_order_acquire);
#else
  out->medium_malloc_class_24k = 0;
  out->medium_malloc_class_48k = 0;
#endif
  out->medium_run_create_count =
      atomic_load_explicit(&h8g.medium_run_create_count, memory_order_acquire);
  out->medium_run_reuse_active_count = atomic_load_explicit(
      &h8g.medium_run_reuse_active_count, memory_order_acquire);
  out->medium_run_reuse_owner_list_count = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_list_count, memory_order_acquire);
  out->medium_run_reuse_global_count = atomic_load_explicit(
      &h8g.medium_run_reuse_global_count, memory_order_acquire);
  out->medium_run_reuse_refill_candidate_count = atomic_load_explicit(
      &h8g.medium_run_reuse_refill_candidate_count, memory_order_acquire);
  out->medium_active_miss_null =
      atomic_load_explicit(&h8g.medium_active_miss_null, memory_order_acquire);
  out->medium_active_miss_owner =
      atomic_load_explicit(&h8g.medium_active_miss_owner, memory_order_acquire);
  out->medium_active_miss_unusable = atomic_load_explicit(
      &h8g.medium_active_miss_unusable, memory_order_acquire);
  out->medium_collect_periodic_fast_skip = atomic_load_explicit(
      &h8g.medium_collect_periodic_fast_skip, memory_order_acquire);
  out->medium_collect_periodic_slow_enter = atomic_load_explicit(
      &h8g.medium_collect_periodic_slow_enter, memory_order_acquire);
  out->medium_collect_periodic_pending_hit = atomic_load_explicit(
      &h8g.medium_collect_periodic_pending_hit, memory_order_acquire);
  out->medium_collect_periodic_pending_miss = atomic_load_explicit(
      &h8g.medium_collect_periodic_pending_miss, memory_order_acquire);
  out->medium_collect_periodic_from_active = atomic_load_explicit(
      &h8g.medium_collect_periodic_from_active, memory_order_acquire);
  out->medium_collect_periodic_from_owner_list = atomic_load_explicit(
      &h8g.medium_collect_periodic_from_owner_list, memory_order_acquire);
  out->medium_collect_active_refill_hint = atomic_load_explicit(
      &h8g.medium_collect_active_refill_hint, memory_order_acquire);
  out->medium_refill_candidate_install = atomic_load_explicit(
      &h8g.medium_refill_candidate_install, memory_order_acquire);
  out->medium_refill_candidate_attempt = atomic_load_explicit(
      &h8g.medium_refill_candidate_attempt, memory_order_acquire);
  out->medium_refill_candidate_hit = atomic_load_explicit(
      &h8g.medium_refill_candidate_hit, memory_order_acquire);
  out->medium_refill_candidate_owner_mismatch = atomic_load_explicit(
      &h8g.medium_refill_candidate_owner_mismatch, memory_order_acquire);
  out->medium_refill_candidate_unusable = atomic_load_explicit(
      &h8g.medium_refill_candidate_unusable, memory_order_acquire);
  out->medium_available_add =
      atomic_load_explicit(&h8g.medium_available_add, memory_order_acquire);
  out->medium_available_remove =
      atomic_load_explicit(&h8g.medium_available_remove, memory_order_acquire);
  out->medium_available_head_attempt = atomic_load_explicit(
      &h8g.medium_available_head_attempt, memory_order_acquire);
  out->medium_available_head_hit = atomic_load_explicit(
      &h8g.medium_available_head_hit, memory_order_acquire);
  out->medium_available_head_unusable = atomic_load_explicit(
      &h8g.medium_available_head_unusable, memory_order_acquire);
  out->medium_available_owner_list_hit_without_available =
      atomic_load_explicit(
          &h8g.medium_available_owner_list_hit_without_available,
          memory_order_acquire);
  out->medium_available_duplicate_membership = atomic_load_explicit(
      &h8g.medium_available_duplicate_membership, memory_order_acquire);
  out->medium_available_owner_mismatch = atomic_load_explicit(
      &h8g.medium_available_owner_mismatch, memory_order_acquire);
  out->medium_available_class_mismatch = atomic_load_explicit(
      &h8g.medium_available_class_mismatch, memory_order_acquire);
  out->medium_available_indexed_full = atomic_load_explicit(
      &h8g.medium_available_indexed_full, memory_order_acquire);
  out->medium_available_indexed_detached = atomic_load_explicit(
      &h8g.medium_available_indexed_detached, memory_order_acquire);
  out->medium_available_indexed_nonactive = atomic_load_explicit(
      &h8g.medium_available_indexed_nonactive, memory_order_acquire);
  out->medium_available_active_indexed = atomic_load_explicit(
      &h8g.medium_available_active_indexed, memory_order_acquire);
  out->medium_available_exit_nonempty = atomic_load_explicit(
      &h8g.medium_available_exit_nonempty, memory_order_acquire);
  out->medium_available_hit_reuse = atomic_load_explicit(
      &h8g.medium_available_hit_reuse, memory_order_acquire);
  out->medium_available_hit_lock_ns = atomic_load_explicit(
      &h8g.medium_available_hit_lock_ns, memory_order_acquire);
  out->medium_available_hit_alloc_ns = atomic_load_explicit(
      &h8g.medium_available_hit_alloc_ns, memory_order_acquire);
  out->medium_available_hit_active_ns = atomic_load_explicit(
      &h8g.medium_available_hit_active_ns, memory_order_acquire);
  out->medium_available_hit_collect_ns = atomic_load_explicit(
      &h8g.medium_available_hit_collect_ns, memory_order_acquire);
  out->medium_alloc_scaffold_count = atomic_load_explicit(
      &h8g.medium_alloc_scaffold_count, memory_order_acquire);
  out->medium_alloc_mark_live_ns = atomic_load_explicit(
      &h8g.medium_alloc_mark_live_ns, memory_order_acquire);
  out->medium_alloc_mask_ns = atomic_load_explicit(
      &h8g.medium_alloc_mask_ns, memory_order_acquire);
  out->medium_alloc_slot_store_ns = atomic_load_explicit(
      &h8g.medium_alloc_slot_store_ns, memory_order_acquire);
  out->medium_alloc_ptr_ns = atomic_load_explicit(
      &h8g.medium_alloc_ptr_ns, memory_order_acquire);
  out->medium_active_miss_total = atomic_load_explicit(
      &h8g.medium_active_miss_total, memory_order_acquire);
  out->medium_active_miss_no_pending = atomic_load_explicit(
      &h8g.medium_active_miss_no_pending, memory_order_acquire);
  out->medium_active_miss_owner_pending = atomic_load_explicit(
      &h8g.medium_active_miss_owner_pending, memory_order_acquire);
  out->medium_active_miss_active_pending = atomic_load_explicit(
      &h8g.medium_active_miss_active_pending, memory_order_acquire);
  out->medium_active_miss_active_pending_zero = atomic_load_explicit(
      &h8g.medium_active_miss_active_pending_zero, memory_order_acquire);
  out->medium_active_miss_active_pending_slots = atomic_load_explicit(
      &h8g.medium_active_miss_active_pending_slots, memory_order_acquire);
  out->medium_active_miss_owner_list_hit = atomic_load_explicit(
      &h8g.medium_active_miss_owner_list_hit, memory_order_acquire);
  out->medium_active_miss_owner_list_pos1 = atomic_load_explicit(
      &h8g.medium_active_miss_owner_list_pos1, memory_order_acquire);
  out->medium_active_miss_owner_list_pos2 = atomic_load_explicit(
      &h8g.medium_active_miss_owner_list_pos2, memory_order_acquire);
  out->medium_active_miss_owner_list_pos3 = atomic_load_explicit(
      &h8g.medium_active_miss_owner_list_pos3, memory_order_acquire);
  out->medium_active_miss_owner_list_pos4 = atomic_load_explicit(
      &h8g.medium_active_miss_owner_list_pos4, memory_order_acquire);
  out->medium_active_miss_owner_list_pos5p = atomic_load_explicit(
      &h8g.medium_active_miss_owner_list_pos5p, memory_order_acquire);
  out->medium_active_miss_create = atomic_load_explicit(
      &h8g.medium_active_miss_create, memory_order_acquire);
  out->medium_active_miss_detached_reuse = atomic_load_explicit(
      &h8g.medium_active_miss_detached_reuse, memory_order_acquire);
  out->medium_active_miss_class_8k = atomic_load_explicit(
      &h8g.medium_active_miss_class_8k, memory_order_acquire);
  out->medium_active_miss_class_16k = atomic_load_explicit(
      &h8g.medium_active_miss_class_16k, memory_order_acquire);
  out->medium_active_miss_class_32k = atomic_load_explicit(
      &h8g.medium_active_miss_class_32k, memory_order_acquire);
  out->medium_active_miss_class_64k = atomic_load_explicit(
      &h8g.medium_active_miss_class_64k, memory_order_acquire);
  out->medium_active_pending_class_8k = atomic_load_explicit(
      &h8g.medium_active_pending_class_8k, memory_order_acquire);
  out->medium_active_pending_class_16k = atomic_load_explicit(
      &h8g.medium_active_pending_class_16k, memory_order_acquire);
  out->medium_active_pending_class_32k = atomic_load_explicit(
      &h8g.medium_active_pending_class_32k, memory_order_acquire);
  out->medium_active_pending_class_64k = atomic_load_explicit(
      &h8g.medium_active_pending_class_64k, memory_order_acquire);
  out->medium_owner_list_hit_class_8k = atomic_load_explicit(
      &h8g.medium_owner_list_hit_class_8k, memory_order_acquire);
  out->medium_owner_list_hit_class_16k = atomic_load_explicit(
      &h8g.medium_owner_list_hit_class_16k, memory_order_acquire);
  out->medium_owner_list_hit_class_32k = atomic_load_explicit(
      &h8g.medium_owner_list_hit_class_32k, memory_order_acquire);
  out->medium_owner_list_hit_class_64k = atomic_load_explicit(
      &h8g.medium_owner_list_hit_class_64k, memory_order_acquire);
  out->medium_active_switch_class_8k = atomic_load_explicit(
      &h8g.medium_active_switch_class_8k, memory_order_acquire);
  out->medium_active_switch_class_16k = atomic_load_explicit(
      &h8g.medium_active_switch_class_16k, memory_order_acquire);
  out->medium_active_switch_class_32k = atomic_load_explicit(
      &h8g.medium_active_switch_class_32k, memory_order_acquire);
  out->medium_active_switch_class_64k = atomic_load_explicit(
      &h8g.medium_active_switch_class_64k, memory_order_acquire);
  out->medium_collect_src_periodic_active_call = atomic_load_explicit(
      &h8g.medium_collect_src_periodic_active_call, memory_order_acquire);
  out->medium_collect_src_periodic_active_run = atomic_load_explicit(
      &h8g.medium_collect_src_periodic_active_run, memory_order_acquire);
  out->medium_collect_src_periodic_active_slot = atomic_load_explicit(
      &h8g.medium_collect_src_periodic_active_slot, memory_order_acquire);
  out->medium_collect_src_periodic_owner_call = atomic_load_explicit(
      &h8g.medium_collect_src_periodic_owner_call, memory_order_acquire);
  out->medium_collect_src_periodic_owner_run = atomic_load_explicit(
      &h8g.medium_collect_src_periodic_owner_run, memory_order_acquire);
  out->medium_collect_src_periodic_owner_slot = atomic_load_explicit(
      &h8g.medium_collect_src_periodic_owner_slot, memory_order_acquire);
  out->medium_collect_src_capacity_call = atomic_load_explicit(
      &h8g.medium_collect_src_capacity_call, memory_order_acquire);
  out->medium_collect_src_capacity_run = atomic_load_explicit(
      &h8g.medium_collect_src_capacity_run, memory_order_acquire);
  out->medium_collect_src_capacity_slot = atomic_load_explicit(
      &h8g.medium_collect_src_capacity_slot, memory_order_acquire);
  out->medium_collect_src_owner_exit_call = atomic_load_explicit(
      &h8g.medium_collect_src_owner_exit_call, memory_order_acquire);
  out->medium_collect_src_owner_exit_run = atomic_load_explicit(
      &h8g.medium_collect_src_owner_exit_run, memory_order_acquire);
  out->medium_collect_src_owner_exit_slot = atomic_load_explicit(
      &h8g.medium_collect_src_owner_exit_slot, memory_order_acquire);
  out->medium_collect_src_demand_call = atomic_load_explicit(
      &h8g.medium_collect_src_demand_call, memory_order_acquire);
  out->medium_collect_src_demand_run = atomic_load_explicit(
      &h8g.medium_collect_src_demand_run, memory_order_acquire);
  out->medium_collect_src_demand_slot = atomic_load_explicit(
      &h8g.medium_collect_src_demand_slot, memory_order_acquire);
  out->medium_collect_full_to_nonfull_run = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_run, memory_order_acquire);
  out->medium_collect_full_to_nonfull_class_8k = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_class_8k, memory_order_acquire);
  out->medium_collect_full_to_nonfull_class_16k = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_class_16k, memory_order_acquire);
  out->medium_collect_full_to_nonfull_class_32k = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_class_32k, memory_order_acquire);
  out->medium_collect_full_to_nonfull_class_64k = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_class_64k, memory_order_acquire);
  out->medium_collect_full_to_nonfull_periodic_active = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_periodic_active,
      memory_order_acquire);
  out->medium_collect_full_to_nonfull_periodic_owner = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_periodic_owner,
      memory_order_acquire);
  out->medium_collect_full_to_nonfull_capacity = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_capacity, memory_order_acquire);
  out->medium_collect_full_to_nonfull_owner_exit = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_owner_exit, memory_order_acquire);
  out->medium_collect_full_to_nonfull_demand = atomic_load_explicit(
      &h8g.medium_collect_full_to_nonfull_demand, memory_order_acquire);
  out->medium_collect_zero_run_call = atomic_load_explicit(
      &h8g.medium_collect_zero_run_call, memory_order_acquire);
  out->medium_collect_zero_slot_run = atomic_load_explicit(
      &h8g.medium_collect_zero_slot_run, memory_order_acquire);
  out->medium_collect_active_run = atomic_load_explicit(
      &h8g.medium_collect_active_run, memory_order_acquire);
  out->medium_collect_nonactive_run = atomic_load_explicit(
      &h8g.medium_collect_nonactive_run, memory_order_acquire);
  out->medium_collect_credit_created = atomic_load_explicit(
      &h8g.medium_collect_credit_created, memory_order_acquire);
  out->medium_collect_credit_reused = atomic_load_explicit(
      &h8g.medium_collect_credit_reused, memory_order_acquire);
  out->medium_collect_credit_outstanding_at_next_collect =
      atomic_load_explicit(&h8g.medium_collect_credit_outstanding_at_next_collect,
                           memory_order_acquire);
  out->medium_collect_credit_reuse_d0_1 = atomic_load_explicit(
      &h8g.medium_collect_credit_reuse_d0_1, memory_order_acquire);
  out->medium_collect_credit_reuse_d2_3 = atomic_load_explicit(
      &h8g.medium_collect_credit_reuse_d2_3, memory_order_acquire);
  out->medium_collect_credit_reuse_d4_7 = atomic_load_explicit(
      &h8g.medium_collect_credit_reuse_d4_7, memory_order_acquire);
  out->medium_collect_credit_reuse_d8_31 = atomic_load_explicit(
      &h8g.medium_collect_credit_reuse_d8_31, memory_order_acquire);
  out->medium_collect_credit_reuse_d32p = atomic_load_explicit(
      &h8g.medium_collect_credit_reuse_d32p, memory_order_acquire);
  out->medium_collect_credit_discarded = atomic_load_explicit(
      &h8g.medium_collect_credit_discarded, memory_order_acquire);
  out->medium_collect_credit_underflow = atomic_load_explicit(
      &h8g.medium_collect_credit_underflow, memory_order_acquire);
  out->medium_collect_credit_reused_class_8k = atomic_load_explicit(
      &h8g.medium_collect_credit_reused_class_8k, memory_order_acquire);
  out->medium_collect_credit_reused_class_16k = atomic_load_explicit(
      &h8g.medium_collect_credit_reused_class_16k, memory_order_acquire);
  out->medium_collect_credit_reused_class_32k = atomic_load_explicit(
      &h8g.medium_collect_credit_reused_class_32k, memory_order_acquire);
  out->medium_collect_credit_reused_class_64k = atomic_load_explicit(
      &h8g.medium_collect_credit_reused_class_64k, memory_order_acquire);
  out->medium_collect_credit_quick_class_8k = atomic_load_explicit(
      &h8g.medium_collect_credit_quick_class_8k, memory_order_acquire);
  out->medium_collect_credit_quick_class_16k = atomic_load_explicit(
      &h8g.medium_collect_credit_quick_class_16k, memory_order_acquire);
  out->medium_collect_credit_quick_class_32k = atomic_load_explicit(
      &h8g.medium_collect_credit_quick_class_32k, memory_order_acquire);
  out->medium_collect_credit_quick_class_64k = atomic_load_explicit(
      &h8g.medium_collect_credit_quick_class_64k, memory_order_acquire);
  out->medium_demand64_trigger = atomic_load_explicit(
      &h8g.medium_demand64_trigger, memory_order_acquire);
  out->medium_demand64_qstate_not_queued = atomic_load_explicit(
      &h8g.medium_demand64_qstate_not_queued, memory_order_acquire);
  out->medium_demand64_processed_0 = atomic_load_explicit(
      &h8g.medium_demand64_processed_0, memory_order_acquire);
  out->medium_demand64_processed_1 = atomic_load_explicit(
      &h8g.medium_demand64_processed_1, memory_order_acquire);
  out->medium_demand64_processed_2 = atomic_load_explicit(
      &h8g.medium_demand64_processed_2, memory_order_acquire);
  out->medium_demand64_target_opened = atomic_load_explicit(
      &h8g.medium_demand64_target_opened, memory_order_acquire);
  out->medium_demand64_retry_hit = atomic_load_explicit(
      &h8g.medium_demand64_retry_hit, memory_order_acquire);
  out->medium_demand64_target_not_reached = atomic_load_explicit(
      &h8g.medium_demand64_target_not_reached, memory_order_acquire);
  out->medium_demand64_owner_list_fallback = atomic_load_explicit(
      &h8g.medium_demand64_owner_list_fallback, memory_order_acquire);
  out->medium_demand64_active_slots_created = atomic_load_explicit(
      &h8g.medium_demand64_active_slots_created, memory_order_acquire);
  out->medium_demand64_nonactive_runs_processed = atomic_load_explicit(
      &h8g.medium_demand64_nonactive_runs_processed, memory_order_acquire);
  out->medium_demand64_periodic_tick_replaced = atomic_load_explicit(
      &h8g.medium_demand64_periodic_tick_replaced, memory_order_acquire);
  out->medium_run_reuse_active_class_8k = atomic_load_explicit(
      &h8g.medium_run_reuse_active_class_8k, memory_order_acquire);
  out->medium_run_reuse_active_class_16k = atomic_load_explicit(
      &h8g.medium_run_reuse_active_class_16k, memory_order_acquire);
  out->medium_run_reuse_active_class_32k = atomic_load_explicit(
      &h8g.medium_run_reuse_active_class_32k, memory_order_acquire);
  out->medium_run_reuse_active_class_64k = atomic_load_explicit(
      &h8g.medium_run_reuse_active_class_64k, memory_order_acquire);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  out->medium_run_reuse_active_class_24k = atomic_load_explicit(
      &h8g.medium_run_reuse_active_class_24k, memory_order_acquire);
  out->medium_run_reuse_active_class_48k = atomic_load_explicit(
      &h8g.medium_run_reuse_active_class_48k, memory_order_acquire);
#else
  out->medium_run_reuse_active_class_24k = 0;
  out->medium_run_reuse_active_class_48k = 0;
#endif
  out->medium_run_reuse_owner_class_8k = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_class_8k, memory_order_acquire);
  out->medium_run_reuse_owner_class_16k = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_class_16k, memory_order_acquire);
  out->medium_run_reuse_owner_class_32k = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_class_32k, memory_order_acquire);
  out->medium_run_reuse_owner_class_64k = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_class_64k, memory_order_acquire);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  out->medium_run_reuse_owner_class_24k = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_class_24k, memory_order_acquire);
  out->medium_run_reuse_owner_class_48k = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_class_48k, memory_order_acquire);
#else
  out->medium_run_reuse_owner_class_24k = 0;
  out->medium_run_reuse_owner_class_48k = 0;
#endif
  out->medium_run_reuse_global_class_8k = atomic_load_explicit(
      &h8g.medium_run_reuse_global_class_8k, memory_order_acquire);
  out->medium_run_reuse_global_class_16k = atomic_load_explicit(
      &h8g.medium_run_reuse_global_class_16k, memory_order_acquire);
  out->medium_run_reuse_global_class_32k = atomic_load_explicit(
      &h8g.medium_run_reuse_global_class_32k, memory_order_acquire);
  out->medium_run_reuse_global_class_64k = atomic_load_explicit(
      &h8g.medium_run_reuse_global_class_64k, memory_order_acquire);
  out->medium_run_create_class_8k = atomic_load_explicit(
      &h8g.medium_run_create_class_8k, memory_order_acquire);
  out->medium_run_create_class_16k = atomic_load_explicit(
      &h8g.medium_run_create_class_16k, memory_order_acquire);
