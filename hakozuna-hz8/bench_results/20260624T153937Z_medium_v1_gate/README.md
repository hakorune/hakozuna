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
throughput median=25798132.547 p25=22626115.991 p75=26046152.676 min=16870553.990 max=26357125.075
post_rss median=5107712 min=4763648 max=5320704
peak_rss median=32899072 min=32899072 max=50589696 source=VmHWM_process
page_faults minor_median=5763 minor_min=4760 minor_max=11556
steady_work throughput_median=26933517.675 p25=24505993.255 p75=27239860.987
interleaved_phase_ms work_median=61.066 tail_median=10.452
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=33650737.606 p25=31969884.369 p75=33832616.766 min=27584435.786 max=34381167.458
post_rss median=9826304 min=9347072 max=10027008
peak_rss median=45744128 min=45744128 max=45744128 source=VmHWM_process
page_faults minor_median=5740 minor_min=5136 minor_max=11154
steady_work throughput_median=36024701.237 p25=34903482.491 p75=36399672.039
interleaved_phase_ms work_median=44.768 tail_median=5.140
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=89523260.661 p25=85750988.669 p75=94240903.073 min=75469031.778 max=109400923.959
post_rss median=4915200 min=2220032 max=5001216
peak_rss median=5079040 min=2228224 max=5132288 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=490
steady_work throughput_median=95360269.908 p25=92329251.188 p75=97996120.211
interleaved_phase_ms work_median=17.307 tail_median=3.561
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k2
throughput median=257940.954 p25=254500.370 p75=262550.539 min=143904.635 max=265909.878
post_rss median=3444736 min=3129344 max=3866624
peak_rss median=64667648 min=64094208 max=65196032 source=VmHWM_process
page_faults minor_median=15007 minor_min=14936 minor_max=15427
steady_work throughput_median=364342.028 p25=359509.650 p75=368708.706
phase_ms alloc_median=22.005 remote_median=0.475
```

