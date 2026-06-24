# MediumRun V1 Gate

```text
runs: 1
threads: 1
iters: 1000
bench: h8_bench_release
```

## main_interleaved_remote90

```text
summary runs=1 threads=1 iters=1000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=3882228.710 p25=3882228.710 p75=3882228.710 min=3882228.710 max=3882228.710
post_rss median=1703936 min=1703936 max=1703936
peak_rss median=1703936 min=1703936 max=1703936 source=VmHWM_process
page_faults minor_median=43 minor_min=43 minor_max=43
steady_work throughput_median=5699304.685 p25=5699304.685 p75=5699304.685
interleaved_phase_ms work_median=0.175 tail_median=0.000
```

## medium_interleaved_remote50

```text
summary runs=1 threads=1 iters=1000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=4256931.348 p25=4256931.348 p75=4256931.348 min=4256931.348 max=4256931.348
post_rss median=1703936 min=1703936 max=1703936
peak_rss median=1703936 min=1703936 max=1703936 source=VmHWM_process
page_faults minor_median=47 minor_min=47 minor_max=47
steady_work throughput_median=6398198.267 p25=6398198.267 p75=6398198.267
interleaved_phase_ms work_median=0.156 tail_median=0.000
```

## medium_local0

```text
summary runs=1 threads=1 iters=1000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=3313265.986 p25=3313265.986 p75=3313265.986 min=3313265.986 max=3313265.986
post_rss median=1703936 min=1703936 max=1703936
peak_rss median=1703936 min=1703936 max=1703936 source=VmHWM_process
page_faults minor_median=46 minor_min=46 minor_max=46
steady_work throughput_median=5133575.638 p25=5133575.638 p75=5133575.638
interleaved_phase_ms work_median=0.195 tail_median=0.000
```

## medium_phase_remote90

```text
summary runs=1 threads=1 iters=1000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=88032.764 p25=88032.764 p75=88032.764 min=88032.764 max=88032.764
post_rss median=2355200 min=2355200 max=2355200
peak_rss median=9699328 min=9699328 max=9699328 source=VmHWM_process
page_faults minor_median=2182 minor_min=2182 minor_max=2182
steady_work throughput_median=103064.244 p25=103064.244 p75=103064.244
phase_ms alloc_median=9.703 remote_median=0.960
```
