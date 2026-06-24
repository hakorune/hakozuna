# Main Fault Variance Chunk A/B

```text
./h8_bench_release --runs 5 --threads 16 --iters 100000 --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 4096
./h8_bench_release_mediumchunk --runs 5 --threads 16 --iters 100000 --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 4096
```

## baseline_main_interleaved_remote90

```text
run=1 ops/s=2736996.876 post_rss=4374528 peak_rss=36700160 minor_faults=1147088
run_interleaved=1 work_ms=581.406 work_ops/s=2751948.406 tail_ms=11.435 remote_enqueue=1440700 local_free=159300 drain_calls=1637553 drain_objects=1440700 drain_empty=1198415 push_yields=0 finish_yields=32377
run=2 ops/s=17263739.760 post_rss=5218304 peak_rss=36700160 minor_faults=60926
run_interleaved=2 work_ms=90.148 work_ops/s=17748563.839 tail_ms=22.651 remote_enqueue=1439076 local_free=160924 drain_calls=1661738 drain_objects=1439076 drain_empty=1369429 push_yields=0 finish_yields=49312
run=3 ops/s=7947265.128 post_rss=5410816 peak_rss=36700160 minor_faults=281136
run_interleaved=3 work_ms=198.176 work_ops/s=8073634.330 tail_ms=79.275 remote_enqueue=1440286 local_free=159714 drain_calls=1820249 drain_objects=1440286 drain_empty=1476782 push_yields=0 finish_yields=188578
run=4 ops/s=11846772.833 post_rss=5365760 peak_rss=36700160 minor_faults=135687
run_interleaved=4 work_ms=132.438 work_ops/s=12081168.995 tail_ms=51.218 remote_enqueue=1440285 local_free=159715 drain_calls=1769068 drain_objects=1440285 drain_empty=1456919 push_yields=0 finish_yields=143706
run=5 ops/s=6213340.225 post_rss=5238784 peak_rss=36700160 minor_faults=369624
run_interleaved=5 work_ms=254.674 work_ops/s=6282546.511 tail_ms=55.196 remote_enqueue=1439795 local_free=160205 drain_calls=1845253 drain_objects=1439795 drain_empty=1500896 push_yields=0 finish_yields=211648
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
throughput median=7947265.128 p25=6213340.225 p75=11846772.833 min=2736996.876 max=17263739.760
post_rss median=5238784 min=4374528 max=5410816
peak_rss median=36700160 min=36700160 max=36700160 source=VmHWM_process
page_faults minor_median=281136 minor_min=60926 minor_max=1147088
steady_work throughput_median=8073634.330 p25=6282546.511 p75=12081168.995
interleaved_phase_ms work_median=198.176 tail_median=51.218
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=0 remote_lease_ms=0.000 remote_lease_enter_ms=0.000 remote_lease_exit_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_lockless_claim=0 remote_lockless_accept=0 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=0 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=0 remote_qpush=0 remote_qpush_ms=0.000 remote_collect_call=0 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.000 collect_finish_pending_rearm=0 empty_with_pending=0
medium_remote_class pub=[0,0,0,0] qpush=[0,0,0,0] collect_run=[0,0,0,0] collect_slot=[0,0,0,0]
medium_remote_class_density slots_per_run=[0.000,0.000,0.000,0.000]
medium_residual lease_ns_per_pub=0.0 claim_ns_per_pub=0.0 qpush_ns_per_push=0.0 collect_ns_per_run=0.0 collect_runs_per_call=0.000 collect_slots_per_run=0.000
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
```

## chunk_main_interleaved_remote90

```text
run=1 ops/s=24446764.001 post_rss=4435968 peak_rss=27131904 minor_faults=7487
run_interleaved=1 work_ms=62.370 work_ops/s=25653460.169 tail_ms=6.153 remote_enqueue=1440700 local_free=159300 drain_calls=1618400 drain_objects=1440700 drain_empty=1298866 push_yields=0 finish_yields=11511
run=2 ops/s=17663031.269 post_rss=5287936 peak_rss=35741696 minor_faults=50044
run_interleaved=2 work_ms=87.313 work_ops/s=18324852.191 tail_ms=28.044 remote_enqueue=1439076 local_free=160924 drain_calls=1690373 drain_objects=1439076 drain_empty=1371671 push_yields=0 finish_yields=74048
run=3 ops/s=23854752.705 post_rss=5300224 peak_rss=35741696 minor_faults=6012
run_interleaved=3 work_ms=63.840 work_ops/s=25062741.833 tail_ms=6.913 remote_enqueue=1440286 local_free=159714 drain_calls=1616829 drain_objects=1440286 drain_empty=1315908 push_yields=0 finish_yields=11343
run=4 ops/s=6143367.200 post_rss=5128192 peak_rss=38146048 minor_faults=415284
run_interleaved=4 work_ms=257.285 work_ops/s=6218775.987 tail_ms=75.208 remote_enqueue=1440285 local_free=159715 drain_calls=1894038 drain_objects=1440285 drain_empty=1523254 push_yields=0 finish_yields=254725
run=5 ops/s=23593657.294 post_rss=4853760 peak_rss=38146048 minor_faults=5351
run_interleaved=5 work_ms=65.073 work_ops/s=24587758.189 tail_ms=6.593 remote_enqueue=1439795 local_free=160205 drain_calls=1620272 drain_objects=1439795 drain_empty=1305772 push_yields=0 finish_yields=13946
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
throughput median=23593657.294 p25=17663031.269 p75=23854752.705 min=6143367.200 max=24446764.001
post_rss median=5128192 min=4435968 max=5300224
peak_rss median=35741696 min=27131904 max=38146048 source=VmHWM_process
page_faults minor_median=7487 minor_min=5351 minor_max=415284
steady_work throughput_median=24587758.189 p25=18324852.191 p75=25062741.833
interleaved_phase_ms work_median=65.073 tail_median=6.913
medium_stats malloc=0 create=0 active_reuse=0 owner_reuse=0 global_reuse=0 madvise=0 owner_scan=0 owner_steps=0 global_scan=0 global_steps=0 free_lookup=0 route_lookup=0 invalid_owned=0 empty=0 retain=0 budget_reject=0 reactivate=0 exit_drain=0 madvise_fail=0 resident_bytes=0 resident_peak=0 madvise_ms=0.000 global_lock_ms=0.000 run_lock_ms=0.000 alloc_slot_ms=0.000 free_slot_ms=0.000 alloc_slot=0 free_slot=0 lock_elide_alloc=0 lock_elide_free=0 lock_elide_mismatch=0 active_owner_mismatch=0 owner_list_mismatch=0 global_skip_foreign=0 local_free_owner=0 remote_free_owner=0 free_steps=0 route_steps=0 route_authority_mismatch=0 remote_pub=0 remote_lease_ms=0.000 remote_lease_enter_ms=0.000 remote_lease_exit_ms=0.000 remote_run_lock_ms=0.000 remote_claim=0 remote_claim_ms=0.000 remote_lockless_claim=0 remote_lockless_accept=0 remote_lockless_rb_invalid=0 remote_lockless_rb_accept=0 writer_overlap=0 writer_foreign=0 writer_token_change=0 collect_wrong_owner=0 detached_while_attached=0 remote_shadow_attempt=0 remote_shadow_accept=0 remote_shadow_reject=0 remote_shadow_match=0 remote_shadow_mismatch=0 remote_notify=0 remote_qpush=0 remote_qpush_ms=0.000 remote_collect_call=0 remote_collect_run=0 remote_collect_slot=0 remote_collect_ms=0.000 collect_finish_pending_rearm=0 empty_with_pending=0
medium_remote_class pub=[0,0,0,0] qpush=[0,0,0,0] collect_run=[0,0,0,0] collect_slot=[0,0,0,0]
medium_remote_class_density slots_per_run=[0.000,0.000,0.000,0.000]
medium_residual lease_ns_per_pub=0.0 claim_ns_per_pub=0.0 qpush_ns_per_push=0.0 collect_ns_per_run=0.0 collect_runs_per_call=0.000 collect_slots_per_run=0.000
span_commit_lower_bound actual_total=0 actual_per_run=0.0 lower_bound_median=0 excess_ratio=0.000
span_purge runs=0 spans=0 avg_spans_per_run=0.000 max_run=0 singleton_runs=0 madvise_calls=0 madvise_bytes=0 madvise_ms=0.000
```
