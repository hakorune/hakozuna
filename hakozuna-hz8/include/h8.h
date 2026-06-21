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
  size_t pending_word_summary_repair;
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
