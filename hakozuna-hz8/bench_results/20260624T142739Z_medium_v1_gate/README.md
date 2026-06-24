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
throughput median=25209938.520 p25=24242270.708 p75=25384761.994 min=18785213.102 max=25730500.157
post_rss median=5115904 min=4337664 max=5525504
peak_rss median=34603008 min=34603008 max=45129728 source=VmHWM_process
page_faults minor_median=5909 minor_min=5683 minor_max=10277
steady_work throughput_median=26544644.475 p25=25538970.849 p75=26812471.546
interleaved_phase_ms work_median=61.549 tail_median=7.729
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=32078022.733 p25=28235729.446 p75=32447165.419 min=4289230.030 max=32972038.784
post_rss median=9916416 min=9564160 max=10223616
peak_rss median=59973632 min=42991616 max=60203008 source=VmHWM_process
page_faults minor_median=6403 minor_min=5499 minor_max=332751
steady_work throughput_median=34441167.200 p25=31408164.647 p75=35055992.856
interleaved_phase_ms work_median=46.574 tail_median=5.840
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=109603342.080 p25=105678858.198 p75=113266854.657 min=101185885.935 max=116383518.901
post_rss median=4808704 min=2093056 max=4927488
peak_rss median=4927488 min=2228224 max=5038080 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=499
steady_work throughput_median=114587546.067 p25=111488367.617 p75=121972146.288
interleaved_phase_ms work_median=14.154 tail_median=1.420
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=257207.774 p25=250738.975 p75=260243.459 min=150120.059 max=262608.326
post_rss median=3145728 min=2977792 max=3395584
peak_rss median=64618496 min=63832064 max=64618496 source=VmHWM_process
page_faults minor_median=15007 minor_min=14936 minor_max=15428
steady_work throughput_median=360448.118 p25=348091.575 p75=365866.704
phase_ms alloc_median=22.322 remote_median=0.471
```

