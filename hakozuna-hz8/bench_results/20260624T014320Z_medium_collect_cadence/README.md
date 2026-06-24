# Medium Owner Collect Cadence

timestamp_utc: 20260624T014320Z_medium_collect_cadence
commit_base: 0381b66c21d64dee6141320da000ca922f9ebcd9

## Summary

`MediumRunOwnerCollectCadence-L1` removes the unconditional medium malloc-entry
full drain and replaces it with:

```text
active allocation success:
  periodic budgeted maintenance
  period = 8 medium malloc calls
  budget = 4 queued runs

capacity miss before global reuse/create:
  full medium pending drain

owner exit:
  full medium pending drain
```

Result versus `MediumRunRemoteCostAudit-L1` baseline:

```text
debug r50:
  remote_collect_call:
    20002 -> 2511

  remote_collect_run:
    9657 -> 8671

  remote_collect_slot:
    9972 -> 9972

  remote_collect_ms:
    2.390 -> 1.120

  remote_notify / remote_qpush:
    9657 -> 8671

release r50 median:
  about 5.18M -> about 5.42M ops/s
```

Interpretation:

```text
correctness:
  smoke / safety pass
  active_owner_mismatch=0
  owner_list_mismatch=0
  invalid_owned=0

mechanism:
  eager empty collect checks are substantially reduced
  queued work coalesces more, but 1-slot / 2-slot medium runs still limit reuse
  capacity-miss full drain prevents material run-create growth in this probe

remaining bottleneck:
  release improvement is real but modest
  next measurement should split queue push time and run-lock / route authority
```

Next candidates:

```text
MediumRunRemoteCadenceReaudit-L1:
  add queue push wait/time
  class-split remote_notify / remote_qpush
  slots per collected run

MediumRunRouteSlotAuthority-L1:
  VALID = slot_state ALLOCATED && pending bit 0
  free_mask / allocated_mask become accounting, not route authority

MediumRunOwnerLocalLockElision-L1:
  after route authority cleanup
```

## Smoke / Safety
arena=68719476736 committed=2228224 owners=68 local=68 remote=32
safety_stress owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7

## Debug medium r50
run=1 ops/s=2473061.865 post_rss=1871872 peak_rss=2359296 minor_faults=250
run_interleaved=1 work_ms=7.866 work_ops/s=2542692.119 tail_ms=0.498 remote_enqueue=9972 local_free=10028 drain_calls=20684 drain_objects=9972 drain_empty=15295 push_yields=0 finish_yields=443
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=1 class_map_id=p2-v0
throughput median=2473061.865 p25=2473061.865 p75=2473061.865 min=2473061.865 max=2473061.865
post_rss median=1871872 min=1871872 max=1871872
peak_rss median=2359296 min=2359296 max=2359296 source=VmHWM_process
page_faults minor_median=250 minor_min=250 minor_max=250
steady_work throughput_median=2542692.119 p25=2542692.119 p75=2542692.119
interleaved_phase_ms work_median=7.866 tail_median=0.498
interleaved_work remote_enqueue=9972 local_free=10028 drain_calls=20684 drain_objects=9972 drain_empty=15295 push_yields=0 finish_yields=443
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
medium_stats malloc=20000 create=23 active_reuse=14463 owner_reuse=5514 global_reuse=0 madvise=23 owner_scan=5647 owner_steps=13589 global_scan=23 global_steps=246 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=16733 retain=16733 budget_reject=0 reactivate=16710 exit_drain=23 madvise_fail=0 resident_bytes=0 resident_peak=1507328 madvise_ms=0.131 global_lock_ms=0.088 run_lock_ms=1.850 alloc_slot_ms=0.985 free_slot_ms=0.547 alloc_slot=20000 free_slot=10028 lock_elide_alloc=25613 lock_elide_free=10028 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=246 local_free_owner=10028 remote_free_owner=9972 free_steps=0 route_steps=0 remote_pub=9972 remote_lease_ms=0.800 remote_run_lock_ms=0.550 remote_claim=9972 remote_claim_ms=0.370 remote_notify=8671 remote_qpush=8671 remote_collect_call=2511 remote_collect_run=8671 remote_collect_slot=9972 remote_collect_ms=1.120
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.212 owner_exit_collect_ms=0.001 owner_exit_span_walk_ms=0.210 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## Release medium r50
run=1 ops/s=4982211.760 post_rss=1753088 peak_rss=3014656 minor_faults=483
run_interleaved=1 work_ms=19.691 work_ops/s=5078435.677 tail_ms=0.615 remote_enqueue=50182 local_free=49818 drain_calls=100466 drain_objects=50182 drain_empty=86923 push_yields=0 finish_yields=234
run=2 ops/s=5213195.453 post_rss=2097152 peak_rss=3588096 minor_faults=404
run_interleaved=2 work_ms=18.878 work_ops/s=5297196.003 tail_ms=0.096 remote_enqueue=49830 local_free=50170 drain_calls=100093 drain_objects=49830 drain_empty=85556 push_yields=0 finish_yields=46
run=3 ops/s=5420408.216 post_rss=1933312 peak_rss=3588096 minor_faults=355
run_interleaved=3 work_ms=18.203 work_ops/s=5493606.897 tail_ms=0.524 remote_enqueue=49887 local_free=50113 drain_calls=100422 drain_objects=49887 drain_empty=86663 push_yields=0 finish_yields=220
run=4 ops/s=5503776.636 post_rss=2142208 peak_rss=3768320 minor_faults=430
run_interleaved=4 work_ms=17.895 work_ops/s=5588100.029 tail_ms=0.884 remote_enqueue=49992 local_free=50008 drain_calls=100751 drain_objects=49992 drain_empty=85957 push_yields=0 finish_yields=381
run=5 ops/s=5425334.565 post_rss=2240512 peak_rss=3768320 minor_faults=395
run_interleaved=5 work_ms=18.160 work_ops/s=5506731.043 tail_ms=1.263 remote_enqueue=50056 local_free=49944 drain_calls=101016 drain_objects=50056 drain_empty=87513 push_yields=0 finish_yields=521
summary runs=5 threads=2 iters=50000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=0 class_map_id=p2-v0
throughput median=5420408.216 p25=5213195.453 p75=5425334.565 min=4982211.760 max=5503776.636
post_rss median=2097152 min=1753088 max=2240512
peak_rss median=3588096 min=3014656 max=3768320 source=VmHWM_process
page_faults minor_median=404 minor_min=355 minor_max=483
steady_work throughput_median=5493606.897 p25=5297196.003 p75=5506731.043
interleaved_phase_ms work_median=18.203 tail_median=0.615
interleaved_work remote_enqueue=249947 local_free=250053 drain_calls=502748 drain_objects=249947 drain_empty=432612 push_yields=0 finish_yields=1402
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

## Release medium local
run=1 ops/s=22434432.007 post_rss=1572864 peak_rss=1572864 minor_faults=67
run_phase=1 alloc_ms=4.338 remote_ms=0.000
run=2 ops/s=22975702.735 post_rss=2228224 peak_rss=2228224 minor_faults=38
run_phase=2 alloc_ms=4.245 remote_ms=0.000
run=3 ops/s=11856937.512 post_rss=2359296 peak_rss=2359296 minor_faults=38
run_phase=3 alloc_ms=8.349 remote_ms=0.000
summary runs=3 threads=2 iters=50000 size=4097..65536 remote_pct=0 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=22434432.007 p25=22434432.007 p75=22975702.735 min=11856937.512 max=22975702.735
post_rss median=2228224 min=1572864 max=2359296
peak_rss median=2228224 min=1572864 max=2359296 source=VmHWM_process
page_faults minor_median=38 minor_min=38 minor_max=67
steady_work throughput_median=23054569.012 p25=23054569.012 p75=23559523.608
phase_ms alloc_median=4.338 remote_median=0.000
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

## Small quick guard local
run=1 ops/s=257028855.666 post_rss=1966080 peak_rss=1966080 minor_faults=381
run_phase=1 alloc_ms=4.609 remote_ms=0.000
run=2 ops/s=313920428.627 post_rss=4325376 peak_rss=4325376 minor_faults=171
run_phase=2 alloc_ms=4.768 remote_ms=0.000
run=3 ops/s=355412758.607 post_rss=4448256 peak_rss=4448256 minor_faults=153
run_phase=3 alloc_ms=4.144 remote_ms=0.000
summary runs=3 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=313920428.627 p25=313920428.627 p75=355412758.607 min=257028855.666 max=355412758.607
post_rss median=4325376 min=1966080 max=4448256
peak_rss median=4325376 min=1966080 max=4448256 source=VmHWM_process
page_faults minor_median=171 minor_min=153 minor_max=381
steady_work throughput_median=347173852.948 p25=347173852.948 p75=386106349.133
phase_ms alloc_median=4.609 remote_median=0.000
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

## Small quick interleaved r90
run=1 ops/s=58018501.375 post_rss=2732032 peak_rss=19922944 minor_faults=4746
run_interleaved=1 work_ms=26.621 work_ops/s=60103497.471 tail_ms=13.681 remote_enqueue=1440700 local_free=159300 drain_calls=1608906 drain_objects=1440700 drain_empty=1591186 push_yields=0 finish_yields=7545
run=2 ops/s=53771045.777 post_rss=3039232 peak_rss=21999616 minor_faults=4945
run_interleaved=2 work_ms=28.758 work_ops/s=55636245.498 tail_ms=13.268 remote_enqueue=1439076 local_free=160924 drain_calls=1612920 drain_objects=1439076 drain_empty=1597722 push_yields=472 finish_yields=11434
run=3 ops/s=55372208.177 post_rss=3051520 peak_rss=21999616 minor_faults=4612
run_interleaved=3 work_ms=28.061 work_ops/s=57018146.239 tail_ms=15.583 remote_enqueue=1440286 local_free=159714 drain_calls=1609554 drain_objects=1440286 drain_empty=1591459 push_yields=0 finish_yields=8709
summary runs=3 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=1024 bench_attribution=0 class_map_id=p2-v0
throughput median=55372208.177 p25=55372208.177 p75=58018501.375 min=53771045.777 max=58018501.375
post_rss median=3039232 min=2732032 max=3051520
peak_rss median=21999616 min=19922944 max=21999616 source=VmHWM_process
page_faults minor_median=4746 minor_min=4612 minor_max=4945
steady_work throughput_median=57018146.239 p25=57018146.239 p75=60103497.471
interleaved_phase_ms work_median=28.061 tail_median=13.681
interleaved_work remote_enqueue=4320062 local_free=479938 drain_calls=4831380 drain_objects=4320062 drain_empty=4780367 push_yields=472 finish_yields=27688
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
