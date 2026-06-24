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
medium_geometry_id=q64-run64k
throughput median=21800066.953 p25=4112321.608 p75=22483810.743 min=2214721.254 max=24578790.578
post_rss median=5238784 min=4435968 max=5763072
peak_rss median=46837760 min=36433920 max=46837760 source=VmHWM_process
page_faults minor_median=80486 minor_min=5852 minor_max=1268917
steady_work throughput_median=22768223.512 p25=4146853.540 p75=23424566.505
interleaved_phase_ms work_median=117.378 tail_median=32.142
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=2186258.994 p25=1784194.598 p75=2389009.383 min=1449779.616 max=3523624.183
post_rss median=10461184 min=9826304 max=10670080
peak_rss median=30457856 min=24436736 max=32358400 source=VmHWM_process
page_faults minor_median=1330822 minor_min=757801 minor_max=1762817
steady_work throughput_median=2193181.159 p25=1789312.811 p75=2397643.304
interleaved_phase_ms work_median=805.962 tail_median=51.939
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=13038159.294 p25=12829261.699 p75=13099148.610 min=12527789.868 max=13332999.231
post_rss median=4923392 min=1966080 max=5255168
peak_rss median=5062656 min=1966080 max=5431296 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=496
steady_work throughput_median=13111597.213 p25=12919932.738 p75=13167846.207
interleaved_phase_ms work_median=122.854 tail_median=11.969
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=140134.770 p25=136064.909 p75=141662.495 min=87391.872 max=145665.822
post_rss median=3756032 min=3416064 max=3969024
peak_rss median=65093632 min=64225280 max=65105920 source=VmHWM_process
page_faults minor_median=15007 minor_min=14937 minor_max=15546
steady_work throughput_median=175422.924 p25=170405.818 p75=176485.407
phase_ms alloc_median=45.749 remote_median=0.540
```

