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
throughput median=13764828.829 p25=13686683.701 p75=13913031.864 min=13625049.736 max=14126828.084
post_rss median=10031104 min=9928704 max=10252288
peak_rss median=32546816 min=30932992 max=32546816 source=VmHWM_process
page_faults minor_median=6718 minor_min=6010 minor_max=7536
steady_work throughput_median=14289971.677 p25=14233991.646 p75=14384903.590
interleaved_phase_ms work_median=111.967 tail_median=13.134
```

## medium_interleaved_remote50_chunk

```text
summary runs=5 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=5617120.759 p25=5495615.243 p75=13930963.058 min=2688478.020 max=13950424.551
post_rss median=10567680 min=10223616 max=10625024
peak_rss median=46534656 min=46534656 max=48771072 source=VmHWM_process
page_faults minor_median=307150 minor_min=6500 minor_max=967960
steady_work throughput_median=5694884.261 p25=5568550.367 p75=14459142.404
interleaved_phase_ms work_median=280.954 tail_median=55.349
```

## main_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=21524855.181 p25=6789392.316 p75=23523728.017 min=5212942.766 max=24600721.828
post_rss median=5128192 min=4419584 max=5349376
peak_rss median=63680512 min=35651584 max=63680512 source=VmHWM_process
page_faults minor_median=8586 minor_min=5662 minor_max=277546
steady_work throughput_median=22970266.827 p25=6942866.931 p75=25007822.760
interleaved_phase_ms work_median=69.655 tail_median=9.382
```

## main_interleaved_remote90_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=24060832.039 p25=23447541.914 p75=24280747.355 min=23264083.862 max=24303069.238
post_rss median=5394432 min=4448256 max=5410816
peak_rss median=29753344 min=29753344 max=32526336 source=VmHWM_process
page_faults minor_median=5849 minor_min=5685 minor_max=7194
steady_work throughput_median=25551246.586 p25=24931464.573 p75=25559817.921
interleaved_phase_ms work_median=62.619 tail_median=9.407
```

## small_guard_local0_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=339984853.675 p25=310443734.692 p75=340089060.823 min=285034478.483 max=348912798.623
post_rss median=4063232 min=1703936 max=4194304
peak_rss median=4063232 min=1703936 max=4194304 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=354
steady_work throughput_median=364194408.796 p25=334178037.945 p75=378499165.646
interleaved_phase_ms work_median=4.393 tail_median=0.674
```

## small_guard_local0_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..2048 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=321350894.892 p25=314941225.078 p75=323168472.976 min=257991945.491 max=339942885.346
post_rss median=4308992 min=1835008 max=4395008
peak_rss median=4308992 min=1835008 max=4395008 source=VmHWM_process
page_faults minor_median=153 minor_min=153 minor_max=337
steady_work throughput_median=374821198.575 p25=367566738.059 p75=379259618.854
interleaved_phase_ms work_median=4.269 tail_median=0.446
```

## small_interleaved_remote90_baseline

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=per-run-mmap
throughput median=54866822.619 p25=53841442.202 p75=56524427.420 min=53428305.709 max=59747919.792
post_rss median=14454784 min=14061568 max=14471168
peak_rss median=26615808 min=23592960 max=27279360 source=VmHWM_process
page_faults minor_median=5450 minor_min=5205 minor_max=5643
steady_work throughput_median=56412709.459 p25=55490165.790 p75=58022841.925
interleaved_phase_ms work_median=28.362 tail_median=10.777
```

## small_interleaved_remote90_chunk

```text
summary runs=5 threads=16 iters=100000 size=16..4096 remote_pct=90 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
medium_arena_id=chunk16m
throughput median=54171782.514 p25=54083246.217 p75=54236986.615 min=53041186.017 max=54542037.611
post_rss median=14442496 min=14118912 max=14483456
peak_rss median=26755072 min=25427968 max=26894336 source=VmHWM_process
page_faults minor_median=5560 minor_min=5071 minor_max=6112
steady_work throughput_median=56157440.473 p25=55573217.419 p75=56219081.262
interleaved_phase_ms work_median=28.491 tail_median=9.684
```

