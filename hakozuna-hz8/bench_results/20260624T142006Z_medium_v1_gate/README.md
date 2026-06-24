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
throughput median=24050233.723 p25=18041978.497 p75=24573447.570 min=14974231.453 max=24954302.434
post_rss median=5050368 min=4595712 max=5242880
peak_rss median=62242816 min=50855936 max=62242816 source=VmHWM_process
page_faults minor_median=7446 minor_min=5523 minor_max=14475
steady_work throughput_median=25397597.806 p25=19612935.771 p75=25994008.024
interleaved_phase_ms work_median=67.779 tail_median=14.366
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=19357446.970 p25=18729034.163 p75=19453874.068 min=18523178.899 max=19557951.665
post_rss median=9797632 min=9392128 max=10080256
peak_rss median=42893312 min=35389440 max=48627712 source=VmHWM_process
page_faults minor_median=6736 minor_min=6319 minor_max=10891
steady_work throughput_median=20286726.506 p25=20140136.581 p75=20420187.735
interleaved_phase_ms work_median=79.190 tail_median=10.170
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=10518706.336 p25=10426268.676 p75=10772559.563 min=10354378.072 max=10941174.985
post_rss median=4808704 min=2342912 max=4857856
peak_rss median=4964352 min=2490368 max=4993024 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=471
steady_work throughput_median=10565669.014 p25=10490872.721 p75=10823440.376
interleaved_phase_ms work_median=151.823 tail_median=19.350
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=262056.731 p25=247438.562 p75=262417.412 min=152164.485 max=263873.374
post_rss median=3387392 min=3133440 max=3534848
peak_rss median=64729088 min=63832064 max=64729088 source=VmHWM_process
page_faults minor_median=15007 minor_min=14936 minor_max=15427
steady_work throughput_median=362952.775 p25=350120.595 p75=366970.019
phase_ms alloc_median=22.175 remote_median=0.488
```

