# HZ6 Owner-Aware MT Remote

Generated: 2026-06-01 07:04:18 +09:00

Benchmark:
benchmark: `bench_random_mixed_mt_remote_hz6_owner_compare`
- owner-aware remote frees use per-thread HZ6 allocators and owner-slot switching
- peak working set is sampled from Windows process counters
runs: `1` threads=`16` iters=`2000000` ws=`400` size=`16..2048` remote_pct=`90` ring_slots=`65536` timeout=`900s`

| allocator | median ops/s | median peak_kb | median alloc_failures | median remote_frees | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| strict | 17.203M | 7,760 | 21,906,683 | 0 | `17.203M / 7760 KB / alloc_fail=21,906,683 / remote_frees=0` |
| speed | 16.465M | 7,768 | 21,906,451 | 4,542,153 | `16.465M / 7768 KB / alloc_fail=21,906,451 / remote_frees=4,542,153` |
| rss | 17.126M | 7,680 | 21,906,451 | 4,542,153 | `17.126M / 7680 KB / alloc_fail=21,906,451 / remote_frees=4,542,153` |
| remote | 16.475M | 7,784 | 21,906,451 | 4,542,153 | `16.475M / 7784 KB / alloc_fail=21,906,451 / remote_frees=4,542,153` |

Artifacts: [out_win_mt_remote_hz6_owner](/C:/git/hakozuna-win/out_win_mt_remote_hz6_owner)
