# MediumRunOwnerLocalLockElisionShadow-L1
timestamp_utc=20260624T001822Z
sha=384ad60f

## Interpretation

```text
status:
  smoke pass
  safety stress pass
  owner-token shadow counters are visible

debug r50:
  lock_elide_alloc=10943
  lock_elide_free=9958
  lock_elide_mismatch=19092
  free_steps=0
  invalid_owned=0

debug local:
  lock_elide_alloc=16598
  lock_elide_free=16604
  lock_elide_mismatch=6790
  free_steps=0
  invalid_owned=0

release r50:
  median=8.26M ops/s

decision:
  same-owner lock-elision opportunities are material
  actual lock elision is HOLD while medium remote free directly mutates run masks
  next correctness boundary is MediumRunRemote-L1: remote publish/owner collect
```

## smoke
arena=68719476736 committed=2228224 owners=68 local=68 remote=32

## safety
safety_stress owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7

## medium debug r50 2x10k
run=1 ops/s=2643301.992 post_rss=1703936 peak_rss=1810432 minor_faults=162
run_interleaved=1 work_ms=7.424 work_ops/s=2693876.616 tail_ms=0.176 remote_enqueue=9972 local_free=10028 drain_calls=20255 drain_objects=9972 drain_empty=14512 push_yields=0 finish_yields=165
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=1 class_map_id=p2-v0
throughput median=2643301.992 p25=2643301.992 p75=2643301.992 min=2643301.992 max=2643301.992
post_rss median=1703936 min=1703936 max=1703936
peak_rss median=1810432 min=1810432 max=1810432 source=VmHWM_process
page_faults minor_median=162 minor_min=162 minor_max=162
steady_work throughput_median=2693876.616 p25=2693876.616 p75=2693876.616
interleaved_phase_ms work_median=7.424 tail_median=0.176
interleaved_work remote_enqueue=9972 local_free=10028 drain_calls=20255 drain_objects=9972 drain_empty=14512 push_yields=0 finish_yields=165
fragmentation requested_bytes=0 rounded_bytes=0 rounding_ratio=0.000000
fragmentation_candidates upper1536_rounded_bytes=0 upper1536_ratio=0.000000 upper1p5_rounded_bytes=0 upper1p5_ratio=0.000000
fragmentation_by_class class_count=9 allocs=[0,0,0,0,0,0,0,0,0] rounded_bytes=[0,0,0,0,0,0,0,0,0]
medium_route_shadow candidate_count=20000 remote_live_count=9972 requested_bytes=696381158 rounded_bytes=928030720 remote_requested_bytes=345892886 remote_rounded_bytes=462159872
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
medium_stats malloc=20000 create=14 active_reuse=16073 owner_reuse=3830 global_reuse=83 madvise=14 owner_scan=3927 owner_steps=5462 global_scan=97 global_steps=354 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=18283 retain=18283 budget_reject=0 reactivate=18269 exit_drain=14 madvise_fail=0 resident_bytes=0 resident_peak=917504 madvise_ms=0.074 global_lock_ms=0.047 run_lock_ms=3.263 alloc_slot_ms=1.150 free_slot_ms=1.495 alloc_slot=20000 free_slot=20000 lock_elide_alloc=10943 lock_elide_free=9958 lock_elide_mismatch=19092 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.107 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.106 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## medium debug local 2x10k
run=1 ops/s=3748947.249 post_rss=1835008 peak_rss=1835008 minor_faults=66
run_phase=1 alloc_ms=5.233 remote_ms=0.000
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=0 interleaved=0 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=3748947.249 p25=3748947.249 p75=3748947.249 min=3748947.249 max=3748947.249
post_rss median=1835008 min=1835008 max=1835008
peak_rss median=1835008 min=1835008 max=1835008 source=VmHWM_process
page_faults minor_median=66 minor_min=66 minor_max=66
steady_work throughput_median=3821835.215 p25=3821835.215 p75=3821835.215
phase_ms alloc_median=5.233 remote_median=0.000
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
medium_stats malloc=20000 create=6 active_reuse=19991 owner_reuse=0 global_reuse=3 madvise=6 owner_scan=9 owner_steps=0 global_scan=9 global_steps=18 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=19107 retain=19107 budget_reject=0 reactivate=19101 exit_drain=6 madvise_fail=0 resident_bytes=0 resident_peak=393216 madvise_ms=0.032 global_lock_ms=0.028 run_lock_ms=2.191 alloc_slot_ms=1.124 free_slot_ms=1.285 alloc_slot=20000 free_slot=20000 lock_elide_alloc=16598 lock_elide_free=16604 lock_elide_mismatch=6790 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.053 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.053 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## medium release r50 2x50k
run=1 ops/s=8257970.407 post_rss=1703936 peak_rss=2359296 minor_faults=307
run_interleaved=1 work_ms=11.859 work_ops/s=8432387.180 tail_ms=0.284 remote_enqueue=50182 local_free=49818 drain_calls=100347 drain_objects=50182 drain_empty=84020 push_yields=0 finish_yields=161
run=2 ops/s=8663395.715 post_rss=2015232 peak_rss=2752512 minor_faults=228
run_interleaved=2 work_ms=11.370 work_ops/s=8795144.377 tail_ms=0.605 remote_enqueue=49830 local_free=50170 drain_calls=100757 drain_objects=49830 drain_empty=85052 push_yields=0 finish_yields=366
run=3 ops/s=8179179.653 post_rss=2052096 peak_rss=2932736 minor_faults=293
run_interleaved=3 work_ms=12.001 work_ops/s=8332759.067 tail_ms=0.649 remote_enqueue=49887 local_free=50113 drain_calls=100798 drain_objects=49887 drain_empty=85100 push_yields=0 finish_yields=381
summary runs=3 threads=2 iters=50000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=0 class_map_id=p2-v0
throughput median=8257970.407 p25=8257970.407 p75=8663395.715 min=8179179.653 max=8663395.715
post_rss median=2015232 min=1703936 max=2052096
peak_rss median=2752512 min=2359296 max=2932736 source=VmHWM_process
page_faults minor_median=293 minor_min=228 minor_max=307
steady_work throughput_median=8432387.180 p25=8432387.180 p75=8795144.377
interleaved_phase_ms work_median=11.859 tail_median=0.605
interleaved_work remote_enqueue=149899 local_free=150101 drain_calls=301902 drain_objects=149899 drain_empty=254172 push_yields=0 finish_yields=908
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
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.000 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.000 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0
