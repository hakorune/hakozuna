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
throughput median=22982699.313 p25=21461305.380 p75=24289096.477 min=19355220.741 max=24650142.834
post_rss median=5341184 min=4501504 max=5443584
peak_rss median=42991616 min=42991616 max=42991616 source=VmHWM_process
page_faults minor_median=7407 minor_min=5479 minor_max=10474
steady_work throughput_median=24316290.501 p25=22834836.158 p75=25664921.987
interleaved_phase_ms work_median=68.712 tail_median=11.662
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=13970956.739 p25=4876911.509 p75=14002070.836 min=2867937.466 max=14204137.767
post_rss median=10256384 min=9932800 max=10608640
peak_rss median=49860608 min=35520512 max=49860608 source=VmHWM_process
page_faults minor_median=8616 minor_min=6390 minor_max=763879
steady_work throughput_median=14497766.383 p25=4933342.876 p75=14512317.910
interleaved_phase_ms work_median=113.916 tail_median=15.118
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=11283616.609 p25=11080883.615 p75=11324629.770 min=11040822.246 max=11389987.683
post_rss median=4706304 min=2097152 max=4923392
peak_rss median=5058560 min=2097152 max=5058560 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=476
steady_work throughput_median=11376712.877 p25=11137628.722 p75=11381375.790
interleaved_phase_ms work_median=142.762 tail_median=10.034
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=140982.817 p25=133865.207 p75=142432.879 min=85198.508 max=144395.817
post_rss median=3584000 min=3489792 max=3760128
peak_rss median=64954368 min=64225280 max=64954368 source=VmHWM_process
page_faults minor_median=15007 minor_min=14936 minor_max=15546
steady_work throughput_median=179007.752 p25=167878.462 p75=180090.082
phase_ms alloc_median=45.008 remote_median=0.555
```

