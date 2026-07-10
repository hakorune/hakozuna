# HZ12 Windows Stable-Duration MT Gate

Generated: 2026-07-11 07:10:59 +09:00

Target duration: 1 seconds per measured process; rounds: 3; affinity: 0xFF; priority: High.
Each allocator/profile uses a pilot-calibrated iteration count. Row order rotates every round.

## balanced

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-coldspanowner | 62.132M | 61.930M | 64.692M | 0.951s | 47.21 MiB | 7384434 |
| hz12-owner-ledger-p0 | 65.606M | 64.228M | 66.199M | 0.973s | 46.19 MiB | 7977281 |

## wide_ws

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-coldspanowner | 36.105M | 35.399M | 39.090M | 0.950s | 87.09 MiB | 4287246 |
| hz12-owner-ledger-p0 | 33.608M | 32.720M | 33.824M | 0.972s | 90.57 MiB | 4081766 |

## larger_sizes

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-coldspanowner | 74.329M | 74.320M | 75.188M | 1.047s | 92.66 MiB | 19452731 |
| hz12-owner-ledger-p0 | 73.383M | 72.306M | 74.275M | 1.014s | 95.35 MiB | 18601191 |

## Raw Runs

```text
round=1 profile=balanced allocator=hz12-coldspanowner iters=7384434 elapsed=0.957 ops=61728914.063 peak_mib=47.21 raw=[BENCH_ARGS] threads=8 iters=7384434 ws=4096 min=16 max=2048 threads=8 iters=7384434 ws=4096 size=16..2048 time=0.957 ops/s=61728914.063 alloc_attempts=29551504 alloc_success=29551504 alloc_fail=0 frees=29551504
round=1 profile=balanced allocator=hz12-owner-ledger-p0 iters=7977281 elapsed=0.955 ops=66791407.388 peak_mib=46.19 raw=[BENCH_ARGS] threads=8 iters=7977281 ws=4096 min=16 max=2048 threads=8 iters=7977281 ws=4096 size=16..2048 time=0.955 ops/s=66791407.388 alloc_attempts=31916032 alloc_success=31916032 alloc_fail=0 frees=31916032
round=2 profile=balanced allocator=hz12-owner-ledger-p0 iters=7977281 elapsed=1.015 ops=62849294.147 peak_mib=45.98 raw=[BENCH_ARGS] threads=8 iters=7977281 ws=4096 min=16 max=2048 threads=8 iters=7977281 ws=4096 size=16..2048 time=1.015 ops/s=62849294.147 alloc_attempts=31916032 alloc_success=31916032 alloc_fail=0 frees=31916032
round=2 profile=balanced allocator=hz12-coldspanowner iters=7384434 elapsed=0.878 ops=67252838.827 peak_mib=45.45 raw=[BENCH_ARGS] threads=8 iters=7384434 ws=4096 min=16 max=2048 threads=8 iters=7384434 ws=4096 size=16..2048 time=0.878 ops/s=67252838.827 alloc_attempts=29551504 alloc_success=29551504 alloc_fail=0 frees=29551504
round=3 profile=balanced allocator=hz12-coldspanowner iters=7384434 elapsed=0.951 ops=62131601.200 peak_mib=47.55 raw=[BENCH_ARGS] threads=8 iters=7384434 ws=4096 min=16 max=2048 threads=8 iters=7384434 ws=4096 size=16..2048 time=0.951 ops/s=62131601.200 alloc_attempts=29551504 alloc_success=29551504 alloc_fail=0 frees=29551504
round=3 profile=balanced allocator=hz12-owner-ledger-p0 iters=7977281 elapsed=0.973 ops=65606139.966 peak_mib=47.95 raw=[BENCH_ARGS] threads=8 iters=7977281 ws=4096 min=16 max=2048 threads=8 iters=7977281 ws=4096 size=16..2048 time=0.973 ops/s=65606139.966 alloc_attempts=31916032 alloc_success=31916032 alloc_fail=0 frees=31916032
round=1 profile=wide_ws allocator=hz12-coldspanowner iters=4287246 elapsed=0.815 ops=42074181.299 peak_mib=87.09 raw=[BENCH_ARGS] threads=8 iters=4287246 ws=16384 min=16 max=1024 threads=8 iters=4287246 ws=16384 size=16..1024 time=0.815 ops/s=42074181.299 alloc_attempts=17170432 alloc_success=17170432 alloc_fail=0 frees=17170432
round=1 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=4081766 elapsed=1.026 ops=31832657.621 peak_mib=92.25 raw=[BENCH_ARGS] threads=8 iters=4081766 ws=16384 min=16 max=1024 threads=8 iters=4081766 ws=16384 size=16..1024 time=1.026 ops/s=31832657.621 alloc_attempts=16384000 alloc_success=16384000 alloc_fail=0 frees=16384000
round=2 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=4081766 elapsed=0.972 ops=33608017.643 peak_mib=90.57 raw=[BENCH_ARGS] threads=8 iters=4081766 ws=16384 min=16 max=1024 threads=8 iters=4081766 ws=16384 size=16..1024 time=0.972 ops/s=33608017.643 alloc_attempts=16384000 alloc_success=16384000 alloc_fail=0 frees=16384000
round=2 profile=wide_ws allocator=hz12-coldspanowner iters=4287246 elapsed=0.989 ops=34692252.387 peak_mib=88.20 raw=[BENCH_ARGS] threads=8 iters=4287246 ws=16384 min=16 max=1024 threads=8 iters=4287246 ws=16384 size=16..1024 time=0.989 ops/s=34692252.387 alloc_attempts=17170432 alloc_success=17170432 alloc_fail=0 frees=17170432
round=3 profile=wide_ws allocator=hz12-coldspanowner iters=4287246 elapsed=0.950 ops=36105161.302 peak_mib=81.79 raw=[BENCH_ARGS] threads=8 iters=4287246 ws=16384 min=16 max=1024 threads=8 iters=4287246 ws=16384 size=16..1024 time=0.950 ops/s=36105161.302 alloc_attempts=17170432 alloc_success=17170432 alloc_fail=0 frees=17170432
round=3 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=4081766 elapsed=0.959 ops=34039597.485 peak_mib=89.39 raw=[BENCH_ARGS] threads=8 iters=4081766 ws=16384 min=16 max=1024 threads=8 iters=4081766 ws=16384 size=16..1024 time=0.959 ops/s=34039597.485 alloc_attempts=16384000 alloc_success=16384000 alloc_fail=0 frees=16384000
round=1 profile=larger_sizes allocator=hz12-coldspanowner iters=19452731 elapsed=1.047 ops=74310469.868 peak_mib=92.66 raw=[BENCH_ARGS] threads=4 iters=19452731 ws=4096 min=256 max=8192 threads=4 iters=19452731 ws=4096 size=256..8192 time=1.047 ops/s=74310469.868 alloc_attempts=38912000 alloc_success=38912000 alloc_fail=0 frees=38912000
round=1 profile=larger_sizes allocator=hz12-owner-ledger-p0 iters=18601191 elapsed=0.990 ops=75167253.099 peak_mib=94.49 raw=[BENCH_ARGS] threads=4 iters=18601191 ws=4096 min=256 max=8192 threads=4 iters=18601191 ws=4096 size=256..8192 time=0.990 ops/s=75167253.099 alloc_attempts=37208064 alloc_success=37208064 alloc_fail=0 frees=37208064
round=2 profile=larger_sizes allocator=hz12-owner-ledger-p0 iters=18601191 elapsed=1.045 ops=71228438.622 peak_mib=95.35 raw=[BENCH_ARGS] threads=4 iters=18601191 ws=4096 min=256 max=8192 threads=4 iters=18601191 ws=4096 size=256..8192 time=1.045 ops/s=71228438.622 alloc_attempts=37208064 alloc_success=37208064 alloc_fail=0 frees=37208064
round=2 profile=larger_sizes allocator=hz12-coldspanowner iters=19452731 elapsed=1.047 ops=74329032.503 peak_mib=91.93 raw=[BENCH_ARGS] threads=4 iters=19452731 ws=4096 min=256 max=8192 threads=4 iters=19452731 ws=4096 size=256..8192 time=1.047 ops/s=74329032.503 alloc_attempts=38912000 alloc_success=38912000 alloc_fail=0 frees=38912000
round=3 profile=larger_sizes allocator=hz12-coldspanowner iters=19452731 elapsed=1.023 ops=76046463.534 peak_mib=95.07 raw=[BENCH_ARGS] threads=4 iters=19452731 ws=4096 min=256 max=8192 threads=4 iters=19452731 ws=4096 size=256..8192 time=1.023 ops/s=76046463.534 alloc_attempts=38912000 alloc_success=38912000 alloc_fail=0 frees=38912000
round=3 profile=larger_sizes allocator=hz12-owner-ledger-p0 iters=18601191 elapsed=1.014 ops=73383420.886 peak_mib=100.50 raw=[BENCH_ARGS] threads=4 iters=18601191 ws=4096 min=256 max=8192 threads=4 iters=18601191 ws=4096 size=256..8192 time=1.014 ops/s=73383420.886 alloc_attempts=37208064 alloc_success=37208064 alloc_fail=0 frees=37208064
```
