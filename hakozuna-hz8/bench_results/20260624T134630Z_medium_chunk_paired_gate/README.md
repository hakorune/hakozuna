# Medium Chunk Paired Gate

```text
runs: 5
threads: 16
iters: 100000
baseline: h8_bench_release
candidate: h8_bench_release_mediumchunk
```

## medium_interleaved_remote50_baseline

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=2688537.701 p25=2449037.815 p75=3471971.439 min=2385830.520 max=3554797.104
post_rss median=10080256 min=9838592 max=10563584
peak_rss median=28368896 min=24641536 max=28368896 source=VmHWM_process
page_faults minor_median=962937 minor_min=720708 minor_max=1182004
steady_work throughput_median=2699433.903 p25=2458758.846 p75=3488461.320
interleaved_phase_ms work_median=592.717 tail_median=36.761
```

## medium_interleaved_remote50_chunk

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=1500675.538 p25=1477045.471 p75=2106892.917 min=1174639.639 max=4712269.210
post_rss median=10625024 min=10428416 max=11026432
peak_rss median=32919552 min=30035968 max=37990400 source=VmHWM_process
page_faults minor_median=1776124 minor_min=547051 minor_max=2117867
steady_work throughput_median=1504138.519 p25=1479947.753 p75=2114161.715
interleaved_phase_ms work_median=1063.732 tail_median=45.117
```

## main_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=10655645.249 p25=3349968.741 p75=15408970.893 min=3066837.023 max=21215082.725
post_rss median=5177344 min=4608000 max=5525504
peak_rss median=36298752 min=36298752 max=39288832 source=VmHWM_process
page_faults minor_median=146178 minor_min=17731 minor_max=1002176
steady_work throughput_median=10853422.226 p25=3372841.797 p75=15859435.602
interleaved_phase_ms work_median=147.419 tail_median=26.788
```

## main_interleaved_remote90_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=24424243.119 p25=21296983.358 p75=24628004.384 min=6831764.715 max=25519223.695
post_rss median=5103616 min=4468736 max=5488640
peak_rss median=45727744 min=25821184 max=45727744 source=VmHWM_process
page_faults minor_median=6143 minor_min=5634 minor_max=310343
steady_work throughput_median=25486853.459 p25=22249840.764 p75=25724356.892
interleaved_phase_ms work_median=62.777 tail_median=8.805
```

## small_guard_local0_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=324032782.397 p25=269354963.884 p75=345341654.053 min=215994821.524 max=353099552.005
post_rss median=4190208 min=1835008 max=4235264
peak_rss median=4194304 min=1835008 max=4235264 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=347
steady_work throughput_median=360853282.694 p25=356173318.389 p75=373990022.414
interleaved_phase_ms work_median=4.434 tail_median=0.600
```

## small_guard_local0_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=345868920.003 p25=325404116.321 p75=390382250.100 min=260329042.894 max=394237042.907
post_rss median=4227072 min=1966080 max=4276224
peak_rss median=4276224 min=1966080 max=4276224 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=337
steady_work throughput_median=432467614.257 p25=432198344.464 p75=437244126.787
interleaved_phase_ms work_median=3.700 tail_median=0.584
```

## small_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=53102990.263 p25=52635564.706 p75=54129094.168 min=50910006.826 max=60434221.392
post_rss median=14430208 min=14118912 max=14548992
peak_rss median=28053504 min=24248320 max=28053504 source=VmHWM_process
page_faults minor_median=5726 minor_min=5301 minor_max=5825
steady_work throughput_median=54650428.704 p25=54089475.351 p75=55664381.354
interleaved_phase_ms work_median=29.277 tail_median=9.506
```

## small_interleaved_remote90_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=53909420.179 p25=53750870.092 p75=54155656.286 min=53425574.368 max=55040761.984
post_rss median=14462976 min=14123008 max=14479360
peak_rss median=27664384 min=24248320 max=27664384 source=VmHWM_process
page_faults minor_median=5481 minor_min=5326 minor_max=5828
steady_work throughput_median=55406880.426 p25=55361705.008 p75=55419322.161
interleaved_phase_ms work_median=28.877 tail_median=9.550
```

