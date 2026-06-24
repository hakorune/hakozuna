#ifndef H8_H
#define H8_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum H8RouteKind {
  H8_ROUTE_MISS = 0,
  H8_ROUTE_VALID = 1,
  H8_ROUTE_INVALID = 2
} H8RouteKind;

typedef enum H8PublishResult {
  H8_PUBLISH_OK = 0,
  H8_PUBLISH_MISS = 1,
  H8_PUBLISH_INVALID = 2,
  H8_PUBLISH_DOUBLE_FREE = 3,
  H8_PUBLISH_OWNER_TRANSITION = 4,
  H8_PUBLISH_RETRY = 5
} H8PublishResult;

typedef struct H8Stats {
  size_t arena_reserved_bytes;
  size_t arena_committed_bytes;
  size_t small_span_count;
  size_t owner_count;
  size_t orphan_span_count;
  size_t local_alloc_count;
  size_t local_free_count;
  size_t remote_publish_count;
  size_t remote_collect_count;
  size_t owner_exit_count;
  size_t pending_enqueue_count;
  size_t pending_dequeue_count;
  size_t orphan_handoff_count;
  size_t handoff_success_count;
} H8Stats;

typedef struct H8DebugStats {
  size_t owner_publish_enter_count;
  size_t owner_publish_exit_count;
  size_t owner_lifecycle_enter_count;
  size_t owner_lifecycle_exit_count;
  size_t span_publish_enter_count;
  size_t span_publish_exit_count;
  size_t remote_regular_admission_count;
  size_t remote_orphan_admission_count;
  size_t remote_stage_enter;
  size_t remote_stage_span_miss;
  size_t remote_stage_owner_missing;
  size_t remote_stage_regular_lease_ok;
  size_t remote_stage_regular_lease_fail;
  size_t remote_stage_regular_lease_elided;
  size_t remote_stage_orphan_lease_ok;
  size_t remote_stage_orphan_lease_fail;
  size_t remote_stage_pending_claim_ok;
  size_t remote_stage_validate_fail;
  size_t remote_stage_notify_first;
  size_t remote_stage_publish_ok;
  size_t remote_stage_pending_publish_elided;
  size_t remote_lookup_enter;
  size_t remote_lookup_arena_miss;
  size_t remote_lookup_span_miss;
  size_t remote_lookup_retired;
  size_t remote_lookup_slot_oob;
  size_t remote_lookup_ok;
  size_t remote_owner_word_load;
  size_t pending_notify_count;
  size_t qstate_notify_attempt_count;
  size_t qstate_notify_success_count;
  size_t qstate_notify_skip_nonidle_count;
  size_t pending_head_push_attempt_count;
  size_t pending_head_push_retry_count;
  size_t pending_head_push_success_count;
  size_t owner_lifecycle_enter_cas_retry_count;
  size_t owner_lifecycle_exit_cas_retry_count;
  size_t remote_publish_pending_claim_duplicate_count;
  size_t pending_collect_call_count;
  size_t pending_collect_carry_hit_count;
  size_t pending_collect_requeue_count;
  size_t pending_word_summary_set;
  size_t pending_word_summary_shadow_hit;
  size_t pending_word_summary_false_positive;
  size_t pending_word_summary_false_negative;
  size_t pending_word_summary_rearm;
  size_t pending_word_drain_count;
  size_t pending_word_popcount_1;
  size_t pending_word_popcount_2;
  size_t pending_word_popcount_3_4;
  size_t pending_word_popcount_5_8;
  size_t pending_word_popcount_9_16;
  size_t pending_word_popcount_17_plus;
  size_t pending_slots_drained;
  size_t pending_words_rearmed;
  size_t pending_word_new_publish_during_drain;
  size_t pending_mask_notify_without_count;
  size_t pending_count_notify_without_mask;
  size_t pending_mask_requeue_without_count;
  size_t pending_count_requeue_without_mask;
  size_t pending_finish_count_mask_zero_bitmap_zero;
  size_t pending_finish_count_mask_zero_bitmap_nonzero;
  size_t pending_finish_mask_nonzero_bitmap_zero;
  size_t pending_finish_mask_nonzero_bitmap_nonzero;
  size_t qstate_dirty_set;
  size_t qstate_dirty_self_set;
  size_t qstate_dirty_requeue;
  size_t quiescent_pending_bitmap_nonzero;
  size_t quiescent_pending_repair;
  size_t local_alloc_pending_nonzero;
  size_t local_free_pending_nonzero;
  size_t owner_live_set_already_live;
  size_t owner_live_clear_already_free;
  size_t local_active_hit;
  size_t local_active_miss;
  size_t local_freelist_pop;
  size_t local_bump_alloc;
  size_t local_slow_collect;
  size_t local_span_commit;
  size_t local_find_scan;
  size_t local_find_scan_span;
  size_t local_active_hint_null;
  size_t local_active_hint_full;
  size_t local_active_hint_state_blocked;
  size_t local_active_hint_trusted;
  size_t local_active_hint_class_mismatch;
  size_t local_active_hint_owner_mismatch;
  size_t local_active_hint_generation_mismatch;
  size_t local_active_hint_state_mismatch;
  size_t local_find_scan_span_usable;
  size_t local_find_scan_span_full;
  size_t local_find_scan_span_state_blocked;
  size_t local_find_skip_scan_no_pending;
  size_t local_free_hit;
  size_t local_free_reject_owner;
  size_t local_free_reject_state;
  size_t local_free_reject_live;
  size_t local_live_touch_alloc;
  size_t local_live_touch_free;
  size_t local_live_word_0;
  size_t local_live_word_1;
  size_t local_live_word_2_7;
  size_t local_live_word_8_31;
  size_t local_live_word_32_63;
  size_t local_free_head_touch_alloc;
  size_t local_free_head_touch_free;
  size_t local_pending_check_alloc;
  size_t local_pending_check_free;
  size_t local_used_touch_alloc;
  size_t local_used_touch_free;
  size_t local_used_count_load_alloc;
  size_t local_used_count_store_alloc;
  size_t local_used_count_load_free;
  size_t local_used_count_store_free;
  size_t local_used_count_full_check;
  size_t local_used_count_underflow;
  size_t local_used_mirror_mismatch;
  size_t local_used_mirror_underflow;
  size_t local_used_cold_active_hint;
  size_t local_used_cold_owner_scan_locked;
  size_t local_used_cold_adoption_locked;
  size_t local_used_cold_owner_exit;
  size_t local_used_cold_verify_quiescent;
  size_t local_used_derived_mismatch;
  size_t local_used_derived_quiescent_scan;
  size_t span_commit_total_ns;
  size_t span_commit_meta_ns;
  size_t span_commit_mprotect_ns;
  size_t owner_exit_total_ns;
  size_t owner_exit_collect_ns;
  size_t owner_exit_span_walk_ns;
  size_t span_retire_count;
  size_t span_retire_total_ns;
  size_t span_retire_lock_wait_ns;
  size_t span_retire_madvise_ns;
  size_t span_retire_meta_free_ns;
  size_t span_purge_run_count;
  size_t span_purge_run_spans_total;
  size_t span_purge_run_max;
  size_t span_purge_singleton_runs;
  size_t span_purge_madvise_calls;
  size_t span_purge_madvise_bytes;
  size_t span_purge_madvise_ns;
  size_t slot_shadow_valid_mismatch;
  size_t slot_shadow_invalid_mismatch;
  size_t slot_shadow_pending_nonallocated;
  size_t slot_shadow_free_unreachable;
  size_t slot_shadow_free_duplicate;
  size_t slot_shadow_free_cycle;
  size_t slot_shadow_bad_next;
  size_t slot_shadow_never_used_below_bump;
  size_t slot_shadow_nonvirgin_above_bump;
  size_t slot_shadow_used_mismatch;
  size_t slot_shadow_reserved_quiescent;
  size_t medium_malloc_count;
  size_t medium_run_create_count;
  size_t medium_run_reuse_active_count;
  size_t medium_run_reuse_owner_list_count;
  size_t medium_run_reuse_global_count;
  size_t medium_run_madvise_count;
  size_t medium_owner_scan_count;
  size_t medium_owner_scan_step_count;
  size_t medium_global_scan_count;
  size_t medium_global_scan_step_count;
  size_t medium_free_lookup_count;
  size_t medium_route_lookup_count;
  size_t medium_invalid_owned_count;
  size_t medium_empty_transition_count;
  size_t medium_empty_retain_count;
  size_t medium_empty_budget_reject_count;
  size_t medium_empty_reactivate_count;
  size_t medium_owner_exit_drain_count;
  size_t medium_madvise_fail_count;
  size_t medium_resident_empty_bytes;
  size_t medium_resident_empty_peak;
  size_t medium_madvise_ns;
  size_t medium_global_lock_wait_ns;
  size_t medium_run_lock_wait_ns;
  size_t medium_alloc_slot_ns;
  size_t medium_free_slot_ns;
  size_t medium_alloc_slot_count;
  size_t medium_free_slot_count;
  size_t medium_lock_elide_alloc_candidate;
  size_t medium_lock_elide_free_candidate;
  size_t medium_lock_elide_owner_mismatch;
  size_t medium_active_alloc_owner_mismatch;
  size_t medium_owner_list_owner_mismatch;
  size_t medium_global_skip_foreign_attached;
  size_t medium_local_free_owner_match;
  size_t medium_remote_free_owner_mismatch;
  size_t medium_free_lookup_step_count;
  size_t medium_route_lookup_step_count;
  size_t medium_route_authority_mismatch;
  size_t medium_remote_publish_count;
  size_t medium_remote_owner_lease_ns;
  size_t medium_remote_run_lock_ns;
  size_t medium_remote_pending_claim_count;
  size_t medium_remote_pending_claim_ns;
  size_t medium_remote_lockless_claim_count;
  size_t medium_remote_lockless_claim_collector_accept;
  size_t medium_remote_lockless_claim_rollback_invalid;
  size_t medium_remote_lockless_claim_rollback_accepted;
  size_t medium_remote_lockless_shadow_attempt;
  size_t medium_remote_lockless_shadow_would_accept;
  size_t medium_remote_lockless_shadow_would_reject;
  size_t medium_remote_lockless_shadow_match;
  size_t medium_remote_lockless_shadow_mismatch;
  size_t medium_remote_notify_count;
  size_t medium_remote_queue_push_count;
  size_t medium_remote_queue_push_ns;
  size_t medium_remote_collect_call_count;
  size_t medium_remote_collect_run_count;
  size_t medium_remote_collect_slot_count;
  size_t medium_remote_collect_ns;
  size_t medium_remote_publish_class_8k;
  size_t medium_remote_publish_class_16k;
  size_t medium_remote_publish_class_32k;
  size_t medium_remote_publish_class_64k;
  size_t medium_remote_qpush_class_8k;
  size_t medium_remote_qpush_class_16k;
  size_t medium_remote_qpush_class_32k;
  size_t medium_remote_qpush_class_64k;
  size_t medium_remote_collect_run_class_8k;
  size_t medium_remote_collect_run_class_16k;
  size_t medium_remote_collect_run_class_32k;
  size_t medium_remote_collect_run_class_64k;
  size_t medium_remote_collect_slot_class_8k;
  size_t medium_remote_collect_slot_class_16k;
  size_t medium_remote_collect_slot_class_32k;
  size_t medium_remote_collect_slot_class_64k;
  size_t pending_collect_word_count;
  size_t pending_collect_word_nonzero_count;
  size_t pending_collect_bit_count;
  size_t orphan_quiesce_count;
  size_t orphan_ready_count;
  size_t adoption_dry_run_scan_count;
  size_t adoption_dry_run_candidate_count;
  size_t adoption_dry_run_block_state_count;
  size_t adoption_dry_run_block_quiesce_count;
  size_t adoption_dry_run_empty_count;
  size_t adoption_dry_run_target_closed_count;
  size_t adoption_dry_run_would_adopt_count;
  size_t handoff_fail_count;
  size_t invalid_count;
  size_t miss_count;
  size_t owner_transition_count;
  size_t adoption_scan_count;
  size_t adoption_candidate_count;
  size_t adoption_block_state_count;
  size_t adoption_block_quiesce_count;
  size_t adoption_empty_count;
  size_t adoption_target_closed_count;
  size_t adoption_success_count;
} H8DebugStats;

void h8_init(void);
void h8_shutdown(void);
void* h8_malloc(size_t size);
void* h8_calloc(size_t count, size_t size);
void h8_free(void* ptr);
H8RouteKind h8_route(void* ptr);
H8Stats h8_stats(void);
H8DebugStats h8_debug_stats(void);

#ifdef __cplusplus
}
#endif

#endif
