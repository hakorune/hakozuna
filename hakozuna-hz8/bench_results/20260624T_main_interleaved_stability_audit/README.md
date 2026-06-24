# Main Interleaved Stability Audit

```text
./h8_bench_release --runs 10 --threads 16 --iters 100000 --min-size 16 --max-size 32768 --remote-pct 90 --interleaved 1 --live-window 4096
```

```text
summary runs=10 threads=16 iters=100000 size=16..32768 remote_pct=90 interleaved=1 live_window=4096 bench_attribution=0 class_map_id=p2-v0
throughput median=9068944.725 p25=2834319.901 p75=24075692.774 min=1993860.331 max=24732634.425
post_rss median=5328896 min=4689920 max=5779456
peak_rss median=46968832 min=35590144 max=55271424 source=VmHWM_process
page_faults minor_median=713830 minor_min=5294 minor_max=1483675
steady_work throughput_median=9240709.444 p25=2850246.706 p75=25101683.389
interleaved_phase_ms work_median=394.178 tail_median=48.266
```
