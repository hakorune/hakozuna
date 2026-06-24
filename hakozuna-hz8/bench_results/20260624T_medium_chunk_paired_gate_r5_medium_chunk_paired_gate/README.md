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
throughput median=2265400.434 p25=2175195.442 p75=2525850.382 min=1497954.794 max=2942165.363
post_rss median=10125312 min=9707520 max=10547200
peak_rss median=32595968 min=23646208 max=32595968 source=VmHWM_process
page_faults minor_median=1260289 minor_min=904279 minor_max=1749121
steady_work throughput_median=2276433.941 p25=2180574.515 p75=2533555.750
interleaved_phase_ms work_median=702.854 tail_median=30.285
```

## medium_interleaved_remote50_chunk

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=1848556.199 p25=1610073.969 p75=2396981.930 min=1430064.383 max=2829217.650
post_rss median=10641408 min=10289152 max=10825728
peak_rss median=30724096 min=30724096 max=35823616 source=VmHWM_process
page_faults minor_median=1315417 minor_min=957271 minor_max=1921356
steady_work throughput_median=1854924.825 p25=1613448.776 p75=2406414.803
interleaved_phase_ms work_median=862.569 tail_median=30.765
```

## main_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=6603081.169 p25=3314248.475 p75=22841736.432 min=2788701.786 max=24048665.240
post_rss median=5185536 min=4685824 max=5402624
peak_rss median=36728832 min=36728832 max=39661568 source=VmHWM_process
page_faults minor_median=366657 minor_min=6431 minor_max=1102857
steady_work throughput_median=6687761.371 p25=3334106.943 p75=23917357.279
interleaved_phase_ms work_median=239.243 tail_median=26.793
```

## main_interleaved_remote90_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=8245095.693 p25=4550628.520 p75=24420483.981 min=2449656.707 max=25129730.269
post_rss median=5046272 min=4468736 max=5218304
peak_rss median=42442752 min=26083328 max=42442752 source=VmHWM_process
page_faults minor_median=243331 minor_min=6225 minor_max=1241215
steady_work throughput_median=8345580.255 p25=4589011.842 p75=25472531.783
interleaved_phase_ms work_median=191.718 tail_median=46.789
```

## small_guard_local0_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=337229102.281 p25=335911242.172 p75=340137428.276 min=329284099.324 max=344919986.108
post_rss median=4182016 min=1966080 max=4272128
peak_rss median=4194304 min=1966080 max=4272128 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=337
steady_work throughput_median=382684666.878 p25=380212776.575 p75=384452546.782
interleaved_phase_ms work_median=4.181 tail_median=0.499
```

## small_guard_local0_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=341252379.169 p25=332833597.555 p75=346408898.032 min=331363309.425 max=349140601.320
post_rss median=4132864 min=1966080 max=4194304
peak_rss median=4194304 min=1966080 max=4194304 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=337
steady_work throughput_median=387591547.307 p25=383000430.157 p75=387704344.421
interleaved_phase_ms work_median=4.128 tail_median=0.426
```

## small_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=53956493.059 p25=53044330.141 p75=56611615.762 min=51991417.517 max=59527857.772
post_rss median=14442496 min=14127104 max=14495744
peak_rss median=27021312 min=25821184 max=30318592 source=VmHWM_process
page_faults minor_median=6115 minor_min=5330 minor_max=6334
steady_work throughput_median=55197069.753 p25=54371425.206 p75=58191882.203
interleaved_phase_ms work_median=28.987 tail_median=10.593
```

## small_interleaved_remote90_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=53799732.178 p25=51805605.036 p75=53948417.221 min=51607168.171 max=54035453.809
post_rss median=14491648 min=14123008 max=14548992
peak_rss median=31035392 min=24117248 max=31035392 source=VmHWM_process
page_faults minor_median=5783 minor_min=5180 minor_max=6562
steady_work throughput_median=55446858.535 p25=53356285.429 p75=55480339.241
interleaved_phase_ms work_median=28.856 tail_median=9.999
```

