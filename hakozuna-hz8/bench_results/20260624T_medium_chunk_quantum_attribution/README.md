# Medium Chunk Quantum Attribution

```text
baseline/candidate R3, threads=16, iters=20000, size=4097..65536, remote_pct=50, interleaved=1
candidate macro: H8_MEDIUM_CHUNK_CARVE
```

## baseline_medium_r50_debug

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=6801030.662 p25=6801030.662 p75=6950374.586 min=6598036.503 max=6950374.586
page_faults minor_median=1979 minor_min=1897 minor_max=2909
steady_work throughput_median=7097700.873 p25=7097700.873 p75=7198672.241
interleaved_phase_ms work_median=45.085 tail_median=1.716
medium_stats malloc=960000 create=235 active_reuse=710172 owner_reuse=249193 global_reuse=400 madvise=635 owner_scan=292631 owner_steps=919037 global_scan=635 global_steps=400 free_lookup=960000 route_lookup=0 invalid_owned=0 empty=779315 retain=779315 budget_reject=0 reactivate=778680 exit_drain=635 madvise_fail=0 resident_bytes=0 resident_peak=15400960 madvise_ms=4.283 global_lock_ms=35.033 run_lock_ms=50.028 alloc_slot_ms=246.957 free_slot_ms=185.312 alloc_slot=960000 free_slot=480055 lock_elide_alloc=959365 lock_elide_free=480055 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=480055 remote_free_owner=479945 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=479945 remote_lease_ms=139.020 remote_lease_enter_ms=64.809 remote_lease_exit_ms=74.211 remote_run_lock_ms=0.000 remote_claim=479945 remote_claim_ms=26.338 remote_lockless_claim=479945 remote_lockless_accept=1 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=163 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=397581 remote_qpush=397680 remote_qpush_ms=40.791 remote_collect_call=68058 remote_collect_run=397680 remote_collect_slot=479945 remote_collect_ms=183.057 collect_finish_pending_rearm=322 empty_with_pending=0
medium_remote_class_density slots_per_run=[1.355,1.684,1.596,1.000]
medium_remote_queue push_attempt=397680 push_retry=315 push_success=397680
medium_chunk create=0 alloc=0 reserved_bytes=0 used_bytes=0
medium_residual_budget attributed_ms=910.819 lock_ms=85.061 slot_ms=432.269 qpush_ms=40.791 collect_ms=183.057 lease_ms=139.020 claim_ms=26.338 madvise_ms=4.283 minor_faults_median=1979 minor_faults_per_op=0.006184
```

## chunk_medium_r50_debug

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=6807809.144 p25=6807809.144 p75=6896860.596 min=6627260.424 max=6896860.596
page_faults minor_median=1951 minor_min=1896 minor_max=3003
steady_work throughput_median=7049539.493 p25=7049539.493 p75=7166987.173
interleaved_phase_ms work_median=45.393 tail_median=2.698
medium_stats malloc=960000 create=244 active_reuse=708723 owner_reuse=250637 global_reuse=396 madvise=640 owner_scan=292980 owner_steps=936846 global_scan=640 global_steps=396 free_lookup=960000 route_lookup=0 invalid_owned=0 empty=778391 retain=778391 budget_reject=0 reactivate=777751 exit_drain=640 madvise_fail=0 resident_bytes=0 resident_peak=15990784 madvise_ms=4.259 global_lock_ms=51.030 run_lock_ms=51.253 alloc_slot_ms=248.718 free_slot_ms=187.361 alloc_slot=960000 free_slot=480055 lock_elide_alloc=959360 lock_elide_free=480055 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=480055 remote_free_owner=479945 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=479945 remote_lease_ms=141.430 remote_lease_enter_ms=66.012 remote_lease_exit_ms=75.418 remote_run_lock_ms=0.000 remote_claim=479945 remote_claim_ms=26.427 remote_lockless_claim=479945 remote_lockless_accept=2 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=157 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=396932 remote_qpush=397041 remote_qpush_ms=40.922 remote_collect_call=66959 remote_collect_run=397041 remote_collect_slot=479945 remote_collect_ms=183.448 collect_finish_pending_rearm=322 empty_with_pending=0
medium_remote_class_density slots_per_run=[1.359,1.697,1.602,1.000]
medium_remote_queue push_attempt=397041 push_retry=301 push_success=397041
medium_chunk create=1 alloc=244 reserved_bytes=16777216 used_bytes=15990784
medium_residual_budget attributed_ms=934.847 lock_ms=102.283 slot_ms=436.078 qpush_ms=40.922 collect_ms=183.448 lease_ms=141.430 claim_ms=26.427 madvise_ms=4.259 minor_faults_median=1951 minor_faults_per_op=0.006097
```

## baseline_medium_r50_release

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=7406825.663 p25=7406825.663 p75=8472928.358 min=3151084.766 max=8472928.358
page_faults minor_median=53253 minor_min=37922 minor_max=165428
steady_work throughput_median=7802813.919 p25=7802813.919 p75=8981377.367
interleaved_phase_ms work_median=41.011 tail_median=9.018
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=0 remote_lease_ms=0.000 remote_lease_enter_ms=0.000 remote_lease_exit_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_lockless_claim=0 remote_lockless_accept=0 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=0 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=0 remote_qpush=0 remote_qpush_ms=0.000 remote_collect_call=0 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.000 collect_finish_pending_rearm=0 empty_with_pending=0
medium_remote_class_density slots_per_run=[0.000,0.000,0.000,0.000]
medium_remote_queue push_attempt=0 push_retry=0 push_success=0
medium_chunk create=0 alloc=0 reserved_bytes=0 used_bytes=0
medium_residual_budget attributed_ms=0.000 lock_ms=0.000 slot_ms=0.000 qpush_ms=0.000 collect_ms=0.000 lease_ms=0.000 claim_ms=0.000 madvise_ms=0.000 minor_faults_median=53253 minor_faults_per_op=0.166416
```

## chunk_medium_r50_release

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=9091663.286 p25=9091663.286 p75=13610483.611 min=2004044.902 max=13610483.611
page_faults minor_median=31810 minor_min=3116 minor_max=275410
steady_work throughput_median=9708762.313 p25=9708762.313 p75=15023520.025
interleaved_phase_ms work_median=32.960 tail_median=7.764
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=0 remote_lease_ms=0.000 remote_lease_enter_ms=0.000 remote_lease_exit_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_lockless_claim=0 remote_lockless_accept=0 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=0 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=0 remote_qpush=0 remote_qpush_ms=0.000 remote_collect_call=0 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.000 collect_finish_pending_rearm=0 empty_with_pending=0
medium_remote_class_density slots_per_run=[0.000,0.000,0.000,0.000]
medium_remote_queue push_attempt=0 push_retry=0 push_success=0
medium_chunk create=0 alloc=0 reserved_bytes=0 used_bytes=0
medium_residual_budget attributed_ms=0.000 lock_ms=0.000 slot_ms=0.000 qpush_ms=0.000 collect_ms=0.000 lease_ms=0.000 claim_ms=0.000 madvise_ms=0.000 minor_faults_median=31810 minor_faults_per_op=0.099406
```
