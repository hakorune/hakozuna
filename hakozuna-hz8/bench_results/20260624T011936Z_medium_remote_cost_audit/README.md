# Medium Remote Cost Audit

timestamp_utc: 20260624T011936Z
commit_base: a1ceaa2dcbd1b1912dd18e4a67c519498d1c7b7b

## Summary

`MediumRunRemoteCostAudit-L1` adds debug-only attribution for the medium
owner-attached remote path.  The audit did not change allocator behavior.

Key r50 debug observations:

```text
remote publishes:
  remote_pub=9972
  remote_claim=9972
  remote_notify=9657
  remote_qpush=9657

owner collect:
  remote_collect_call=20002
  remote_collect_run=9657
  remote_collect_slot=9972

cost split:
  remote_collect_ms=2.390
  remote_lease_ms=0.684
  remote_run_lock_ms=0.574
  remote_claim_ms=0.348
```

Interpretation:

```text
correctness:
  active_owner_mismatch=0
  owner_list_mismatch=0
  invalid_owned=0

remote fixed cost:
  owner lease, pending claim, and run-lock wait are visible but not dominant

owner collect:
  the largest measured debug cost is medium owner collect

handoff shape:
  notify / queue-push happens for almost every remote free
  medium has one pending word, so old_word==0 commonly becomes one notification

collect cadence:
  collect_call is one per medium malloc attempt in this row
  empty fast checks are cheap but still show that owner collect is very eager
```

Next decision:

```text
do not change owner lease yet
do not add chunk arena for this specific regression yet

next likely boxes:
  MediumRunRemoteNotifyCoalescing-L1
    reduce near-one-notify-per-remote-free handoff pressure

  MediumRunOwnerCollectCadence-L1
    batch owner medium collect instead of collecting on every malloc attempt

  MediumRunOwnerLocalLockElision-L1
    still valid after remote direct mutation has been removed,
    but it does not directly address remote queue handoff cost
```

## Debug r50
run=1 ops/s=2318872.305 post_rss=1900544 peak_rss=2228224 minor_faults=238
run_interleaved=1 work_ms=8.432 work_ops/s=2371862.500 tail_ms=0.131 remote_enqueue=9972 local_free=10028 drain_calls=20177 drain_objects=9972 drain_empty=14165 push_yields=0 finish_yields=126
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=1 class_map_id=p2-v0
throughput median=2318872.305 p25=2318872.305 p75=2318872.305 min=2318872.305 max=2318872.305
post_rss median=1900544 min=1900544 max=1900544
peak_rss median=2228224 min=2228224 max=2228224 source=VmHWM_process
page_faults minor_median=238 minor_min=238 minor_max=238
steady_work throughput_median=2371862.500 p25=2371862.500 p75=2371862.500
interleaved_phase_ms work_median=8.432 tail_median=0.131
interleaved_work remote_enqueue=9972 local_free=10028 drain_calls=20177 drain_objects=9972 drain_empty=14165 push_yields=0 finish_yields=126
fragmentation requested_bytes=0 rounded_bytes=0 rounding_ratio=0.000000
fragmentation_candidates upper1536_rounded_bytes=0 upper1536_ratio=0.000000 upper1p5_rounded_bytes=0 upper1p5_ratio=0.000000
fragmentation_by_class class_count=9 allocs=[0,0,0,0,0,0,0,0,0] rounded_bytes=[0,0,0,0,0,0,0,0,0]
medium_route_shadow candidate_count=20000 remote_live_count=9972 requested_bytes=696381158 rounded_bytes=928030720 remote_requested_bytes=345892886 remote_rounded_bytes=462159872
stats_snapshot owner_exit=2 pending_enqueue=0 pending_dequeue=0 orphan_handoff=0 handoff_ok=0 local=0 remote=0
counters_dbg publish_enter=9972 publish_exit=9972 lifecycle_enter=9972 lifecycle_exit=9972 span_publish_enter=0 span_publish_exit=0 remote_regular=0 remote_orphan=0 pending_notify=0 pending_calls=2 pending_carry_hit=0 pending_requeue=0 pending_word_set=0 pending_word_shadow_hit=0 pending_word_false_pos=0 pending_word_false_neg=0 pending_word_rearm=0 pending_words=0 pending_words_nonzero=0 pending_bits=0 orphan_quiesce=0 orphan_ready=0 dry_scan=0 dry_candidate=0 dry_block_state=0 dry_block_quiesce=0 dry_empty=0 dry_target_closed=0 dry_would_adopt=0 handoff_fail=0 invalid=0 miss=0 owner_transition=0 adopt_scan=0 adopt_candidate=0 adopt_block_state=0 adopt_block_quiesce=0 adopt_empty=0 adopt_target_closed=0 adopt_ok=0
remote_stage enter=0 span_miss=0 owner_missing=0 regular_lease_ok=0 regular_lease_fail=0 regular_lease_elided=0 orphan_lease_ok=0 orphan_lease_fail=0 pending_claim_ok=0 validate_fail=0 notify_first=0 publish_ok=0 pending_publish_elided=0
remote_lookup enter=0 arena_miss=0 span_miss=0 retired=0 slot_oob=0 ok=0 owner_word=0
pending_word_density drain=0 pop1=0 pop2=0 pop3_4=0 pop5_8=0 pop9_16=0 pop17p=0 slots=0 rearmed=0 new_publish=0 slots_per_nonzero_word=0.000 singleton_ratio=0.000 multi_ratio=0.000
pending_count_shadow mask_notify_without_count=0 count_notify_without_mask=0 mask_requeue_without_count=0 count_requeue_without_mask=0
pending_finish_shadow count_mask0_bitmap0=0 count_mask0_bitmap1=0 mask1_bitmap0=0 mask1_bitmap1=0
qstate_dirty set=1 self_set=0 requeue=1
quiescent_pending bitmap_nonzero=0 repair=0
queue_contention qstate_attempt=0 qstate_success=0 qstate_skip=0 pending_push_attempt=0 pending_push_retry=0 pending_push_success=0 owner_enter_retry=0 owner_exit_retry=0 duplicate_claim=0
local_zero_gates alloc_pending=0 free_pending=0 live_set_already=0 live_clear_free=0
local_path active_hit=0 active_miss=0 freelist=0 bump=0 slow_collect=0 span_commit=0 find_scan=0 scan_span=0 free_hit=0 reject_owner=0 reject_state=0 reject_live=0
medium_stats malloc=20000 create=23 active_reuse=16900 owner_reuse=3077 global_reuse=0 madvise=23 owner_scan=3100 owner_steps=5118 global_scan=23 global_steps=244 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=18807 retain=18807 budget_reject=0 reactivate=18784 exit_drain=23 madvise_fail=0 resident_bytes=0 resident_peak=1507328 madvise_ms=0.117 global_lock_ms=0.080 run_lock_ms=1.489 alloc_slot_ms=0.957 free_slot_ms=0.533 alloc_slot=20000 free_slot=10028 lock_elide_alloc=23069 lock_elide_free=10028 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=244 local_free_owner=10028 remote_free_owner=9972 free_steps=0 route_steps=0 remote_pub=9972 remote_lease_ms=0.684 remote_run_lock_ms=0.574 remote_claim=9972 remote_claim_ms=0.348 remote_notify=9657 remote_qpush=9657 remote_collect_call=20002 remote_collect_run=9657 remote_collect_slot=9972 remote_collect_ms=2.390
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.188 owner_exit_collect_ms=0.001 owner_exit_span_walk_ms=0.187 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## Debug local
run=1 ops/s=4072184.354 post_rss=1966080 peak_rss=1966080 minor_faults=67
run_phase=1 alloc_ms=4.788 remote_ms=0.000
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=0 interleaved=0 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=4072184.354 p25=4072184.354 p75=4072184.354 min=4072184.354 max=4072184.354
post_rss median=1966080 min=1966080 max=1966080
peak_rss median=1966080 min=1966080 max=1966080 source=VmHWM_process
page_faults minor_median=67 minor_min=67 minor_max=67
steady_work throughput_median=4177465.415 p25=4177465.415 p75=4177465.415
phase_ms alloc_median=4.788 remote_median=0.000
live_span_lower_bound remote_live_median=0 spans_median=0
candidate_span_lower_bound upper1536_median=0 upper1p5_median=0
size_policy_live_bytes p2_median=0 upper1536_median=0 upper1p5_median=0
fragmentation requested_bytes=0 rounded_bytes=0 rounding_ratio=0.000000
fragmentation_candidates upper1536_rounded_bytes=0 upper1536_ratio=0.000000 upper1p5_rounded_bytes=0 upper1p5_ratio=0.000000
fragmentation_by_class class_count=9 allocs=[0,0,0,0,0,0,0,0,0] rounded_bytes=[0,0,0,0,0,0,0,0,0]
medium_route_shadow candidate_count=20000 remote_live_count=0 requested_bytes=694787380 rounded_bytes=925548544 remote_requested_bytes=0 remote_rounded_bytes=0
stats_snapshot owner_exit=2 pending_enqueue=0 pending_dequeue=0 orphan_handoff=0 handoff_ok=0 local=0 remote=0
counters_dbg publish_enter=0 publish_exit=0 lifecycle_enter=0 lifecycle_exit=0 span_publish_enter=0 span_publish_exit=0 remote_regular=0 remote_orphan=0 pending_notify=0 pending_calls=2 pending_carry_hit=0 pending_requeue=0 pending_word_set=0 pending_word_shadow_hit=0 pending_word_false_pos=0 pending_word_false_neg=0 pending_word_rearm=0 pending_words=0 pending_words_nonzero=0 pending_bits=0 orphan_quiesce=0 orphan_ready=0 dry_scan=0 dry_candidate=0 dry_block_state=0 dry_block_quiesce=0 dry_empty=0 dry_target_closed=0 dry_would_adopt=0 handoff_fail=0 invalid=0 miss=0 owner_transition=0 adopt_scan=0 adopt_candidate=0 adopt_block_state=0 adopt_block_quiesce=0 adopt_empty=0 adopt_target_closed=0 adopt_ok=0
remote_stage enter=0 span_miss=0 owner_missing=0 regular_lease_ok=0 regular_lease_fail=0 regular_lease_elided=0 orphan_lease_ok=0 orphan_lease_fail=0 pending_claim_ok=0 validate_fail=0 notify_first=0 publish_ok=0 pending_publish_elided=0
remote_lookup enter=0 arena_miss=0 span_miss=0 retired=0 slot_oob=0 ok=0 owner_word=0
pending_word_density drain=0 pop1=0 pop2=0 pop3_4=0 pop5_8=0 pop9_16=0 pop17p=0 slots=0 rearmed=0 new_publish=0 slots_per_nonzero_word=0.000 singleton_ratio=0.000 multi_ratio=0.000
pending_count_shadow mask_notify_without_count=0 count_notify_without_mask=0 mask_requeue_without_count=0 count_requeue_without_mask=0
pending_finish_shadow count_mask0_bitmap0=0 count_mask0_bitmap1=0 mask1_bitmap0=0 mask1_bitmap1=0
qstate_dirty set=0 self_set=0 requeue=0
quiescent_pending bitmap_nonzero=0 repair=0
queue_contention qstate_attempt=0 qstate_success=0 qstate_skip=0 pending_push_attempt=0 pending_push_retry=0 pending_push_success=0 owner_enter_retry=0 owner_exit_retry=0 duplicate_claim=0
local_zero_gates alloc_pending=0 free_pending=0 live_set_already=0 live_clear_free=0
local_path active_hit=0 active_miss=0 freelist=0 bump=0 slow_collect=0 span_commit=0 find_scan=0 scan_span=0 free_hit=0 reject_owner=0 reject_state=0 reject_live=0
medium_stats malloc=20000 create=8 active_reuse=19992 owner_reuse=0 global_reuse=0 madvise=8 owner_scan=8 owner_steps=0 global_scan=8 global_steps=25 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=20000 retain=20000 budget_reject=0 reactivate=19992 exit_drain=8 madvise_fail=0 resident_bytes=0 resident_peak=524288 madvise_ms=0.039 global_lock_ms=0.035 run_lock_ms=1.137 alloc_slot_ms=0.927 free_slot_ms=1.069 alloc_slot=20000 free_slot=20000 lock_elide_alloc=19992 lock_elide_free=20000 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=25 local_free_owner=20000 remote_free_owner=0 free_steps=0 route_steps=0 remote_pub=0 remote_lease_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_notify=0 remote_qpush=0 remote_collect_call=20002 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.468
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.063 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.062 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## Release r50
run=1 ops/s=4843796.289 post_rss=1904640 peak_rss=3145728 minor_faults=479
run_interleaved=1 work_ms=20.301 work_ops/s=4925849.949 tail_ms=0.352 remote_enqueue=50182 local_free=49818 drain_calls=100277 drain_objects=50182 drain_empty=86572 push_yields=0 finish_yields=157
run=2 ops/s=5109068.914 post_rss=1966080 peak_rss=3739648 minor_faults=477
run_interleaved=2 work_ms=19.230 work_ops/s=5200083.347 tail_ms=0.958 remote_enqueue=49830 local_free=50170 drain_calls=100871 drain_objects=49830 drain_empty=86547 push_yields=0 finish_yields=513
run=3 ops/s=5308670.806 post_rss=1966080 peak_rss=3739648 minor_faults=373
run_interleaved=3 work_ms=18.572 work_ops/s=5384574.959 tail_ms=0.882 remote_enqueue=49887 local_free=50113 drain_calls=100705 drain_objects=49887 drain_empty=86965 push_yields=0 finish_yields=359
run=4 ops/s=5179334.724 post_rss=1966080 peak_rss=3739648 minor_faults=416
run_interleaved=4 work_ms=18.992 work_ops/s=5265423.967 tail_ms=1.824 remote_enqueue=49992 local_free=50008 drain_calls=101463 drain_objects=49992 drain_empty=87612 push_yields=0 finish_yields=741
run=5 ops/s=5315914.994 post_rss=1966080 peak_rss=3739648 minor_faults=379
run_interleaved=5 work_ms=18.520 work_ops/s=5399597.482 tail_ms=0.697 remote_enqueue=50056 local_free=49944 drain_calls=100530 drain_objects=50056 drain_empty=85898 push_yields=0 finish_yields=271
summary runs=5 threads=2 iters=50000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=0 class_map_id=p2-v0
throughput median=5179334.724 p25=5109068.914 p75=5308670.806 min=4843796.289 max=5315914.994
post_rss median=1966080 min=1904640 max=1966080
peak_rss median=3739648 min=3145728 max=3739648 source=VmHWM_process
page_faults minor_median=416 minor_min=373 minor_max=479
steady_work throughput_median=5265423.967 p25=5200083.347 p75=5384574.959
interleaved_phase_ms work_median=18.992 tail_median=0.882
interleaved_work remote_enqueue=249947 local_free=250053 drain_calls=503846 drain_objects=249947 drain_empty=433594 push_yields=0 finish_yields=2041
fragmentation attribution=disabled
stats_snapshot owner_exit=0 pending_enqueue=0 pending_dequeue=0 orphan_handoff=0 handoff_ok=0 local=0 remote=0
counters_dbg publish_enter=0 publish_exit=0 lifecycle_enter=0 lifecycle_exit=0 span_publish_enter=0 span_publish_exit=0 remote_regular=0 remote_orphan=0 pending_notify=0 pending_calls=0 pending_carry_hit=0 pending_requeue=0 pending_word_set=0 pending_word_shadow_hit=0 pending_word_false_pos=0 pending_word_false_neg=0 pending_word_rearm=0 pending_words=0 pending_words_nonzero=0 pending_bits=0 orphan_quiesce=0 orphan_ready=0 dry_scan=0 dry_candidate=0 dry_block_state=0 dry_block_quiesce=0 dry_empty=0 dry_target_closed=0 dry_would_adopt=0 handoff_fail=0 invalid=0 miss=0 owner_transition=0 adopt_scan=0 adopt_candidate=0 adopt_block_state=0 adopt_block_quiesce=0 adopt_empty=0 adopt_target_closed=0 adopt_ok=0
remote_stage enter=0 span_miss=0 owner_missing=0 regular_lease_ok=0 regular_lease_fail=0 regular_lease_elided=0 orphan_lease_ok=0 orphan_lease_fail=0 pending_claim_ok=0 validate_fail=0 notify_first=0 publish_ok=0 pending_publish_elided=0
remote_lookup enter=0 arena_miss=0 span_miss=0 retired=0 slot_oob=0 ok=0 owner_word=0
pending_word_density drain=0 pop1=0 pop2=0 pop3_4=0 pop5_8=0 pop9_16=0 pop17p=0 slots=0 rearmed=0 new_publish=0 slots_per_nonzero_word=0.000 singleton_ratio=0.000 multi_ratio=0.000
pending_count_shadow mask_notify_without_count=0 count_notify_without_mask=0 mask_requeue_without_count=0 count_requeue_without_mask=0
pending_finish_shadow count_mask0_bitmap0=0 count_mask0_bitmap1=0 mask1_bitmap0=0 mask1_bitmap1=0
qstate_dirty set=0 self_set=0 requeue=0
quiescent_pending bitmap_nonzero=0 repair=0
queue_contention qstate_attempt=0 qstate_success=0 qstate_skip=0 pending_push_attempt=0 pending_push_retry=0 pending_push_success=0 owner_enter_retry=0 owner_exit_retry=0 duplicate_claim=0
local_zero_gates alloc_pending=0 free_pending=0 live_set_already=0 live_clear_free=0
local_path active_hit=0 active_miss=0 freelist=0 bump=0 slow_collect=0 span_commit=0 find_scan=0 scan_span=0 free_hit=0 reject_owner=0 reject_state=0 reject_live=0
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0 remote_pub=0 remote_lease_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_notify=0 remote_qpush=0 remote_collect_call=0 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.000
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.000 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.000 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0
