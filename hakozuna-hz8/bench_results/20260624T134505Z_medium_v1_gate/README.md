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
throughput median=17841049.275 p25=2875598.960 p75=24370762.153 min=2271801.046 max=24573696.662
post_rss median=5451776 min=4685824 max=5689344
peak_rss median=51523584 min=38010880 max=53751808 source=VmHWM_process
page_faults minor_median=458735 minor_min=5647 minor_max=1222610
steady_work throughput_median=18508177.931 p25=2894393.573 p75=25396793.147
interleaved_phase_ms work_median=379.725 tail_median=32.734
```

## medium_interleaved_remote50

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=50 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=2189315.476 p25=1866798.534 p75=3004118.885 min=1419680.717 max=3868930.566
post_rss median=10330112 min=9973760 max=10444800
peak_rss median=32952320 min=27959296 max=32952320 source=VmHWM_process
page_faults minor_median=1329477 minor_min=691055 minor_max=1877267
steady_work throughput_median=2196546.826 p25=1870734.660 p75=3016103.347
interleaved_phase_ms work_median=816.147 tail_median=45.483
```

## medium_local0

```text
summary runs=10 threads=16 iters=100000 size=4097..65536 remote_pct=0 interleaved=1 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=11239356.382 p25=11173723.146 p75=11250401.235 min=11113386.037 max=11271085.859
post_rss median=4755456 min=2072576 max=5025792
peak_rss median=5025792 min=2228224 max=5025792 source=VmHWM_process
page_faults minor_median=328 minor_min=328 minor_max=475
steady_work throughput_median=11300460.210 p25=11231692.613 p75=11320457.902
interleaved_phase_ms work_median=141.920 tail_median=11.709
```

## medium_phase_remote90

```text
summary runs=10 threads=2 iters=4000 size=4097..65536 remote_pct=90 interleaved=0 live_window=0 bench_attribution=0 class_map_id=p2-v0
medium_geometry_id=q64-run64k
throughput median=142566.623 p25=126887.388 p75=142969.024 min=85512.219 max=147243.365
post_rss median=3670016 min=3502080 max=3792896
peak_rss median=65142784 min=64356352 max=65142784 source=VmHWM_process
page_faults minor_median=15007 minor_min=14936 minor_max=15546
steady_work throughput_median=179016.460 p25=163821.178 p75=180289.007
phase_ms alloc_median=44.801 remote_median=0.538
```

