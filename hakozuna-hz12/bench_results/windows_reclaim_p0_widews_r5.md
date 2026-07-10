# HZ12 Windows Stable-Duration MT Gate

Generated: 2026-07-11 07:11:46 +09:00

Target duration: 2 seconds per measured process; rounds: 5; affinity: 0xFF; priority: High.
Each allocator/profile uses a pilot-calibrated iteration count. Row order rotates every round.

## wide_ws

| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hz12-coldspanowner | 34.442M | 34.196M | 34.969M | 2.169s | 88.29 MiB | 9338377 |
| hz12-owner-ledger-p0 | 35.753M | 34.888M | 35.864M | 1.839s | 91.20 MiB | 8220507 |

## Raw Runs

```text
round=1 profile=wide_ws allocator=hz12-coldspanowner iters=9338377 elapsed=1.967 ops=37971980.428 peak_mib=89.32 raw=[BENCH_ARGS] threads=8 iters=9338377 ws=16384 min=16 max=1024 threads=8 iters=9338377 ws=16384 size=16..1024 time=1.967 ops/s=37971980.428 alloc_attempts=37355520 alloc_success=37355520 alloc_fail=0 frees=37355520
round=1 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=8220507 elapsed=1.963 ops=33504004.720 peak_mib=91.20 raw=[BENCH_ARGS] threads=8 iters=8220507 ws=16384 min=16 max=1024 threads=8 iters=8220507 ws=16384 size=16..1024 time=1.963 ops/s=33504004.720 alloc_attempts=32899072 alloc_success=32899072 alloc_fail=0 frees=32899072
round=2 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=8220507 elapsed=1.885 ops=34888321.022 peak_mib=91.06 raw=[BENCH_ARGS] threads=8 iters=8220507 ws=16384 min=16 max=1024 threads=8 iters=8220507 ws=16384 size=16..1024 time=1.885 ops/s=34888321.022 alloc_attempts=32899072 alloc_success=32899072 alloc_fail=0 frees=32899072
round=2 profile=wide_ws allocator=hz12-coldspanowner iters=9338377 elapsed=2.169 ops=34442484.332 peak_mib=88.01 raw=[BENCH_ARGS] threads=8 iters=9338377 ws=16384 min=16 max=1024 threads=8 iters=9338377 ws=16384 size=16..1024 time=2.169 ops/s=34442484.332 alloc_attempts=37355520 alloc_success=37355520 alloc_fail=0 frees=37355520
round=3 profile=wide_ws allocator=hz12-coldspanowner iters=9338377 elapsed=2.136 ops=34968737.982 peak_mib=90.60 raw=[BENCH_ARGS] threads=8 iters=9338377 ws=16384 min=16 max=1024 threads=8 iters=9338377 ws=16384 size=16..1024 time=2.136 ops/s=34968737.982 alloc_attempts=37355520 alloc_success=37355520 alloc_fail=0 frees=37355520
round=3 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=8220507 elapsed=1.685 ops=39027769.964 peak_mib=88.47 raw=[BENCH_ARGS] threads=8 iters=8220507 ws=16384 min=16 max=1024 threads=8 iters=8220507 ws=16384 size=16..1024 time=1.685 ops/s=39027769.964 alloc_attempts=32899072 alloc_success=32899072 alloc_fail=0 frees=32899072
round=4 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=8220507 elapsed=1.834 ops=35864382.751 peak_mib=92.23 raw=[BENCH_ARGS] threads=8 iters=8220507 ws=16384 min=16 max=1024 threads=8 iters=8220507 ws=16384 size=16..1024 time=1.834 ops/s=35864382.751 alloc_attempts=32899072 alloc_success=32899072 alloc_fail=0 frees=32899072
round=4 profile=wide_ws allocator=hz12-coldspanowner iters=9338377 elapsed=2.185 ops=34195851.137 peak_mib=88.29 raw=[BENCH_ARGS] threads=8 iters=9338377 ws=16384 min=16 max=1024 threads=8 iters=9338377 ws=16384 size=16..1024 time=2.185 ops/s=34195851.137 alloc_attempts=37355520 alloc_success=37355520 alloc_fail=0 frees=37355520
round=5 profile=wide_ws allocator=hz12-coldspanowner iters=9338377 elapsed=2.214 ops=33735602.471 peak_mib=87.69 raw=[BENCH_ARGS] threads=8 iters=9338377 ws=16384 min=16 max=1024 threads=8 iters=9338377 ws=16384 size=16..1024 time=2.214 ops/s=33735602.471 alloc_attempts=37355520 alloc_success=37355520 alloc_fail=0 frees=37355520
round=5 profile=wide_ws allocator=hz12-owner-ledger-p0 iters=8220507 elapsed=1.839 ops=35753314.086 peak_mib=92.30 raw=[BENCH_ARGS] threads=8 iters=8220507 ws=16384 min=16 max=1024 threads=8 iters=8220507 ws=16384 size=16..1024 time=1.839 ops/s=35753314.086 alloc_attempts=32899072 alloc_success=32899072 alloc_fail=0 frees=32899072
```
