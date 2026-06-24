# Medium SizePolicy Paired Gate

```text
runs: 5
threads: 16
iters: 100000
phase_threads: 2
phase_iters: 4000
baseline: h8_bench_release
candidate: h8_bench_release_mediumupper48
```

## medium_interleaved_remote50_baseline

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=13883508.065 p25=8991943.202 p75=13917029.679 min=2999701.687 max=14036791.096
post_rss median=10125312 min=9752576 max=10477568
peak_rss median=48611328 min=45649920 max=48611328 source=VmHWM_process
page_faults minor_median=6953 minor_min=6691 minor_max=781046
steady_work throughput_median=14406991.439 p25=9261736.130 p75=14505322.275
interleaved_phase_ms work_median=111.057 tail_median=15.471
```

## medium_interleaved_remote50_upper48

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-upper48
medium_arena_id=per-run-mmap
throughput median=14637319.695 p25=14314993.944 p75=14728029.763 min=6694357.542 max=14932833.979
post_rss median=10276864 min=10014720 max=10354688
peak_rss median=35016704 min=24903680 max=35016704 source=VmHWM_process
page_faults minor_median=5335 minor_min=5085 minor_max=279205
steady_work throughput_median=15078825.787 p25=14810932.979 p75=15237417.536
interleaved_phase_ms work_median=106.109 tail_median=12.685
```

## medium_phase_remote90_baseline

```text
summary runs=5 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=140710.095 p25=116225.967 p75=144298.351 min=87669.044 max=145130.393
post_rss median=3592192 min=3526656 max=3645440
peak_rss median=64667648 min=64356352 max=64868352 source=VmHWM_process
page_faults minor_median=15007 minor_min=14939 minor_max=15546
steady_work throughput_median=176096.506 p25=139870.449 p75=182183.165
phase_ms alloc_median=45.430 remote_median=0.560
```

## medium_phase_remote90_upper48

```text
summary runs=5 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-upper48
medium_arena_id=per-run-mmap
throughput median=188848.157 p25=172418.973 p75=189426.627 min=95160.021 max=189700.789
post_rss median=3653632 min=3379200 max=3747840
peak_rss median=64671744 min=64094208 max=64827392 source=VmHWM_process
page_faults minor_median=14946 minor_min=14873 minor_max=15479
steady_work throughput_median=259871.233 p25=232831.371 p75=263803.476
phase_ms alloc_median=30.784 remote_median=0.495
```

## small_guard_local0_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=334122698.208 p25=317332440.043 p75=344657411.610 min=264553676.453 max=350138293.684
post_rss median=4112384 min=1835008 max=4194304
peak_rss median=4194304 min=1835008 max=4194304 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=352
steady_work throughput_median=385099999.639 p25=350732669.586 p75=386616870.414
interleaved_phase_ms work_median=4.155 tail_median=0.452
```

## small_guard_local0_upper48

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-upper48
medium_arena_id=per-run-mmap
throughput median=347269323.639 p25=338578549.906 p75=349597864.132 min=334189834.029 max=350038482.356
post_rss median=4112384 min=1835008 max=4194304
peak_rss median=4194304 min=1835008 max=4194304 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=337
steady_work throughput_median=384258652.184 p25=383684396.563 p75=390643788.288
interleaved_phase_ms work_median=4.164 tail_median=0.357
```

## small_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=55900236.804 p25=53588748.024 p75=56771280.999 min=46579559.591 max=59439183.870
post_rss median=14422016 min=14123008 max=14553088
peak_rss median=26611712 min=23986176 max=40988672 source=VmHWM_process
page_faults minor_median=5470 minor_min=5130 minor_max=9042
steady_work throughput_median=57654075.833 p25=54974913.573 p75=58321918.033
interleaved_phase_ms work_median=27.752 tail_median=9.804
```

## small_interleaved_remote90_upper48

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-upper48
medium_arena_id=per-run-mmap
throughput median=48404394.514 p25=42651969.367 p75=54358644.270 min=41301991.592 max=54399270.723
post_rss median=14819328 min=14213120 max=14884864
peak_rss median=58195968 min=58195968 max=58195968 source=VmHWM_process
page_faults minor_median=11024 minor_min=5314 minor_max=14112
steady_work throughput_median=51432073.034 p25=45304296.073 p75=55935892.433
interleaved_phase_ms work_median=31.109 tail_median=10.475
```

