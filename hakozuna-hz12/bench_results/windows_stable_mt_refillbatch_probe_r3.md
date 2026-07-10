# HZ12 Windows Stable-Duration MT Gate

Generated: 2026-07-10 22:24:19 +09:00

Target duration: 2 seconds per measured process; rounds: 3; affinity: 0xFF; priority: High.
Each allocator/profile uses a pilot-calibrated iteration count. Row order rotates every round.

## balanced

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-span-cache256 | 27.855M | 27.122M | 27.970M | 1.867s | 41.61 MiB | 6499838 |
| hz12-core | 24.382M | 24.331M | 25.244M | 2.102s | 42.90 MiB | 6406562 |
| hz12-coldspanowner | 54.725M | 54.138M | 57.051M | 2.272s | 50.24 MiB | 15543881 |
| tcmalloc | 507.371M | 504.973M | 507.918M | 1.582s | 47.76 MiB | 100326061 |
| hz12-coldspanowner-batch32 | 166.157M | 160.210M | 169.801M | 1.559s | 46.36 MiB | 32373985 |

## wide_ws

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-span-cache256 | 17.093M | 16.937M | 17.297M | 1.823s | 88.51 MiB | 3895675 |
| hz12-core | 21.393M | 21.187M | 21.562M | 1.796s | 89.10 MiB | 4802890 |
| hz12-coldspanowner | 33.480M | 32.910M | 36.110M | 1.839s | 89.38 MiB | 7696542 |
| tcmalloc | 441.301M | 439.029M | 441.718M | 1.808s | 75.90 MiB | 99725755 |
| hz12-coldspanowner-batch32 | 70.661M | 67.283M | 70.761M | 1.663s | 90.69 MiB | 14692056 |

## larger_sizes

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz11-span-cache256 | 48.514M | 47.801M | 48.632M | 1.962s | 59.10 MiB | 23792058 |
| hz12-core | 40.281M | 40.215M | 40.477M | 1.990s | 90.10 MiB | 20038408 |
| hz12-coldspanowner | 74.503M | 73.569M | 75.693M | 1.960s | 93.27 MiB | 36510787 |
| tcmalloc | 249.126M | 247.205M | 249.399M | 1.915s | 80.93 MiB | 119260587 |
| hz12-coldspanowner-batch32 | 128.981M | 127.309M | 130.757M | 1.958s | 96.50 MiB | 63131314 |

## Raw Runs

```text
round=1 profile=balanced allocator=hz11-span-cache256 iters=6499838 elapsed=1.867 ops=27855078.201 peak_mib=40.60 raw=[BENCH_ARGS] threads=8 iters=6499838 ws=4096 min=16 max=2048 threads=8 iters=6499838 ws=4096 size=16..2048 time=1.867 ops/s=27855078.201 alloc_attempts=26013680 alloc_success=26013680 alloc_fail=0 frees=26013680
round=1 profile=balanced allocator=hz12-core iters=6406562 elapsed=2.111 ops=24281277.678 peak_mib=43.02 raw=[BENCH_ARGS] threads=8 iters=6406562 ws=4096 min=16 max=2048 threads=8 iters=6406562 ws=4096 size=16..2048 time=2.111 ops/s=24281277.678 alloc_attempts=25627920 alloc_success=25627920 alloc_fail=0 frees=25627920
round=1 profile=balanced allocator=hz12-coldspanowner iters=15543881 elapsed=2.272 ops=54724946.858 peak_mib=50.24 raw=[BENCH_ARGS] threads=8 iters=15543881 ws=4096 min=16 max=2048 threads=8 iters=15543881 ws=4096 size=16..2048 time=2.272 ops/s=54724946.858 alloc_attempts=62190152 alloc_success=62190152 alloc_fail=0 frees=62190152
round=1 profile=balanced allocator=tcmalloc iters=100326061 elapsed=1.597 ops=502575398.036 peak_mib=47.76 raw=threads=8 iters=100326061 ws=4096 size=16..2048 time=1.597 ops/s=502575398.036 peak_kb=48908 current_kb=0 scavenge_released=0
round=1 profile=balanced allocator=hz12-coldspanowner-batch32 iters=32373985 elapsed=1.559 ops=166157078.793 peak_mib=46.36 raw=[BENCH_ARGS] threads=8 iters=32373985 ws=4096 min=16 max=2048 threads=8 iters=32373985 ws=4096 size=16..2048 time=1.559 ops/s=166157078.793 alloc_attempts=129499136 alloc_success=129499136 alloc_fail=0 frees=129499136
round=2 profile=balanced allocator=hz12-core iters=6406562 elapsed=2.102 ops=24381523.673 peak_mib=41.11 raw=[BENCH_ARGS] threads=8 iters=6406562 ws=4096 min=16 max=2048 threads=8 iters=6406562 ws=4096 size=16..2048 time=2.102 ops/s=24381523.673 alloc_attempts=25627920 alloc_success=25627920 alloc_fail=0 frees=25627920
round=2 profile=balanced allocator=hz12-coldspanowner iters=15543881 elapsed=2.322 ops=53551220.439 peak_mib=50.63 raw=[BENCH_ARGS] threads=8 iters=15543881 ws=4096 min=16 max=2048 threads=8 iters=15543881 ws=4096 size=16..2048 time=2.322 ops/s=53551220.439 alloc_attempts=62190152 alloc_success=62190152 alloc_fail=0 frees=62190152
round=2 profile=balanced allocator=tcmalloc iters=100326061 elapsed=1.582 ops=507370921.582 peak_mib=47.76 raw=threads=8 iters=100326061 ws=4096 size=16..2048 time=1.582 ops/s=507370921.582 peak_kb=48908 current_kb=0 scavenge_released=0
round=2 profile=balanced allocator=hz12-coldspanowner-batch32 iters=32373985 elapsed=1.493 ops=173445597.451 peak_mib=45.83 raw=[BENCH_ARGS] threads=8 iters=32373985 ws=4096 min=16 max=2048 threads=8 iters=32373985 ws=4096 size=16..2048 time=1.493 ops/s=173445597.451 alloc_attempts=129499136 alloc_success=129499136 alloc_fail=0 frees=129499136
round=2 profile=balanced allocator=hz11-span-cache256 iters=6499838 elapsed=1.852 ops=28084143.271 peak_mib=43.30 raw=[BENCH_ARGS] threads=8 iters=6499838 ws=4096 min=16 max=2048 threads=8 iters=6499838 ws=4096 size=16..2048 time=1.852 ops/s=28084143.271 alloc_attempts=26013680 alloc_success=26013680 alloc_fail=0 frees=26013680
round=3 profile=balanced allocator=hz12-coldspanowner iters=15543881 elapsed=2.094 ops=59377541.430 peak_mib=47.52 raw=[BENCH_ARGS] threads=8 iters=15543881 ws=4096 min=16 max=2048 threads=8 iters=15543881 ws=4096 size=16..2048 time=2.094 ops/s=59377541.430 alloc_attempts=62190152 alloc_success=62190152 alloc_fail=0 frees=62190152
round=3 profile=balanced allocator=tcmalloc iters=100326061 elapsed=1.578 ops=508464158.629 peak_mib=48.91 raw=threads=8 iters=100326061 ws=4096 size=16..2048 time=1.578 ops/s=508464158.629 peak_kb=50092 current_kb=0 scavenge_released=0
round=3 profile=balanced allocator=hz12-coldspanowner-batch32 iters=32373985 elapsed=1.679 ops=154263564.645 peak_mib=48.16 raw=[BENCH_ARGS] threads=8 iters=32373985 ws=4096 min=16 max=2048 threads=8 iters=32373985 ws=4096 size=16..2048 time=1.679 ops/s=154263564.645 alloc_attempts=129499136 alloc_success=129499136 alloc_fail=0 frees=129499136
round=3 profile=balanced allocator=hz11-span-cache256 iters=6499838 elapsed=1.970 ops=26389337.589 peak_mib=41.61 raw=[BENCH_ARGS] threads=8 iters=6499838 ws=4096 min=16 max=2048 threads=8 iters=6499838 ws=4096 size=16..2048 time=1.970 ops/s=26389337.589 alloc_attempts=26013680 alloc_success=26013680 alloc_fail=0 frees=26013680
round=3 profile=balanced allocator=hz12-core iters=6406562 elapsed=1.963 ops=26106757.225 peak_mib=42.90 raw=[BENCH_ARGS] threads=8 iters=6406562 ws=4096 min=16 max=2048 threads=8 iters=6406562 ws=4096 size=16..2048 time=1.963 ops/s=26106757.225 alloc_attempts=25627920 alloc_success=25627920 alloc_fail=0 frees=25627920
round=1 profile=wide_ws allocator=hz11-span-cache256 iters=3895675 elapsed=1.781 ops=17500136.591 peak_mib=87.89 raw=[BENCH_ARGS] threads=8 iters=3895675 ws=16384 min=16 max=1024 threads=8 iters=3895675 ws=16384 size=16..1024 time=1.781 ops/s=17500136.591 alloc_attempts=15597568 alloc_success=15597568 alloc_fail=0 frees=15597568
round=1 profile=wide_ws allocator=hz12-core iters=4802890 elapsed=1.831 ops=20981557.933 peak_mib=89.10 raw=[BENCH_ARGS] threads=8 iters=4802890 ws=16384 min=16 max=1024 threads=8 iters=4802890 ws=16384 size=16..1024 time=1.831 ops/s=20981557.933 alloc_attempts=19267584 alloc_success=19267584 alloc_fail=0 frees=19267584
round=1 profile=wide_ws allocator=hz12-coldspanowner iters=7696542 elapsed=1.839 ops=33479500.035 peak_mib=89.38 raw=[BENCH_ARGS] threads=8 iters=7696542 ws=16384 min=16 max=1024 threads=8 iters=7696542 ws=16384 size=16..1024 time=1.839 ops/s=33479500.035 alloc_attempts=30801920 alloc_success=30801920 alloc_fail=0 frees=30801920
round=1 profile=wide_ws allocator=tcmalloc iters=99725755 elapsed=1.804 ops=442135443.715 peak_mib=75.15 raw=threads=8 iters=99725755 ws=16384 size=16..1024 time=1.804 ops/s=442135443.715 peak_kb=76952 current_kb=0 scavenge_released=0
round=1 profile=wide_ws allocator=hz12-coldspanowner-batch32 iters=14692056 elapsed=1.659 ops=70861740.130 peak_mib=90.69 raw=[BENCH_ARGS] threads=8 iters=14692056 ws=16384 min=16 max=1024 threads=8 iters=14692056 ws=16384 size=16..1024 time=1.659 ops/s=70861740.130 alloc_attempts=58816192 alloc_success=58816192 alloc_fail=0 frees=58816192
round=2 profile=wide_ws allocator=hz12-core iters=4802890 elapsed=1.768 ops=21731017.190 peak_mib=90.41 raw=[BENCH_ARGS] threads=8 iters=4802890 ws=16384 min=16 max=1024 threads=8 iters=4802890 ws=16384 size=16..1024 time=1.768 ops/s=21731017.190 alloc_attempts=19267584 alloc_success=19267584 alloc_fail=0 frees=19267584
round=2 profile=wide_ws allocator=hz12-coldspanowner iters=7696542 elapsed=1.589 ops=38741422.800 peak_mib=84.95 raw=[BENCH_ARGS] threads=8 iters=7696542 ws=16384 min=16 max=1024 threads=8 iters=7696542 ws=16384 size=16..1024 time=1.589 ops/s=38741422.800 alloc_attempts=30801920 alloc_success=30801920 alloc_fail=0 frees=30801920
round=2 profile=wide_ws allocator=tcmalloc iters=99725755 elapsed=1.808 ops=441301161.479 peak_mib=75.90 raw=threads=8 iters=99725755 ws=16384 size=16..1024 time=1.808 ops/s=441301161.479 peak_kb=77724 current_kb=0 scavenge_released=0
round=2 profile=wide_ws allocator=hz12-coldspanowner-batch32 iters=14692056 elapsed=1.663 ops=70661000.310 peak_mib=90.31 raw=[BENCH_ARGS] threads=8 iters=14692056 ws=16384 min=16 max=1024 threads=8 iters=14692056 ws=16384 size=16..1024 time=1.663 ops/s=70661000.310 alloc_attempts=58816192 alloc_success=58816192 alloc_fail=0 frees=58816192
round=2 profile=wide_ws allocator=hz11-span-cache256 iters=3895675 elapsed=1.823 ops=17092986.739 peak_mib=88.51 raw=[BENCH_ARGS] threads=8 iters=3895675 ws=16384 min=16 max=1024 threads=8 iters=3895675 ws=16384 size=16..1024 time=1.823 ops/s=17092986.739 alloc_attempts=15597568 alloc_success=15597568 alloc_fail=0 frees=15597568
round=3 profile=wide_ws allocator=hz12-coldspanowner iters=7696542 elapsed=1.904 ops=32339897.973 peak_mib=89.88 raw=[BENCH_ARGS] threads=8 iters=7696542 ws=16384 min=16 max=1024 threads=8 iters=7696542 ws=16384 size=16..1024 time=1.904 ops/s=32339897.973 alloc_attempts=30801920 alloc_success=30801920 alloc_fail=0 frees=30801920
round=3 profile=wide_ws allocator=tcmalloc iters=99725755 elapsed=1.827 ops=436757370.964 peak_mib=76.74 raw=threads=8 iters=99725755 ws=16384 size=16..1024 time=1.827 ops/s=436757370.964 peak_kb=78584 current_kb=0 scavenge_released=0
round=3 profile=wide_ws allocator=hz12-coldspanowner-batch32 iters=14692056 elapsed=1.839 ops=63905560.017 peak_mib=90.94 raw=[BENCH_ARGS] threads=8 iters=14692056 ws=16384 min=16 max=1024 threads=8 iters=14692056 ws=16384 size=16..1024 time=1.839 ops/s=63905560.017 alloc_attempts=58816192 alloc_success=58816192 alloc_fail=0 frees=58816192
round=3 profile=wide_ws allocator=hz11-span-cache256 iters=3895675 elapsed=1.857 ops=16780538.465 peak_mib=88.94 raw=[BENCH_ARGS] threads=8 iters=3895675 ws=16384 min=16 max=1024 threads=8 iters=3895675 ws=16384 size=16..1024 time=1.857 ops/s=16780538.465 alloc_attempts=15597568 alloc_success=15597568 alloc_fail=0 frees=15597568
round=3 profile=wide_ws allocator=hz12-core iters=4802890 elapsed=1.796 ops=21393366.791 peak_mib=76.71 raw=[BENCH_ARGS] threads=8 iters=4802890 ws=16384 min=16 max=1024 threads=8 iters=4802890 ws=16384 size=16..1024 time=1.796 ops/s=21393366.791 alloc_attempts=19267584 alloc_success=19267584 alloc_fail=0 frees=19267584
round=1 profile=larger_sizes allocator=hz11-span-cache256 iters=23792058 elapsed=2.021 ops=47088052.782 peak_mib=60.41 raw=[BENCH_ARGS] threads=4 iters=23792058 ws=4096 min=256 max=8192 threads=4 iters=23792058 ws=4096 size=256..8192 time=2.021 ops/s=47088052.782 alloc_attempts=47589096 alloc_success=47589096 alloc_fail=0 frees=47589096
round=1 profile=larger_sizes allocator=hz12-core iters=20038408 elapsed=1.996 ops=40149414.171 peak_mib=90.10 raw=[BENCH_ARGS] threads=4 iters=20038408 ws=4096 min=256 max=8192 threads=4 iters=20038408 ws=4096 size=256..8192 time=1.996 ops/s=40149414.171 alloc_attempts=40078368 alloc_success=40078368 alloc_fail=0 frees=40078368
round=1 profile=larger_sizes allocator=hz12-coldspanowner iters=36510787 elapsed=1.960 ops=74503014.287 peak_mib=93.27 raw=[BENCH_ARGS] threads=4 iters=36510787 ws=4096 min=256 max=8192 threads=4 iters=36510787 ws=4096 size=256..8192 time=1.960 ops/s=74503014.287 alloc_attempts=73023488 alloc_success=73023488 alloc_fail=0 frees=73023488
round=1 profile=larger_sizes allocator=tcmalloc iters=119260587 elapsed=1.915 ops=249125721.023 peak_mib=82.00 raw=threads=4 iters=119260587 ws=4096 size=256..8192 time=1.915 ops/s=249125721.023 peak_kb=83968 current_kb=0 scavenge_released=0
round=1 profile=larger_sizes allocator=hz12-coldspanowner-batch32 iters=63131314 elapsed=1.958 ops=128981084.939 peak_mib=95.74 raw=[BENCH_ARGS] threads=4 iters=63131314 ws=4096 min=256 max=8192 threads=4 iters=63131314 ws=4096 size=256..8192 time=1.958 ops/s=128981084.939 alloc_attempts=126270152 alloc_success=126270152 alloc_fail=0 frees=126270152
round=2 profile=larger_sizes allocator=hz12-core iters=20038408 elapsed=1.971 ops=40672575.687 peak_mib=89.29 raw=[BENCH_ARGS] threads=4 iters=20038408 ws=4096 min=256 max=8192 threads=4 iters=20038408 ws=4096 size=256..8192 time=1.971 ops/s=40672575.687 alloc_attempts=40078368 alloc_success=40078368 alloc_fail=0 frees=40078368
round=2 profile=larger_sizes allocator=hz12-coldspanowner iters=36510787 elapsed=1.900 ops=76883165.535 peak_mib=92.09 raw=[BENCH_ARGS] threads=4 iters=36510787 ws=4096 min=256 max=8192 threads=4 iters=36510787 ws=4096 size=256..8192 time=1.900 ops/s=76883165.535 alloc_attempts=73023488 alloc_success=73023488 alloc_fail=0 frees=73023488
round=2 profile=larger_sizes allocator=tcmalloc iters=119260587 elapsed=1.945 ops=245283681.903 peak_mib=80.61 raw=threads=4 iters=119260587 ws=4096 size=256..8192 time=1.945 ops/s=245283681.903 peak_kb=82544 current_kb=0 scavenge_released=0
round=2 profile=larger_sizes allocator=hz12-coldspanowner-batch32 iters=63131314 elapsed=2.010 ops=125637018.467 peak_mib=97.28 raw=[BENCH_ARGS] threads=4 iters=63131314 ws=4096 min=256 max=8192 threads=4 iters=63131314 ws=4096 size=256..8192 time=2.010 ops/s=125637018.467 alloc_attempts=126270152 alloc_success=126270152 alloc_fail=0 frees=126270152
round=2 profile=larger_sizes allocator=hz11-span-cache256 iters=23792058 elapsed=1.952 ops=48750053.978 peak_mib=58.63 raw=[BENCH_ARGS] threads=4 iters=23792058 ws=4096 min=256 max=8192 threads=4 iters=23792058 ws=4096 size=256..8192 time=1.952 ops/s=48750053.978 alloc_attempts=47589096 alloc_success=47589096 alloc_fail=0 frees=47589096
round=3 profile=larger_sizes allocator=hz12-coldspanowner iters=36510787 elapsed=2.011 ops=72635230.839 peak_mib=96.00 raw=[BENCH_ARGS] threads=4 iters=36510787 ws=4096 min=256 max=8192 threads=4 iters=36510787 ws=4096 size=256..8192 time=2.011 ops/s=72635230.839 alloc_attempts=73023488 alloc_success=73023488 alloc_fail=0 frees=73023488
round=3 profile=larger_sizes allocator=tcmalloc iters=119260587 elapsed=1.911 ops=249672365.774 peak_mib=80.93 raw=threads=4 iters=119260587 ws=4096 size=256..8192 time=1.911 ops/s=249672365.774 peak_kb=82876 current_kb=0 scavenge_released=0
round=3 profile=larger_sizes allocator=hz12-coldspanowner-batch32 iters=63131314 elapsed=1.905 ops=132533712.417 peak_mib=96.50 raw=[BENCH_ARGS] threads=4 iters=63131314 ws=4096 min=256 max=8192 threads=4 iters=63131314 ws=4096 size=256..8192 time=1.905 ops/s=132533712.417 alloc_attempts=126270152 alloc_success=126270152 alloc_fail=0 frees=126270152
round=3 profile=larger_sizes allocator=hz11-span-cache256 iters=23792058 elapsed=1.962 ops=48513832.663 peak_mib=59.10 raw=[BENCH_ARGS] threads=4 iters=23792058 ws=4096 min=256 max=8192 threads=4 iters=23792058 ws=4096 size=256..8192 time=1.962 ops/s=48513832.663 alloc_attempts=47589096 alloc_success=47589096 alloc_fail=0 frees=47589096
round=3 profile=larger_sizes allocator=hz12-core iters=20038408 elapsed=1.990 ops=40280757.475 peak_mib=91.64 raw=[BENCH_ARGS] threads=4 iters=20038408 ws=4096 min=256 max=8192 threads=4 iters=20038408 ws=4096 size=256..8192 time=1.990 ops/s=40280757.475 alloc_attempts=40078368 alloc_success=40078368 alloc_fail=0 frees=40078368
```
