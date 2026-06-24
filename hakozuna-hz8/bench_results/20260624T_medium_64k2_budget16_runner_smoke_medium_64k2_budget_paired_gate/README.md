# Medium 64K2 Budget Paired Gate

```text
runs: 5
threads: 16
iters: 100000
budget_classes: 16u
baseline: h8_bench_release
candidate: h8_bench_release_medium64k2
```

## medium_interleaved_remote50_baseline

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=13841750.759 p25=12829708.370 p75=14004054.244 min=8071724.456 max=14050623.642
post_rss median=10170368 min=9682944 max=10444800
peak_rss median=59260928 min=30277632 max=71618560 source=VmHWM_process
page_faults minor_median=7354 minor_min=6483 minor_max=78345
steady_work throughput_median=14508096.856 p25=13712653.673 p75=14558161.836
interleaved_phase_ms work_median=110.283 tail_median=11.748
```

## medium_interleaved_remote50_64k2

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
medium_arena_id=per-run-mmap
throughput median=18826640.459 p25=18690361.248 p75=19150818.935 min=4021158.643 max=19206608.840
post_rss median=10129408 min=9670656 max=10559488
peak_rss median=44634112 min=38535168 max=57036800 source=VmHWM_process
page_faults minor_median=9403 minor_min=7324 minor_max=618790
steady_work throughput_median=20024132.584 p25=20015506.513 p75=20307943.564
interleaved_phase_ms work_median=79.904 tail_median=12.638
```

## small_guard_local0_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=323032499.896 p25=322976943.080 p75=328168507.965 min=318503986.774 max=334691343.459
post_rss median=4210688 min=1835008 max=4263936
peak_rss median=4263936 min=1835008 max=4263936 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=337
steady_work throughput_median=368625627.729 p25=347564053.883 p75=372378369.006
interleaved_phase_ms work_median=4.340 tail_median=0.617
```

## small_guard_local0_64k2

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
medium_arena_id=per-run-mmap
throughput median=334530744.316 p25=333138516.705 p75=335145071.734 min=324624206.902 max=339995618.306
post_rss median=4194304 min=1703936 max=4300800
peak_rss median=4194304 min=1703936 max=4300800 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=350
steady_work throughput_median=378413049.196 p25=372378369.006 p75=389676305.635
interleaved_phase_ms work_median=4.228 tail_median=0.523
```

## small_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=55391479.454 p25=53852092.334 p75=56139803.268 min=53390288.313 max=56458356.034
post_rss median=14438400 min=14127104 max=14475264
peak_rss median=27799552 min=25821184 max=27799552 source=VmHWM_process
page_faults minor_median=5434 minor_min=5268 minor_max=6150
steady_work throughput_median=56956913.163 p25=55597164.628 p75=57699296.797
interleaved_phase_ms work_median=28.091 tail_median=11.111
```

## small_interleaved_remote90_64k2

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
medium_arena_id=per-run-mmap
throughput median=54731614.804 p25=53882910.281 p75=55384275.836 min=53300512.654 max=55509948.736
post_rss median=14458880 min=14262272 max=14479360
peak_rss median=25833472 min=23986176 max=26759168 source=VmHWM_process
page_faults minor_median=5306 minor_min=5272 minor_max=5774
steady_work throughput_median=56299045.045 p25=55525061.115 p75=56818159.624
interleaved_phase_ms work_median=28.420 tail_median=11.540
```

