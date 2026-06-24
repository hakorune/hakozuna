# MediumRunP2SlotDecode-L1
timestamp_utc=20260624T000855Z
sha=13dd9cfe

## Interpretation

```text
status:
  smoke pass
  safety stress pass
  medium slot decode uses shift/mask
  h8_medium_slot_index_from_ptr_checked has no div/idiv in inspected release code

debug r50:
  free_steps=0
  invalid_owned=0
  run_lock_ms=3.362
  alloc_slot_ms=1.171
  free_slot_ms=1.456

release:
  r50 median=8.43M ops/s
  local median=11.96M ops/s
  minor faults remain low

decision:
  p2 slot decode is a safe cleanup with modest positive release signal
  remaining cost is still lock/protocol shape rather than integer division
```

## smoke
arena=68719476736 committed=2228224 owners=68 local=68 remote=32

## safety
safety_stress owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7

## code shape
00000000000048f0 <h8_medium_slot_index_from_ptr_checked>:
    48f0:	f3 0f 1e fa          	endbr64 
    48f4:	48 89 f8             	mov    %rdi,%rax
    48f7:	49 89 d1             	mov    %rdx,%r9
    48fa:	48 85 ff             	test   %rdi,%rdi
    48fd:	74 71                	je     4970 <h8_medium_slot_index_from_ptr_checked+0x80>
    48ff:	48 8b 3f             	mov    (%rdi),%rdi
    4902:	48 85 ff             	test   %rdi,%rdi
    4905:	0f 94 c1             	sete   %cl
    4908:	48 85 f6             	test   %rsi,%rsi
    490b:	0f 94 c2             	sete   %dl
    490e:	08 d1                	or     %dl,%cl
    4910:	41 89 c8             	mov    %ecx,%r8d
    4913:	75 5b                	jne    4970 <h8_medium_slot_index_from_ptr_checked+0x80>
    4915:	8b 50 0c             	mov    0xc(%rax),%edx
    4918:	85 d2                	test   %edx,%edx
    491a:	74 49                	je     4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    491c:	0f b7 48 0a          	movzwl 0xa(%rax),%ecx
    4920:	66 85 c9             	test   %cx,%cx
    4923:	74 40                	je     4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    4925:	48 39 f7             	cmp    %rsi,%rdi
    4928:	77 3b                	ja     4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    492a:	48 29 fe             	sub    %rdi,%rsi
    492d:	0f b7 f9             	movzwl %cx,%edi
    4930:	48 0f af d7          	imul   %rdi,%rdx
    4934:	48 39 d6             	cmp    %rdx,%rsi
    4937:	73 2c                	jae    4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    4939:	0f b7 48 10          	movzwl 0x10(%rax),%ecx
    493d:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
    4944:	48 d3 e0             	shl    %cl,%rax
    4947:	48 f7 d0             	not    %rax
    494a:	48 85 f0             	test   %rsi,%rax
    494d:	75 16                	jne    4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    494f:	48 d3 ee             	shr    %cl,%rsi
    4952:	48 39 f7             	cmp    %rsi,%rdi
    4955:	76 0e                	jbe    4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    4957:	41 b8 01 00 00 00    	mov    $0x1,%r8d
    495d:	4d 85 c9             	test   %r9,%r9
    4960:	74 03                	je     4965 <h8_medium_slot_index_from_ptr_checked+0x75>
    4962:	49 89 31             	mov    %rsi,(%r9)
    4965:	44 89 c0             	mov    %r8d,%eax
    4968:	c3                   	ret    
    4969:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    4970:	45 31 c0             	xor    %r8d,%r8d
    4973:	44 89 c0             	mov    %r8d,%eax
    4976:	c3                   	ret    
    4977:	66 0f 1f 84 00 00 00 00 00 	nopw   0x0(%rax,%rax,1)

    4e09:	e8 e2 fa ff ff       	call   48f0 <h8_medium_slot_index_from_ptr_checked>
    4e0e:	84 c0                	test   %al,%al
    4e10:	74 3e                	je     4e50 <h8_medium_run_free_local_scaffold+0x70>
    4e12:	48 8b 0c 24          	mov    (%rsp),%rcx
    4e16:	41 89 c4             	mov    %eax,%r12d
    4e19:	b8 01 00 00 00       	mov    $0x1,%eax
    4e1e:	48 d3 e0             	shl    %cl,%rax
    4e21:	48 89 c2             	mov    %rax,%rdx
    4e24:	48 23 53 70          	and    0x70(%rbx),%rdx
    4e28:	74 26                	je     4e50 <h8_medium_run_free_local_scaffold+0x70>
    4e2a:	48 89 c2             	mov    %rax,%rdx
    4e2d:	48 23 53 68          	and    0x68(%rbx),%rdx
    4e31:	75 1d                	jne    4e50 <h8_medium_run_free_local_scaffold+0x70>
    4e33:	48 8b 53 30          	mov    0x30(%rbx),%rdx
    4e37:	48 c1 e9 06          	shr    $0x6,%rcx
    4e3b:	48 8b 0c ca          	mov    (%rdx,%rcx,8),%rcx
    4e3f:	48 8b 14 24          	mov    (%rsp),%rdx
    4e43:	48 0f a3 d1          	bt     %rdx,%rcx
    4e47:	73 2f                	jae    4e78 <h8_medium_run_free_local_scaffold+0x98>
    4e49:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    4e50:	45 31 e4             	xor    %r12d,%r12d
    4e53:	48 8b 44 24 08       	mov    0x8(%rsp),%rax
    4e58:	64 48 2b 04 25 28 00 00 00 	sub    %fs:0x28,%rax
    4e61:	0f 85 0a 01 00 00    	jne    4f71 <h8_medium_run_free_local_scaffold+0x191>
    4e67:	48 83 c4 18          	add    $0x18,%rsp
    4e6b:	44 89 e0             	mov    %r12d,%eax
    4e6e:	5b                   	pop    %rbx
    4e6f:	41 5c                	pop    %r12
    4e71:	c3                   	ret    
    4e72:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    4e78:	48 8b 4b 38          	mov    0x38(%rbx),%rcx
    4e7c:	c7 04 91 ff ff ff bf 	movl   $0xbfffffff,(%rcx,%rdx,4)

## medium debug r50 2x10k
run=1 ops/s=2760848.028 post_rss=1818624 peak_rss=1966080 minor_faults=132
run_interleaved=1 work_ms=7.112 work_ops/s=2811974.512 tail_ms=0.202 remote_enqueue=9972 local_free=10028 drain_calls=20303 drain_objects=9972 drain_empty=14583 push_yields=0 finish_yields=169
summary runs=1 threads=2 iters=10000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=1 class_map_id=p2-v0
throughput median=2760848.028 p25=2760848.028 p75=2760848.028 min=2760848.028 max=2760848.028
post_rss median=1818624 min=1818624 max=1818624
peak_rss median=1966080 min=1966080 max=1966080 source=VmHWM_process
page_faults minor_median=132 minor_min=132 minor_max=132
steady_work throughput_median=2811974.512 p25=2811974.512 p75=2811974.512
interleaved_phase_ms work_median=7.112 tail_median=0.202
interleaved_work remote_enqueue=9972 local_free=10028 drain_calls=20303 drain_objects=9972 drain_empty=14583 push_yields=0 finish_yields=169
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
medium_stats malloc=20000 create=11 active_reuse=16112 owner_reuse=3834 global_reuse=43 madvise=11 owner_scan=3888 owner_steps=5493 global_scan=54 global_steps=194 free_lookup=20000 route_lookup=0 invalid_owned=0 empty=18207 retain=18207 budget_reject=0 reactivate=18196 exit_drain=11 madvise_fail=0 resident_bytes=0 resident_peak=720896 madvise_ms=0.062 global_lock_ms=0.049 run_lock_ms=3.362 alloc_slot_ms=1.171 free_slot_ms=1.456 alloc_slot=20000 free_slot=20000 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.101 owner_exit_collect_ms=0.001 owner_exit_span_walk_ms=0.099 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## medium release r50 2x50k
run=1 ops/s=8245908.154 post_rss=1716224 peak_rss=2490368 minor_faults=315
run_interleaved=1 work_ms=11.903 work_ops/s=8401582.892 tail_ms=0.122 remote_enqueue=50182 local_free=49818 drain_calls=100138 drain_objects=50182 drain_empty=83991 push_yields=0 finish_yields=66
run=2 ops/s=8209648.817 post_rss=1990656 peak_rss=3026944 minor_faults=313
run_interleaved=2 work_ms=11.940 work_ops/s=8375518.026 tail_ms=0.817 remote_enqueue=49830 local_free=50170 drain_calls=101026 drain_objects=49830 drain_empty=85714 push_yields=0 finish_yields=500
run=3 ops/s=8478842.997 post_rss=2146304 peak_rss=3039232 minor_faults=249
run_interleaved=3 work_ms=11.610 work_ops/s=8613240.687 tail_ms=0.074 remote_enqueue=49887 local_free=50113 drain_calls=100097 drain_objects=49887 drain_empty=83297 push_yields=0 finish_yields=46
run=4 ops/s=8029633.845 post_rss=2048000 peak_rss=3039232 minor_faults=247
run_interleaved=4 work_ms=12.247 work_ops/s=8165021.619 tail_ms=0.772 remote_enqueue=49992 local_free=50008 drain_calls=100967 drain_objects=49992 drain_empty=85193 push_yields=0 finish_yields=468
run=5 ops/s=8584342.366 post_rss=2109440 peak_rss=3039232 minor_faults=220
run_interleaved=5 work_ms=11.470 work_ops/s=8718528.835 tail_ms=0.351 remote_enqueue=50056 local_free=49944 drain_calls=100460 drain_objects=50056 drain_empty=84112 push_yields=0 finish_yields=223
summary runs=5 threads=2 iters=50000 size=4097..65536 remote_pct=50 interleaved=1 live_window=1024 bench_attribution=0 class_map_id=p2-v0
throughput median=8245908.154 p25=8209648.817 p75=8478842.997 min=8029633.845 max=8584342.366
post_rss median=2048000 min=1716224 max=2146304
peak_rss median=3039232 min=2490368 max=3039232 source=VmHWM_process
page_faults minor_median=249 minor_min=220 minor_max=315
steady_work throughput_median=8401582.892 p25=8375518.026 p75=8613240.687
interleaved_phase_ms work_median=11.903 tail_median=0.351
interleaved_work remote_enqueue=249947 local_free=250053 drain_calls=502688 drain_objects=249947 drain_empty=422307 push_yields=0 finish_yields=1303
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
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.000 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.000 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0

## medium release local 2x50k
run=1 ops/s=12702272.576 post_rss=1703936 peak_rss=1703936 minor_faults=66
run_phase=1 alloc_ms=7.749 remote_ms=0.000
run=2 ops/s=11962861.536 post_rss=2228224 peak_rss=2228224 minor_faults=38
run_phase=2 alloc_ms=8.271 remote_ms=0.000
run=3 ops/s=11954479.732 post_rss=2330624 peak_rss=2490368 minor_faults=38
run_phase=3 alloc_ms=8.293 remote_ms=0.000
run=4 ops/s=12492131.519 post_rss=2445312 peak_rss=2490368 minor_faults=38
run_phase=4 alloc_ms=7.935 remote_ms=0.000
run=5 ops/s=11836542.555 post_rss=2314240 peak_rss=2490368 minor_faults=38
run_phase=5 alloc_ms=8.379 remote_ms=0.000
summary runs=5 threads=2 iters=50000 size=4097..65536 remote_pct=0 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=11962861.536 p25=11954479.732 p75=12492131.519 min=11836542.555 max=12702272.576
post_rss median=2314240 min=1703936 max=2445312
peak_rss median=2490368 min=1703936 max=2490368 source=VmHWM_process
page_faults minor_median=38 minor_min=38 minor_max=66
steady_work throughput_median=12089916.091 p25=12058250.514 p75=12602513.571
phase_ms alloc_median=8.271 remote_median=0.000
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
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 free_steps=0 route_steps=0
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
local_scan_detail hint_null=0 hint_full=0 hint_state_blocked=0 hint_trusted=0 hint_class_mismatch=0 hint_owner_mismatch=0 hint_generation_mismatch=0 hint_state_mismatch=0 scan_usable=0 scan_full=0 scan_state_blocked=0 skip_no_pending=0
local_hot live_alloc=0 live_free=0 word0=0 word1=0 word2_7=0 word8_31=0 word32_63=0 free_head_alloc=0 free_head_free=0 pending_alloc=0 pending_free=0 used_alloc=0 used_free=0
used_count_detail load_alloc=0 store_alloc=0 load_free=0 store_free=0 full_check=0 underflow=0 mirror_mismatch=0 mirror_underflow=0
used_count_cold active_hint=0 owner_scan_locked=0 adoption_locked=0 owner_exit=0 verify_quiescent=0 derived_mismatch=0 derived_scan=0
span_commit_timing total_ms=0.000 meta_ms=0.000 mprotect_ms=0.000
lifecycle_timing owner_exit_total_ms=0.000 owner_exit_collect_ms=0.000 owner_exit_span_walk_ms=0.000 span_retire_count=0 span_retire_total_ms=0.000 span_retire_lock_wait_ms=0.000 span_retire_madvise_ms=0.000 span_retire_meta_free_ms=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
slot_shadow valid_mismatch=0 invalid_mismatch=0 pending_nonallocated=0 free_unreachable=0 free_duplicate=0 free_cycle=0 bad_next=0 never_used_below_bump=0 nonvirgin_above_bump=0 used_mismatch=0 reserved_quiescent=0
