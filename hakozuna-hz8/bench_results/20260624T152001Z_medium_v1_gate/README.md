# MediumRun V1 Gate

```text
runs: 10
threads: 16
iters: 100000
phase_threads: 2
phase_iters: 4000
bench: h8_bench_release
```

## main_interleaved_remote90

```text
summary runs=10 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=25839335.919 p25=23069077.816 p75=26304206.537 min=19444885.702 max=26574937.953
post_rss median=5197824 min=4886528 max=5660672
peak_rss median=44126208 min=30408704 max=44126208 source=VmHWM_process
page_faults minor_median=5307 minor_min=4571 minor_max=10050
steady_work throughput_median=27033476.703 p25=24613119.072 p75=27566670.207
interleaved_phase_ms work_median=60.068 tail_median=9.871
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=33128253.570 p25=28775855.331 p75=33676520.936 min=26471215.564 max=34017561.864
post_rss median=9953280 min=9441280 max=10199040
peak_rss median=47841280 min=47841280 max=49266688 source=VmHWM_process
page_faults minor_median=5802 minor_min=4992 minor_max=11723
steady_work throughput_median=35820967.432 p25=32183286.391 p75=36163084.935
interleaved_phase_ms work_median=45.353 tail_median=5.040
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=102541267.413 p25=96619670.255 p75=103654031.348 min=80427528.614 max=105661501.756
post_rss median=4689920 min=2359296 max=4861952
peak_rss median=4767744 min=2359296 max=4861952 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=475
steady_work throughput_median=107891886.935 p25=106418137.748 p75=108488367.165
interleaved_phase_ms work_median=14.889 tail_median=2.257
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=262531.886 p25=242638.367 p75=264090.499 min=148791.996 max=266051.281
post_rss median=3579904 min=2932736 max=3854336
peak_rss median=64933888 min=63832064 max=64933888 source=VmHWM_process
page_faults minor_median=15007 minor_min=14936 minor_max=15427
steady_work throughput_median=367323.321 p25=335726.180 p75=368688.315
phase_ms alloc_median=21.818 remote_median=0.474
```

