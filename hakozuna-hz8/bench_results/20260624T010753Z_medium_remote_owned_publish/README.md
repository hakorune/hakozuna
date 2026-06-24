# Medium Remote Owned Publish Observation

timestamp_utc: 20260624T010753Z
commit_base: 612f13018dd6f450a44a22fbea817ac775c6f17c

## Debug r50
run=1 ops/s=3029134.977 post_rss=1929216 peak_rss=2097152 minor_faults=208
run_interleaved=1 work_ms=6.409 work_ops/s=3120510.366 tail_ms=0.185 remote_enqueue=9972 local_free=10028 drain_calls=20335 drain_objects=9972 drain_empty=13821 push_yields=0 finish_yields=202
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=1 class_map_id=p2-v0
throughput median=3029134.977 p25=3029134.977 p75=3029134.977 min=3029134.977 max=3029134.977
post_rss median=1929216 min=1929216 max=1929216
peak_rss median=2097152 min=2097152 max=2097152 source=VmHWM_process
page_faults minor_median=208 minor_min=208 minor_max=208
steady_work throughput_median=3120510.366 p25=3120510.366 p75=3120510.366
interleaved_phase_ms work_median=6.409 tail_median=0.185
interleaved_work remote_enqueue=9972 local_free=10028 drain_calls=20335 drain_objects=9972 drain_empty=13821 push_yields=0 finish_yields=202
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
qstate_dirty set=0 self_set=0 requeue=0
quiescent_pending bitmap_nonzero=0 repair=0
queue_contention qstate_attempt=0 qstate_success=0 qstate_skip=0 pending_push_attempt=0 pending_push_retry=0 pending_push_success=0 owner_enter_retry=0 owner_exit_retry=0 duplicate_claim=0
local_zero_gates alloc_pending=0 free_pending=0 live_set_already=0 live_clear_free=0
local_path active_hit=0 active_miss=0 freelist=0 bump=0 slow_collect=0 span_commit=0 find_scan=0 scan_span=0 free_hit=0 reject_owner=0 reject_state=0 reject_live=0
medium_stats malloc=20000 create=20 active_reuse=16887 owner_reuse=3093 global_reuse=0 madvise=20 owner_scan=3113 owner_steps=5064 global_scan=20 global_steps=182 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=18867 retain=18867 budget_reject=0 reactivate=18847 exit_drain=20 madvise_fail=0 resident_bytes=0 resident_peak=1310720 madvise_ms=0.102 global_lock_ms=0.072 run_lock_ms=1.166 alloc_slot_ms=0.952 free_slot_ms=0.591 alloc_slot=20000 free_slot=10028 lock_elide_alloc=23085 lock_elide_free=10028 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=182 local_free_owner=10028 remote_free_owner=9972 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.169 owner_exit_collect_ms=0.001 owner_exit_span_walk_ms=0.168 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## Interpretation

MediumRunRemoteOwnedPublish-L1 is now active for owner-attached runs.

```text
debug r50:
  publish_enter/lifecycle_enter = 9972
  medium remote_free_owner      = 9972
  medium free_slot              = 10028
  active_owner_mismatch         = 0
  owner_list_mismatch           = 0
  invalid_owned                 = 0

debug local:
  remote_free_owner             = 0
  lock_elide_mismatch           = 0
```

This confirms the intended ownership split:

```text
same-owner free:
  direct run mutation remains

foreign free to attached run:
  owner lifecycle lease + pending bit publish
  owner collect performs the actual slot mutation

detached run:
  still uses the direct-lock fallback
```

Performance impact:

```text
release r50 median:
  5.18M ops/s

release local median:
  16.25M ops/s in this short R5 row
```

The r50 regression is expected for this correctness box because every attached
remote free now pays owner lease + pending publication + owner collect.  The next
performance box should reduce that cost rather than re-open direct remote mask
mutation.

Next candidates:

```text
MediumRunRemoteCostAudit-L1
MediumRunOwnerLocalLockElision-L1
MediumRunDetachedOrphanRemote-L1
```

## Debug local
run=1 ops/s=4347005.826 post_rss=1835008 peak_rss=1835008 minor_faults=67
run_phase=1 alloc_ms=4.464 remote_ms=0.000
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=0 interleaved=0 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=4347005.826 p25=4347005.826 p75=4347005.826 min=4347005.826 max=4347005.826
post_rss median=1835008 min=1835008 max=1835008
peak_rss median=1835008 min=1835008 max=1835008 source=VmHWM_process
page_faults minor_median=67 minor_min=67 minor_max=67
steady_work throughput_median=4480438.294 p25=4480438.294 p75=4480438.294
phase_ms alloc_median=4.464 remote_median=0.000
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
medium_stats malloc=20000 create=8 active_reuse=19992 owner_reuse=0 global_reuse=0 madvise=8 owner_scan=8 owner_steps=0 global_scan=8 global_steps=25 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=20000 retain=20000 budget_reject=0 reactivate=19992 exit_drain=8 madvise_fail=0 resident_bytes=0 resident_peak=524288 madvise_ms=0.035 global_lock_ms=0.032 run_lock_ms=1.148 alloc_slot_ms=1.226 free_slot_ms=1.434 alloc_slot=20000 free_slot=20000 lock_elide_alloc=19992 lock_elide_free=20000 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=25 local_free_owner=20000 remote_free_owner=0 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.058 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.058 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## Release r50
run=1 ops/s=5182945.270 post_rss=2015232 peak_rss=3276800 minor_faults=500
run_interleaved=1 work_ms=18.954 work_ops/s=5275961.264 tail_ms=0.335 remote_enqueue=50182 local_free=49818 drain_calls=100308 drain_objects=50182 drain_empty=86176 push_yields=0 finish_yields=177
run=2 ops/s=5178379.107 post_rss=2088960 peak_rss=3719168 minor_faults=409
run_interleaved=2 work_ms=19.028 work_ops/s=5255348.447 tail_ms=0.487 remote_enqueue=49830 local_free=50170 drain_calls=100383 drain_objects=49830 drain_empty=85945 push_yields=0 finish_yields=194
run=3 ops/s=4978677.569 post_rss=2244608 peak_rss=4055040 minor_faults=494
run_interleaved=3 work_ms=19.718 work_ops/s=5071491.548 tail_ms=0.312 remote_enqueue=49887 local_free=50113 drain_calls=100234 drain_objects=49887 drain_empty=86114 push_yields=0 finish_yields=119
run=4 ops/s=5071666.708 post_rss=2060288 peak_rss=4055040 minor_faults=406
run_interleaved=4 work_ms=19.425 work_ops/s=5148037.216 tail_ms=0.533 remote_enqueue=49992 local_free=50008 drain_calls=100464 drain_objects=49992 drain_empty=85806 push_yields=0 finish_yields=232
run=5 ops/s=5297931.004 post_rss=2101248 peak_rss=4055040 minor_faults=438
run_interleaved=5 work_ms=18.573 work_ops/s=5384073.126 tail_ms=1.097 remote_enqueue=50056 local_free=49944 drain_calls=100873 drain_objects=50056 drain_empty=86852 push_yields=0 finish_yields=448
summary runs=5 threads=2 iters=50000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=0 class_map_id=p2-v0
throughput median=5178379.107 p25=5071666.708 p75=5182945.270 min=4978677.569 max=5297931.004
post_rss median=2088960 min=2015232 max=2244608
peak_rss median=4055040 min=3276800 max=4055040 source=VmHWM_process
page_faults minor_median=438 minor_min=406 minor_max=500
steady_work throughput_median=5255348.447 p25=5148037.216 p75=5275961.264
interleaved_phase_ms work_median=19.028 tail_median=0.487
interleaved_work remote_enqueue=249947 local_free=250053 drain_calls=502262 drain_objects=249947 drain_empty=430893 push_yields=0 finish_yields=1170
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
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.000 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.000 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## Release local
run=1 ops/s=20231533.718 post_rss=1703936 peak_rss=1703936 minor_faults=67
run_phase=1 alloc_ms=4.832 remote_ms=0.000
run=2 ops/s=15875162.800 post_rss=2228224 peak_rss=2228224 minor_faults=38
run_phase=2 alloc_ms=6.196 remote_ms=0.000
run=3 ops/s=21145659.124 post_rss=2359296 peak_rss=2359296 minor_faults=38
run_phase=3 alloc_ms=4.630 remote_ms=0.000
run=4 ops/s=15044380.924 post_rss=2482176 peak_rss=2482176 minor_faults=38
run_phase=4 alloc_ms=6.560 remote_ms=0.000
run=5 ops/s=16247814.263 post_rss=2326528 peak_rss=2482176 minor_faults=38
run_phase=5 alloc_ms=6.053 remote_ms=0.000
summary runs=5 threads=2 iters=50000 size=4097..65536 remote_pct=0 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=16247814.263 p25=15875162.800 p75=20231533.718 min=15044380.924 max=21145659.124
post_rss median=2326528 min=1703936 max=2482176
peak_rss median=2359296 min=1703936 max=2482176 source=VmHWM_process
page_faults minor_median=38 minor_min=38 minor_max=67
steady_work throughput_median=16521224.818 p25=16139384.893 p75=20696058.105
phase_ms alloc_median=6.053 remote_median=0.000
live_span_lower_bound remote_live_median=0 spans_median=0
candidate_span_lower_bound upper1536_median=0 upper1p5_median=0
size_policy_live_bytes p2_median=0 upper1536_median=0 upper1p5_median=0
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
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.000 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.000 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0
