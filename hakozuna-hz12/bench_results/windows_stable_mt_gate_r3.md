# HZ12 Windows Stable-Duration MT Gate

Generated: 2026-07-10 22:20:14 +09:00

Target duration: 2 seconds per measured process; rounds: 3; affinity: 0xFF; priority: High.
Each allocator/profile uses a pilot-calibrated iteration count. Row order rotates every round.

## balanced

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-span-cache256 | 28.400M | 28.237M | 28.668M | 1.956s | 43.00 MiB | 6945482 |
| hz12-core | 26.468M | 26.045M | 26.724M | 1.914s | 43.55 MiB | 6331480 |
| hz12-coldspanowner | 59.380M | 59.128M | 60.930M | 2.008s | 49.09 MiB | 14902242 |
| tcmalloc | 523.964M | 521.101M | 524.360M | 1.962s | 46.98 MiB | 128526445 |

## wide_ws

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-span-cache256 | 16.668M | 16.588M | 16.733M | 1.986s | 85.78 MiB | 4137498 |
| hz12-core | 21.268M | 21.204M | 21.380M | 2.081s | 79.68 MiB | 5533117 |
| hz12-coldspanowner | 32.086M | 31.448M | 33.580M | 2.419s | 88.46 MiB | 9703463 |
| tcmalloc | 441.749M | 440.063M | 442.327M | 1.957s | 73.84 MiB | 108084740 |

## larger_sizes

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-span-cache256 | 48.858M | 47.834M | 49.149M | 2.006s | 59.16 MiB | 24499797 |
| hz12-core | 39.248M | 39.116M | 39.517M | 2.087s | 90.04 MiB | 20481312 |
| hz12-coldspanowner | 74.075M | 73.263M | 75.195M | 2.082s | 91.94 MiB | 38560412 |
| tcmalloc | 245.415M | 236.196M | 249.184M | 1.969s | 82.18 MiB | 120821590 |

## Raw Runs

```text
round=1 profile=balanced allocator=hz11-span-cache256 iters=6945482 elapsed=1.920 ops=28935752.714 peak_mib=43.00 raw=[BENCH_ARGS] threads=8 iters=6945482 ws=4096 min=16 max=2048 threads=8 iters=6945482 ws=4096 size=16..2048 time=1.920 ops/s=28935752.714 alloc_attempts=27787264 alloc_success=27787264 alloc_fail=0 frees=27787264
round=1 profile=balanced allocator=hz12-core iters=6331480 elapsed=1.914 ops=26468013.457 peak_mib=46.17 raw=[BENCH_ARGS] threads=8 iters=6331480 ws=4096 min=16 max=2048 threads=8 iters=6331480 ws=4096 size=16..2048 time=1.914 ops/s=26468013.457 alloc_attempts=25329664 alloc_success=25329664 alloc_fail=0 frees=25329664
round=1 profile=balanced allocator=hz12-coldspanowner iters=14902242 elapsed=1.908 ops=62480273.983 peak_mib=47.11 raw=[BENCH_ARGS] threads=8 iters=14902242 ws=4096 min=16 max=2048 threads=8 iters=14902242 ws=4096 size=16..2048 time=1.908 ops/s=62480273.983 alloc_attempts=59612944 alloc_success=59612944 alloc_fail=0 frees=59612944
round=1 profile=balanced allocator=tcmalloc iters=128526445 elapsed=1.962 ops=523963524.966 peak_mib=46.24 raw=threads=8 iters=128526445 ws=4096 size=16..2048 time=1.962 ops/s=523963524.966 peak_kb=47352 current_kb=0 scavenge_released=0
round=2 profile=balanced allocator=hz12-core iters=6331480 elapsed=1.877 ops=26980714.683 peak_mib=43.55 raw=[BENCH_ARGS] threads=8 iters=6331480 ws=4096 min=16 max=2048 threads=8 iters=6331480 ws=4096 size=16..2048 time=1.877 ops/s=26980714.683 alloc_attempts=25329664 alloc_success=25329664 alloc_fail=0 frees=25329664
round=2 profile=balanced allocator=hz12-coldspanowner iters=14902242 elapsed=2.008 ops=59380332.935 peak_mib=49.09 raw=[BENCH_ARGS] threads=8 iters=14902242 ws=4096 min=16 max=2048 threads=8 iters=14902242 ws=4096 size=16..2048 time=2.008 ops/s=59380332.935 alloc_attempts=59612944 alloc_success=59612944 alloc_fail=0 frees=59612944
round=2 profile=balanced allocator=tcmalloc iters=128526445 elapsed=1.959 ops=524756205.713 peak_mib=46.98 raw=threads=8 iters=128526445 ws=4096 size=16..2048 time=1.959 ops/s=524756205.713 peak_kb=48112 current_kb=0 scavenge_released=0
round=2 profile=balanced allocator=hz11-span-cache256 iters=6945482 elapsed=1.956 ops=28399705.371 peak_mib=42.24 raw=[BENCH_ARGS] threads=8 iters=6945482 ws=4096 min=16 max=2048 threads=8 iters=6945482 ws=4096 size=16..2048 time=1.956 ops/s=28399705.371 alloc_attempts=27787264 alloc_success=27787264 alloc_fail=0 frees=27787264
round=3 profile=balanced allocator=hz12-coldspanowner iters=14902242 elapsed=2.025 ops=58876040.775 peak_mib=49.67 raw=[BENCH_ARGS] threads=8 iters=14902242 ws=4096 min=16 max=2048 threads=8 iters=14902242 ws=4096 size=16..2048 time=2.025 ops/s=58876040.775 alloc_attempts=59612944 alloc_success=59612944 alloc_fail=0 frees=59612944
round=3 profile=balanced allocator=tcmalloc iters=128526445 elapsed=1.984 ops=518238733.903 peak_mib=47.66 raw=threads=8 iters=128526445 ws=4096 size=16..2048 time=1.984 ops/s=518238733.903 peak_kb=48808 current_kb=0 scavenge_released=0
round=3 profile=balanced allocator=hz11-span-cache256 iters=6945482 elapsed=1.979 ops=28075055.443 peak_mib=45.09 raw=[BENCH_ARGS] threads=8 iters=6945482 ws=4096 min=16 max=2048 threads=8 iters=6945482 ws=4096 size=16..2048 time=1.979 ops/s=28075055.443 alloc_attempts=27787264 alloc_success=27787264 alloc_fail=0 frees=27787264
round=3 profile=balanced allocator=hz12-core iters=6331480 elapsed=1.977 ops=25621862.764 peak_mib=42.68 raw=[BENCH_ARGS] threads=8 iters=6331480 ws=4096 min=16 max=2048 threads=8 iters=6331480 ws=4096 size=16..2048 time=1.977 ops/s=25621862.764 alloc_attempts=25329664 alloc_success=25329664 alloc_fail=0 frees=25329664
round=1 profile=wide_ws allocator=hz11-span-cache256 iters=4137498 elapsed=2.005 ops=16508928.517 peak_mib=85.21 raw=[BENCH_ARGS] threads=8 iters=4137498 ws=16384 min=16 max=1024 threads=8 iters=4137498 ws=16384 size=16..1024 time=2.005 ops/s=16508928.517 alloc_attempts=16584912 alloc_success=16584912 alloc_fail=0 frees=16584912
round=1 profile=wide_ws allocator=hz12-core iters=5533117 elapsed=2.094 ops=21139866.699 peak_mib=87.44 raw=[BENCH_ARGS] threads=8 iters=5533117 ws=16384 min=16 max=1024 threads=8 iters=5533117 ws=16384 size=16..1024 time=2.094 ops/s=21139866.699 alloc_attempts=22151168 alloc_success=22151168 alloc_fail=0 frees=22151168
round=1 profile=wide_ws allocator=hz12-coldspanowner iters=9703463 elapsed=2.213 ops=35074476.164 peak_mib=88.05 raw=[BENCH_ARGS] threads=8 iters=9703463 ws=16384 min=16 max=1024 threads=8 iters=9703463 ws=16384 size=16..1024 time=2.213 ops/s=35074476.164 alloc_attempts=38830392 alloc_success=38830392 alloc_fail=0 frees=38830392
round=1 profile=wide_ws allocator=tcmalloc iters=108084740 elapsed=1.972 ops=438377233.775 peak_mib=74.46 raw=threads=8 iters=108084740 ws=16384 size=16..1024 time=1.972 ops/s=438377233.775 peak_kb=76248 current_kb=0 scavenge_released=0
round=2 profile=wide_ws allocator=hz12-core iters=5533117 elapsed=2.060 ops=21491746.385 peak_mib=79.68 raw=[BENCH_ARGS] threads=8 iters=5533117 ws=16384 min=16 max=1024 threads=8 iters=5533117 ws=16384 size=16..1024 time=2.060 ops/s=21491746.385 alloc_attempts=22151168 alloc_success=22151168 alloc_fail=0 frees=22151168
round=2 profile=wide_ws allocator=hz12-coldspanowner iters=9703463 elapsed=2.520 ops=30810361.478 peak_mib=90.84 raw=[BENCH_ARGS] threads=8 iters=9703463 ws=16384 min=16 max=1024 threads=8 iters=9703463 ws=16384 size=16..1024 time=2.520 ops/s=30810361.478 alloc_attempts=38830392 alloc_success=38830392 alloc_fail=0 frees=38830392
round=2 profile=wide_ws allocator=tcmalloc iters=108084740 elapsed=1.957 ops=441749234.723 peak_mib=71.52 raw=threads=8 iters=108084740 ws=16384 size=16..1024 time=1.957 ops/s=441749234.723 peak_kb=73236 current_kb=0 scavenge_released=0
round=2 profile=wide_ws allocator=hz11-span-cache256 iters=4137498 elapsed=1.986 ops=16667914.159 peak_mib=85.78 raw=[BENCH_ARGS] threads=8 iters=4137498 ws=16384 min=16 max=1024 threads=8 iters=4137498 ws=16384 size=16..1024 time=1.986 ops/s=16667914.159 alloc_attempts=16584912 alloc_success=16584912 alloc_fail=0 frees=16584912
round=3 profile=wide_ws allocator=hz12-coldspanowner iters=9703463 elapsed=2.419 ops=32085909.951 peak_mib=88.46 raw=[BENCH_ARGS] threads=8 iters=9703463 ws=16384 min=16 max=1024 threads=8 iters=9703463 ws=16384 size=16..1024 time=2.419 ops/s=32085909.951 alloc_attempts=38830392 alloc_success=38830392 alloc_fail=0 frees=38830392
round=3 profile=wide_ws allocator=tcmalloc iters=108084740 elapsed=1.952 ops=442904558.981 peak_mib=73.84 raw=threads=8 iters=108084740 ws=16384 size=16..1024 time=1.952 ops/s=442904558.981 peak_kb=75612 current_kb=0 scavenge_released=0
round=3 profile=wide_ws allocator=hz11-span-cache256 iters=4137498 elapsed=1.971 ops=16797330.167 peak_mib=90.23 raw=[BENCH_ARGS] threads=8 iters=4137498 ws=16384 min=16 max=1024 threads=8 iters=4137498 ws=16384 size=16..1024 time=1.971 ops/s=16797330.167 alloc_attempts=16584912 alloc_success=16584912 alloc_fail=0 frees=16584912
round=3 profile=wide_ws allocator=hz12-core iters=5533117 elapsed=2.081 ops=21267698.905 peak_mib=79.45 raw=[BENCH_ARGS] threads=8 iters=5533117 ws=16384 min=16 max=1024 threads=8 iters=5533117 ws=16384 size=16..1024 time=2.081 ops/s=21267698.905 alloc_attempts=22151168 alloc_success=22151168 alloc_fail=0 frees=22151168
round=1 profile=larger_sizes allocator=hz11-span-cache256 iters=24499797 elapsed=2.094 ops=46810543.408 peak_mib=59.16 raw=[BENCH_ARGS] threads=4 iters=24499797 ws=4096 min=256 max=8192 threads=4 iters=24499797 ws=4096 size=256..8192 time=2.094 ops/s=46810543.408 alloc_attempts=49004544 alloc_success=49004544 alloc_fail=0 frees=49004544
round=1 profile=larger_sizes allocator=hz12-core iters=20481312 elapsed=2.087 ops=39247976.082 peak_mib=90.18 raw=[BENCH_ARGS] threads=4 iters=20481312 ws=4096 min=256 max=8192 threads=4 iters=20481312 ws=4096 size=256..8192 time=2.087 ops/s=39247976.082 alloc_attempts=40965248 alloc_success=40965248 alloc_fail=0 frees=40965248
round=1 profile=larger_sizes allocator=hz12-coldspanowner iters=38560412 elapsed=2.082 ops=74075164.357 peak_mib=91.94 raw=[BENCH_ARGS] threads=4 iters=38560412 ws=4096 min=256 max=8192 threads=4 iters=38560412 ws=4096 size=256..8192 time=2.082 ops/s=74075164.357 alloc_attempts=77122160 alloc_success=77122160 alloc_fail=0 frees=77122160
round=1 profile=larger_sizes allocator=tcmalloc iters=120821590 elapsed=2.129 ops=226977057.434 peak_mib=82.18 raw=threads=4 iters=120821590 ws=4096 size=256..8192 time=2.129 ops/s=226977057.434 peak_kb=84156 current_kb=0 scavenge_released=0
round=2 profile=larger_sizes allocator=hz12-core iters=20481312 elapsed=2.102 ops=38983891.339 peak_mib=90.04 raw=[BENCH_ARGS] threads=4 iters=20481312 ws=4096 min=256 max=8192 threads=4 iters=20481312 ws=4096 size=256..8192 time=2.102 ops/s=38983891.339 alloc_attempts=40965248 alloc_success=40965248 alloc_fail=0 frees=40965248
round=2 profile=larger_sizes allocator=hz12-coldspanowner iters=38560412 elapsed=2.129 ops=72451828.903 peak_mib=93.53 raw=[BENCH_ARGS] threads=4 iters=38560412 ws=4096 min=256 max=8192 threads=4 iters=38560412 ws=4096 size=256..8192 time=2.129 ops/s=72451828.903 alloc_attempts=77122160 alloc_success=77122160 alloc_fail=0 frees=77122160
round=2 profile=larger_sizes allocator=tcmalloc iters=120821590 elapsed=1.969 ops=245414551.305 peak_mib=83.01 raw=threads=4 iters=120821590 ws=4096 size=256..8192 time=1.969 ops/s=245414551.305 peak_kb=85004 current_kb=0 scavenge_released=0
round=2 profile=larger_sizes allocator=hz11-span-cache256 iters=24499797 elapsed=1.982 ops=49439387.011 peak_mib=58.85 raw=[BENCH_ARGS] threads=4 iters=24499797 ws=4096 min=256 max=8192 threads=4 iters=24499797 ws=4096 size=256..8192 time=1.982 ops/s=49439387.011 alloc_attempts=49004544 alloc_success=49004544 alloc_fail=0 frees=49004544
round=3 profile=larger_sizes allocator=hz12-coldspanowner iters=38560412 elapsed=2.021 ops=76315398.911 peak_mib=91.88 raw=[BENCH_ARGS] threads=4 iters=38560412 ws=4096 min=256 max=8192 threads=4 iters=38560412 ws=4096 size=256..8192 time=2.021 ops/s=76315398.911 alloc_attempts=77122160 alloc_success=77122160 alloc_fail=0 frees=77122160
round=3 profile=larger_sizes allocator=tcmalloc iters=120821590 elapsed=1.911 ops=252953568.808 peak_mib=79.96 raw=threads=4 iters=120821590 ws=4096 size=256..8192 time=1.911 ops/s=252953568.808 peak_kb=81876 current_kb=0 scavenge_released=0
round=3 profile=larger_sizes allocator=hz11-span-cache256 iters=24499797 elapsed=2.006 ops=48857847.613 peak_mib=59.83 raw=[BENCH_ARGS] threads=4 iters=24499797 ws=4096 min=256 max=8192 threads=4 iters=24499797 ws=4096 size=256..8192 time=2.006 ops/s=48857847.613 alloc_attempts=49004544 alloc_success=49004544 alloc_fail=0 frees=49004544
round=3 profile=larger_sizes allocator=hz12-core iters=20481312 elapsed=2.059 ops=39786661.580 peak_mib=89.27 raw=[BENCH_ARGS] threads=4 iters=20481312 ws=4096 min=256 max=8192 threads=4 iters=20481312 ws=4096 size=256..8192 time=2.059 ops/s=39786661.580 alloc_attempts=40965248 alloc_success=40965248 alloc_fail=0 frees=40965248
```
