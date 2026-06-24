# Medium R50 Residual Attribution

```text
debug command: ./h8_bench --runs 3 --threads 16 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
release command: ./h8_bench_release --runs 3 --threads 16 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
```

## Debug

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=6124202.775 p25=6124202.775 p75=6168015.152 min=5358448.560 max=6168015.152
page_faults minor_median=3138 minor_min=2639 minor_max=5094
steady_work throughput_median=6421997.396 p25=6421997.396 p75=6442382.372
interleaved_phase_ms work_median=49.829 tail_median=5.345
medium_stats malloc=960000 create=392 active_reuse=698446 owner_reuse=260635 global_reuse=527 madvise=1136 owner_scan=296392 owner_steps=1130967 global_scan=919 global_steps=527 free_lookup=960000 route_lookup=0 invalid_owned=0 empty=769648 retain=769330 budget_reject=318 reactivate=768512 exit_drain=818 madvise_fail=0 resident_bytes=0 resident_peak=16777216 madvise_ms=9.656 global_lock_ms=42.629 run_lock_ms=57.884 alloc_slot_ms=234.496 free_slot_ms=274.086 alloc_slot=960000 free_slot=480055 lock_elide_alloc=959081 lock_elide_free=480055 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=480055 remote_free_owner=479945 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=479945 remote_lease_ms=137.376 remote_lease_enter_ms=63.795 remote_lease_exit_ms=73.581 remote_run_lock_ms=0.000 remote_claim=479945 remote_claim_ms=26.422 remote_lockless_claim=479945 remote_lockless_accept=1 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=118 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=392735 remote_qpush=392825 remote_qpush_ms=34.365 remote_collect_call=60203 remote_collect_run=392825 remote_collect_slot=479945 remote_collect_ms=316.914 collect_finish_pending_rearm=271 empty_with_pending=0
medium_remote_class pub=[31949,64177,128050,255769] qpush=[22375,36007,78674,255769] collect_run=[22375,36007,78674,255769] collect_slot=[31949,64177,128050,255769]
medium_remote_class_density slots_per_run=[1.428,1.782,1.628,1.000]
medium_remote_queue push_attempt=392825 push_retry=291 push_success=392825
medium_residual lease_ns_per_pub=286.2 claim_ns_per_pub=55.1 qpush_ns_per_push=87.5 collect_ns_per_run=806.8 collect_runs_per_call=6.525 collect_slots_per_run=1.222
medium_residual_budget attributed_ms=1133.828 lock_ms=100.512 slot_ms=508.582 qpush_ms=34.365 collect_ms=316.914 lease_ms=137.376 claim_ms=26.422 madvise_ms=9.656 minor_faults_median=3138 minor_faults_per_op=0.009806
```

## Release

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
throughput median=5774021.576 p25=5774021.576 p75=12383102.545 min=3334754.390 max=12383102.545
page_faults minor_median=57759 minor_min=5734 minor_max=159836
steady_work throughput_median=5996922.042 p25=5996922.042 p75=13611858.617
interleaved_phase_ms work_median=53.361 tail_median=4.658
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=0 remote_lease_ms=0.000 remote_lease_enter_ms=0.000 remote_lease_exit_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_lockless_claim=0 remote_lockless_accept=0 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=0 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=0 remote_qpush=0 remote_qpush_ms=0.000 remote_collect_call=0 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.000 collect_finish_pending_rearm=0 empty_with_pending=0
medium_remote_class pub=[0,0,0,0] qpush=[0,0,0,0] collect_run=[0,0,0,0] collect_slot=[0,0,0,0]
medium_remote_class_density slots_per_run=[0.000,0.000,0.000,0.000]
medium_remote_queue push_attempt=0 push_retry=0 push_success=0
medium_residual lease_ns_per_pub=0.0 claim_ns_per_pub=0.0 qpush_ns_per_push=0.0 collect_ns_per_run=0.0 collect_runs_per_call=0.000 collect_slots_per_run=0.000
medium_residual_budget attributed_ms=0.000 lock_ms=0.000 slot_ms=0.000 qpush_ms=0.000 collect_ms=0.000 lease_ms=0.000 claim_ms=0.000 madvise_ms=0.000 minor_faults_median=57759 minor_faults_per_op=0.180497
```
