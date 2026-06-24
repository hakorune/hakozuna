# Medium Collect Detail

```text
./h8_bench --runs 3 --threads 16 --iters 20000 --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1
```

```text
summary runs=3 threads=16 iters=20000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=1 class_map_id=p2-v0
throughput median=3194787.130 p25=3194787.130 p75=6379263.820 min=2260630.571 max=6379263.820
medium_stats malloc=960000 create=717 active_reuse=685069 owner_reuse=273334 global_reuse=880 madvise=39058 owner_scan=303108 owner_steps=1881255 global_scan=1597 global_steps=880 free_lookup=960000 route_lookup=0 invalid_owned=0 empty=754291 retain=716067 budget_reject=38224 reactivate=715233 exit_drain=834 madvise_fail=0 resident_bytes=0 resident_peak=16777216 madvise_ms=669.194 global_lock_ms=44.574 run_lock_ms=99.645 alloc_slot_ms=200.246 free_slot_ms=416.650 alloc_slot=960000 free_slot=480055 lock_elide_alloc=958403 lock_elide_free=480055 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=480055 remote_free_owner=479945 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=479945 remote_lease_ms=143.169 remote_lease_enter_ms=67.996 remote_lease_exit_ms=75.173 remote_run_lock_ms=0.000 remote_claim=479945 remote_claim_ms=27.971 remote_lockless_claim=479945 remote_lockless_accept=1 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=85 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=384336 remote_qpush=384401 remote_qpush_ms=43.287 remote_collect_call=53819 remote_collect_run=384401 remote_collect_slot=479945 remote_collect_ms=578.547 collect_finish_pending_rearm=221 empty_with_pending=0
medium_collect_detail accepted=479945 rejected=0 reject_ratio=0.000000
medium_remote_class_density slots_per_run=[1.598,1.981,1.679,1.000]
medium_residual lease_ns_per_pub=298.3 claim_ns_per_pub=58.3 qpush_ns_per_push=112.6 collect_ns_per_run=1505.1 collect_runs_per_call=7.142 collect_slots_per_run=1.249
medium_residual_budget attributed_ms=2223.284 lock_ms=144.219 slot_ms=616.896 qpush_ms=43.287 collect_ms=578.547 lease_ms=143.169 claim_ms=27.971 madvise_ms=669.194 minor_faults_median=75933 minor_faults_per_op=0.237291
```
