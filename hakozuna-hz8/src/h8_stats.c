#include "h8_internal.h"

#include <string.h>

void h8_stats_snapshot(H8Stats* out) {
  size_t small_span_count = 0;
  for (size_t i = 0; i < h8g.span_count; ++i) {
    if (h8g.spans &&
        atomic_load_explicit(&h8g.spans[i], memory_order_acquire)) {
      ++small_span_count;
    }
  }
  out->arena_reserved_bytes = h8g.arena_bytes;
  out->arena_committed_bytes =
      atomic_load_explicit(&h8g.arena_committed_bytes, memory_order_acquire);
  out->small_span_count = small_span_count;
  out->owner_count = atomic_load_explicit(&h8g.owner_count, memory_order_acquire);
  out->orphan_span_count = atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire);
  out->local_alloc_count = atomic_load_explicit(&h8g.local_alloc_count, memory_order_acquire);
  out->local_free_count = atomic_load_explicit(&h8g.local_free_count, memory_order_acquire);
  out->remote_publish_count = atomic_load_explicit(&h8g.remote_publish_count, memory_order_acquire);
  out->remote_collect_count = atomic_load_explicit(&h8g.remote_collect_count, memory_order_acquire);
  out->owner_exit_count = atomic_load_explicit(&h8g.owner_exit_count, memory_order_acquire);
  out->pending_enqueue_count =
      atomic_load_explicit(&h8g.pending_enqueue_count, memory_order_acquire);
  out->pending_dequeue_count =
      atomic_load_explicit(&h8g.pending_dequeue_count, memory_order_acquire);
  out->orphan_handoff_count =
      atomic_load_explicit(&h8g.orphan_handoff_count, memory_order_acquire);
  out->handoff_success_count =
      atomic_load_explicit(&h8g.handoff_success_count, memory_order_acquire);
}

H8Stats h8_stats(void) {
  H8Stats stats;
  memset(&stats, 0, sizeof(stats));
  h8_stats_snapshot(&stats);
  return stats;
}

void h8_debug_stats_snapshot(H8DebugStats* out) {
#if !defined(H8_ENABLE_DEBUG_STATS)
  memset(out, 0, sizeof(*out));
  return;
#else
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
  out->medium_run_create_count =
      atomic_load_explicit(&h8g.medium_run_create_count, memory_order_acquire);
  out->medium_run_reuse_active_count = atomic_load_explicit(
      &h8g.medium_run_reuse_active_count, memory_order_acquire);
  out->medium_run_reuse_owner_list_count = atomic_load_explicit(
      &h8g.medium_run_reuse_owner_list_count, memory_order_acquire);
  out->medium_run_reuse_global_count = atomic_load_explicit(
      &h8g.medium_run_reuse_global_count, memory_order_acquire);
  out->medium_run_madvise_count =
      atomic_load_explicit(&h8g.medium_run_madvise_count, memory_order_acquire);
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
  out->medium_remote_collect_ns = atomic_load_explicit(
      &h8g.medium_remote_collect_ns, memory_order_acquire);
  out->medium_collect_finish_pending_rearm = atomic_load_explicit(
      &h8g.medium_collect_finish_pending_rearm, memory_order_acquire);
  out->medium_empty_with_pending = atomic_load_explicit(
      &h8g.medium_empty_with_pending, memory_order_acquire);
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
#endif
}

H8DebugStats h8_debug_stats(void) {
  H8DebugStats stats;
  memset(&stats, 0, sizeof(stats));
  h8_debug_stats_snapshot(&stats);
  return stats;
}
