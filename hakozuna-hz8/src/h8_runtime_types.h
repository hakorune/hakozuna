#ifndef H8_RUNTIME_TYPES_H
#define H8_RUNTIME_TYPES_H

/* Included by h8_internal.h after core constants and medium/class types. */
typedef enum H8OwnerState {
  H8_OWNER_ALIVE = 0,
  H8_OWNER_DYING = 1,
  H8_OWNER_DEAD = 2
} H8OwnerState;

typedef enum H8SpanQueueState {
  H8_Q_IDLE = 0,
  H8_Q_QUEUED = 1,
  H8_Q_DRAINING = 2,
  H8_Q_DRAINING_DIRTY = 3
} H8SpanQueueState;

typedef enum H8SpanState {
  H8_SPAN_OWNED_ACTIVE = 0,
  H8_SPAN_ORPHAN_QUIESCING = 1,
  H8_SPAN_ORPHAN_READY = 2,
  H8_SPAN_ADOPTING = 3,
  H8_SPAN_RETIRING = 4,
  H8_SPAN_RETIRED = 5
} H8SpanState;

typedef enum H8OwnerPlacement {
  H8_OWNER_PLACEMENT_OWNED = 0,
  H8_OWNER_PLACEMENT_ORPHAN = 1
} H8OwnerPlacement;

typedef struct H8OwnerRecord H8OwnerRecord;
typedef struct H8Span H8Span;
typedef struct H8ThreadCtx H8ThreadCtx;

typedef struct H8OwnerWord {
  uint8_t slot;
  uint16_t generation;
  uint8_t state;
  uint32_t span_epoch;
} H8OwnerWord;

typedef struct H8CtlWord {
  uint16_t generation;
  uint8_t state;
  uint8_t publish_closed;
  uint16_t publish_refs;
} H8CtlWord;

struct H8Span {
  union {
    struct {
      uint8_t* base;
      uint16_t class_id;
      uint16_t slot_count;
      _Atomic uint64_t* live_bits;
      _Atomic uint64_t* pending_bits;
      _Atomic uint32_t* slot_state;
      void* meta_alloc_base;
      bool meta_bundled;
    };
    struct {
      uint8_t* immutable_base;
      uint16_t immutable_class_id;
      uint16_t immutable_slot_count;
      _Atomic uint64_t* immutable_live_bits;
      _Atomic uint64_t* immutable_pending_bits;
      _Atomic uint32_t* immutable_slot_state;
      void* immutable_meta_alloc_base;
      bool immutable_meta_bundled;
    } immutable;
  };
  union {
    struct {
      _Atomic uint64_t owner_word;
      uint32_t owner_slot;
      uint16_t owner_generation;
      _Atomic uint8_t span_state;
      _Atomic uint8_t publish_closed;
      _Atomic uint16_t publish_refs;
      _Atomic uint8_t qstate;
      _Atomic size_t pending_count;
      _Atomic uint64_t pending_word_mask;
      _Atomic uint32_t span_epoch;
    };
    struct {
      _Atomic uint64_t remote_owner_word;
      uint32_t remote_owner_slot;
      uint16_t remote_owner_generation;
      _Atomic uint8_t remote_span_state;
      _Atomic uint8_t remote_publish_closed;
      _Atomic uint16_t remote_publish_refs;
      _Atomic uint8_t remote_qstate;
      _Atomic size_t remote_pending_count;
      _Atomic uint64_t remote_pending_word_mask;
      _Atomic uint32_t remote_span_epoch;
    } remote_hot;
  } H8_CACHELINE_ALIGNED;
  struct {
    _Atomic uint32_t local_bump_index;
    _Atomic uint32_t local_free_head_word;
#if defined(H8_ENABLE_DEBUG_STATS)
    size_t local_used_mirror;
#endif
  } local_hot H8_CACHELINE_ALIGNED;
  struct H8Span* next_owned;
  struct H8Span* next_owned_class;
  struct H8Span* next_pending;
  struct H8Span* next_orphan;
  struct H8Span* next_orphan_class;
};

_Static_assert(_Alignof(H8Span) >= H8_CACHELINE_BYTES,
               "H8Span must be cacheline aligned");
_Static_assert(offsetof(H8Span, remote_hot) % H8_CACHELINE_BYTES == 0,
               "remote span hot fields must start on a cacheline");
_Static_assert(offsetof(H8Span, local_hot) % H8_CACHELINE_BYTES == 0,
               "local span hot fields must start on a cacheline");

struct H8OwnerRecord {
  _Atomic uint64_t control;
  uint32_t slot;
  uint32_t generation;
  bool permanent;
  H8OwnerPlacement placement;
  H8Span* owned_by_class[H8_CLASS_COUNT];
  H8MediumRun* medium_by_class[H8_MEDIUM_CLASS_COUNT];
  _Atomic(H8Span*) pending_head;
  _Atomic size_t pending_span_count;
  H8Span* pending_carry;
  _Atomic(H8MediumRun*) medium_pending_head;
  H8MediumRun* medium_pending_carry;
  H8Span* owned_head;
  H8Span* orphan_head;
  H8Span* orphan_by_class[H8_CLASS_COUNT];
  size_t span_chunk_next;
  size_t span_chunk_end;
  atomic_size_t medium_pending_count;
  pthread_mutex_t owned_lock;
  pthread_mutex_t pending_lock;
  struct H8OwnerRecord* free_next;
};

struct H8ThreadCtx {
  H8OwnerRecord* owner;
  H8Span* active_spans[H8_CLASS_COUNT];
  H8MediumRun* active_medium_runs[H8_MEDIUM_CLASS_COUNT];
  uint8_t medium_collect_credit;
};

typedef struct H8Global {
  pthread_once_t once;
  pthread_key_t thread_key;
  _Atomic bool ready;
  void* arena_base;
  size_t arena_bytes;
  size_t span_count;
  atomic_size_t arena_committed_bytes;
  atomic_size_t span_alloc_cursor;
  _Atomic(H8Span*)* spans;
  H8OwnerRecord owners[H8_OWNER_MAX];
  H8OwnerRecord* owner_free;
  pthread_mutex_t owner_lock;
  H8OwnerRecord* orphan_owner;
  H8OwnerRecord* current_owner;
  atomic_size_t owner_count;
  atomic_size_t orphan_span_count;
  atomic_size_t local_alloc_count;
  atomic_size_t local_free_count;
  atomic_size_t remote_publish_count;
  atomic_size_t remote_collect_count;
  atomic_size_t owner_publish_enter_count;
  atomic_size_t owner_publish_exit_count;
  atomic_size_t owner_exit_count;
  atomic_size_t remote_regular_admission_count;
  atomic_size_t remote_orphan_admission_count;
  atomic_size_t remote_stage_enter;
  atomic_size_t remote_stage_span_miss;
  atomic_size_t remote_stage_owner_missing;
  atomic_size_t remote_stage_regular_lease_ok;
  atomic_size_t remote_stage_regular_lease_fail;
  atomic_size_t remote_stage_regular_lease_elided;
  atomic_size_t remote_stage_orphan_lease_ok;
  atomic_size_t remote_stage_orphan_lease_fail;
  atomic_size_t remote_stage_pending_claim_ok;
  atomic_size_t remote_stage_validate_fail;
  atomic_size_t remote_stage_notify_first;
  atomic_size_t remote_stage_publish_ok;
  atomic_size_t remote_stage_pending_publish_elided;
  atomic_size_t remote_lookup_enter;
  atomic_size_t remote_lookup_arena_miss;
  atomic_size_t remote_lookup_span_miss;
  atomic_size_t remote_lookup_retired;
  atomic_size_t remote_lookup_slot_oob;
  atomic_size_t remote_lookup_ok;
  atomic_size_t remote_owner_word_load;
  atomic_size_t pending_notify_count;
  atomic_size_t qstate_notify_attempt_count;
  atomic_size_t qstate_notify_success_count;
  atomic_size_t qstate_notify_skip_nonidle_count;
  atomic_size_t pending_head_push_attempt_count;
  atomic_size_t pending_head_push_retry_count;
  atomic_size_t pending_head_push_success_count;
  atomic_size_t owner_lifecycle_enter_cas_retry_count;
  atomic_size_t owner_lifecycle_exit_cas_retry_count;
  atomic_size_t remote_publish_pending_claim_duplicate_count;
  atomic_size_t pending_collect_call_count;
  atomic_size_t pending_collect_carry_hit_count;
  atomic_size_t pending_collect_requeue_count;
  atomic_size_t pending_enqueue_count;
  atomic_size_t pending_dequeue_count;
  atomic_size_t pending_word_summary_set;
  atomic_size_t pending_word_summary_shadow_hit;
  atomic_size_t pending_word_summary_false_positive;
  atomic_size_t pending_word_summary_false_negative;
  atomic_size_t pending_word_summary_rearm;
  atomic_size_t pending_word_drain_count;
  atomic_size_t pending_word_popcount_1;
  atomic_size_t pending_word_popcount_2;
  atomic_size_t pending_word_popcount_3_4;
  atomic_size_t pending_word_popcount_5_8;
  atomic_size_t pending_word_popcount_9_16;
  atomic_size_t pending_word_popcount_17_plus;
  atomic_size_t pending_slots_drained;
  atomic_size_t pending_words_rearmed;
  atomic_size_t pending_word_new_publish_during_drain;
  atomic_size_t pending_mask_notify_without_count;
  atomic_size_t pending_count_notify_without_mask;
  atomic_size_t pending_mask_requeue_without_count;
  atomic_size_t pending_count_requeue_without_mask;
  atomic_size_t pending_finish_count_mask_zero_bitmap_zero;
  atomic_size_t pending_finish_count_mask_zero_bitmap_nonzero;
  atomic_size_t pending_finish_mask_nonzero_bitmap_zero;
  atomic_size_t pending_finish_mask_nonzero_bitmap_nonzero;
  atomic_size_t qstate_dirty_set;
  atomic_size_t qstate_dirty_self_set;
  atomic_size_t qstate_dirty_requeue;
  atomic_size_t quiescent_pending_bitmap_nonzero;
  atomic_size_t quiescent_pending_repair;
  atomic_size_t local_alloc_pending_nonzero;
  atomic_size_t local_free_pending_nonzero;
  atomic_size_t owner_live_set_already_live;
  atomic_size_t owner_live_clear_already_free;
  atomic_size_t local_active_hit;
  atomic_size_t local_active_miss;
  atomic_size_t local_freelist_pop;
  atomic_size_t local_bump_alloc;
  atomic_size_t local_slow_collect;
  atomic_size_t local_span_commit;
  atomic_size_t local_find_scan;
  atomic_size_t local_find_scan_span;
  atomic_size_t local_active_hint_null;
  atomic_size_t local_active_hint_full;
  atomic_size_t local_active_hint_state_blocked;
  atomic_size_t local_active_hint_trusted;
  atomic_size_t local_active_hint_class_mismatch;
  atomic_size_t local_active_hint_owner_mismatch;
  atomic_size_t local_active_hint_generation_mismatch;
  atomic_size_t local_active_hint_state_mismatch;
  atomic_size_t local_find_scan_span_usable;
  atomic_size_t local_find_scan_span_full;
  atomic_size_t local_find_scan_span_state_blocked;
  atomic_size_t local_find_skip_scan_no_pending;
  atomic_size_t local_free_hit;
  atomic_size_t local_free_reject_owner;
  atomic_size_t local_free_reject_state;
  atomic_size_t local_free_reject_live;
  atomic_size_t local_live_touch_alloc;
  atomic_size_t local_live_touch_free;
  atomic_size_t local_live_word_0;
  atomic_size_t local_live_word_1;
  atomic_size_t local_live_word_2_7;
  atomic_size_t local_live_word_8_31;
  atomic_size_t local_live_word_32_63;
  atomic_size_t local_free_head_touch_alloc;
  atomic_size_t local_free_head_touch_free;
  atomic_size_t local_pending_check_alloc;
  atomic_size_t local_pending_check_free;
  atomic_size_t local_used_touch_alloc;
  atomic_size_t local_used_touch_free;
  atomic_size_t local_used_count_load_alloc;
  atomic_size_t local_used_count_store_alloc;
  atomic_size_t local_used_count_load_free;
  atomic_size_t local_used_count_store_free;
  atomic_size_t local_used_count_full_check;
  atomic_size_t local_used_count_underflow;
  atomic_size_t local_used_mirror_mismatch;
  atomic_size_t local_used_mirror_underflow;
  atomic_size_t local_used_cold_active_hint;
  atomic_size_t local_used_cold_owner_scan_locked;
  atomic_size_t local_used_cold_adoption_locked;
  atomic_size_t local_used_cold_owner_exit;
  atomic_size_t local_used_cold_verify_quiescent;
  atomic_size_t local_used_derived_mismatch;
  atomic_size_t local_used_derived_quiescent_scan;
  atomic_size_t span_commit_total_ns;
  atomic_size_t span_commit_meta_ns;
  atomic_size_t span_commit_mprotect_ns;
  atomic_size_t owner_exit_total_ns;
  atomic_size_t owner_exit_collect_ns;
  atomic_size_t owner_exit_span_walk_ns;
  atomic_size_t span_retire_count;
  atomic_size_t span_retire_total_ns;
  atomic_size_t span_retire_lock_wait_ns;
  atomic_size_t span_retire_madvise_ns;
  atomic_size_t span_retire_meta_free_ns;
  atomic_size_t span_purge_run_count;
  atomic_size_t span_purge_run_spans_total;
  atomic_size_t span_purge_run_max;
  atomic_size_t span_purge_singleton_runs;
  atomic_size_t span_purge_madvise_calls;
  atomic_size_t span_purge_madvise_bytes;
  atomic_size_t span_purge_madvise_ns;
  atomic_size_t slot_shadow_valid_mismatch;
  atomic_size_t slot_shadow_invalid_mismatch;
  atomic_size_t slot_shadow_pending_nonallocated;
  atomic_size_t slot_shadow_free_unreachable;
  atomic_size_t slot_shadow_free_duplicate;
  atomic_size_t slot_shadow_free_cycle;
  atomic_size_t slot_shadow_bad_next;
  atomic_size_t slot_shadow_never_used_below_bump;
  atomic_size_t slot_shadow_nonvirgin_above_bump;
  atomic_size_t slot_shadow_used_mismatch;
  atomic_size_t slot_shadow_reserved_quiescent;
  atomic_size_t medium_malloc_count;
  atomic_size_t medium_run_create_count;
  atomic_size_t medium_run_reuse_active_count;
  atomic_size_t medium_run_reuse_owner_list_count;
  atomic_size_t medium_run_reuse_global_count;
  atomic_size_t medium_run_madvise_count;
  atomic_size_t medium_owner_scan_count;
  atomic_size_t medium_owner_scan_step_count;
  atomic_size_t medium_global_scan_count;
  atomic_size_t medium_global_scan_step_count;
  atomic_size_t medium_free_lookup_count;
  atomic_size_t medium_route_lookup_count;
  atomic_size_t medium_invalid_owned_count;
  atomic_size_t medium_empty_transition_count;
  atomic_size_t medium_empty_retain_count;
  atomic_size_t medium_empty_budget_reject_count;
  atomic_size_t medium_empty_reactivate_count;
  atomic_size_t medium_owner_exit_drain_count;
  atomic_size_t medium_madvise_fail_count;
  atomic_size_t medium_resident_empty_bytes;
  atomic_size_t medium_resident_empty_peak;
  atomic_size_t medium_madvise_ns;
  atomic_size_t medium_global_lock_wait_ns;
  atomic_size_t medium_run_lock_wait_ns;
  atomic_size_t medium_alloc_slot_ns;
  atomic_size_t medium_free_slot_ns;
  atomic_size_t medium_alloc_slot_count;
  atomic_size_t medium_free_slot_count;
  atomic_size_t medium_lock_elide_alloc_candidate;
  atomic_size_t medium_lock_elide_free_candidate;
  atomic_size_t medium_lock_elide_owner_mismatch;
  atomic_size_t medium_active_alloc_owner_mismatch;
  atomic_size_t medium_owner_list_owner_mismatch;
  atomic_size_t medium_global_skip_foreign_attached;
  atomic_size_t medium_local_free_owner_match;
  atomic_size_t medium_remote_free_owner_mismatch;
  atomic_size_t medium_free_lookup_step_count;
  atomic_size_t medium_route_lookup_step_count;
  atomic_size_t medium_route_authority_mismatch;
  atomic_size_t medium_remote_publish_count;
  atomic_size_t medium_remote_owner_lease_ns;
  atomic_size_t medium_remote_owner_lease_enter_ns;
  atomic_size_t medium_remote_owner_lease_exit_ns;
  atomic_size_t medium_remote_run_lock_ns;
  atomic_size_t medium_remote_pending_claim_count;
  atomic_size_t medium_remote_pending_claim_ns;
  atomic_size_t medium_remote_lockless_claim_count;
  atomic_size_t medium_remote_lockless_claim_collector_accept;
  atomic_size_t medium_remote_lockless_claim_rollback_invalid;
  atomic_size_t medium_remote_lockless_claim_rollback_accepted;
  atomic_size_t medium_attached_writer_overlap;
  atomic_size_t medium_attached_foreign_mask_writer;
  atomic_size_t medium_owner_token_changed_during_mutation;
  atomic_size_t medium_collect_wrong_owner;
  atomic_size_t medium_detached_direct_free_while_attached;
  atomic_size_t medium_remote_lockless_shadow_attempt;
  atomic_size_t medium_remote_lockless_shadow_would_accept;
  atomic_size_t medium_remote_lockless_shadow_would_reject;
  atomic_size_t medium_remote_lockless_shadow_match;
  atomic_size_t medium_remote_lockless_shadow_mismatch;
  atomic_size_t medium_remote_notify_count;
  atomic_size_t medium_remote_queue_push_count;
  atomic_size_t medium_remote_queue_push_attempt_count;
  atomic_size_t medium_remote_queue_push_retry_count;
  atomic_size_t medium_remote_queue_push_success_count;
  atomic_size_t medium_remote_queue_push_ns;
  atomic_size_t medium_remote_collect_call_count;
  atomic_size_t medium_remote_collect_run_count;
  atomic_size_t medium_remote_collect_slot_count;
  atomic_size_t medium_remote_collect_ns;
  atomic_size_t medium_collect_finish_pending_rearm;
  atomic_size_t medium_empty_with_pending;
  atomic_size_t medium_remote_publish_class_8k;
  atomic_size_t medium_remote_publish_class_16k;
  atomic_size_t medium_remote_publish_class_32k;
  atomic_size_t medium_remote_publish_class_64k;
  atomic_size_t medium_remote_qpush_class_8k;
  atomic_size_t medium_remote_qpush_class_16k;
  atomic_size_t medium_remote_qpush_class_32k;
  atomic_size_t medium_remote_qpush_class_64k;
  atomic_size_t medium_remote_collect_run_class_8k;
  atomic_size_t medium_remote_collect_run_class_16k;
  atomic_size_t medium_remote_collect_run_class_32k;
  atomic_size_t medium_remote_collect_run_class_64k;
  atomic_size_t medium_remote_collect_slot_class_8k;
  atomic_size_t medium_remote_collect_slot_class_16k;
  atomic_size_t medium_remote_collect_slot_class_32k;
  atomic_size_t medium_remote_collect_slot_class_64k;
  atomic_size_t pending_collect_word_count;
  atomic_size_t pending_collect_word_nonzero_count;
  atomic_size_t pending_collect_bit_count;
  atomic_size_t orphan_handoff_count;
  atomic_size_t handoff_success_count;
  atomic_size_t owner_lifecycle_enter_count;
  atomic_size_t owner_lifecycle_exit_count;
  atomic_size_t span_publish_enter_count;
  atomic_size_t span_publish_exit_count;
  atomic_size_t orphan_quiesce_count;
  atomic_size_t orphan_ready_count;
  atomic_size_t adoption_dry_run_scan_count;
  atomic_size_t adoption_dry_run_candidate_count;
  atomic_size_t adoption_dry_run_block_state_count;
  atomic_size_t adoption_dry_run_block_quiesce_count;
  atomic_size_t adoption_dry_run_empty_count;
  atomic_size_t adoption_dry_run_target_closed_count;
  atomic_size_t adoption_dry_run_would_adopt_count;
  atomic_size_t handoff_fail_count;
  atomic_size_t invalid_count;
  atomic_size_t miss_count;
  atomic_size_t owner_transition_count;
  atomic_size_t adoption_scan_count;
  atomic_size_t adoption_candidate_count;
  atomic_size_t adoption_block_state_count;
  atomic_size_t adoption_block_quiesce_count;
  atomic_size_t adoption_empty_count;
  atomic_size_t adoption_target_closed_count;
  atomic_size_t adoption_success_count;
  _Atomic bool regular_adoption_enabled;
  _Atomic bool remote_lease_elision_enabled;
  _Atomic bool remote_pending_publish_elision_enabled;
} H8Global;

#endif
