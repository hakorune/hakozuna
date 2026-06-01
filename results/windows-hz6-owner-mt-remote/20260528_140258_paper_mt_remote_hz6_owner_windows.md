# HZ6 Owner-Aware MT Remote

Generated: 2026-05-28 14:02:58 +09:00

Benchmark:
benchmark: `bench_random_mixed_mt_remote_hz6_owner_compare`
- owner-aware remote frees use per-thread HZ6 allocators and owner-slot switching
- peak working set is sampled from Windows process counters
runs: `1` threads=`4` iters=`10000` ws=`64` size=`16..256` remote_pct=`75` ring_slots=`65536` timeout=`60s`

| allocator | median ops/s | median peak_kb | median alloc_failures | median remote_frees | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| strict | 21.115M | n/a | 23 | 0 | `21.115M / NA KB / alloc_fail=23 / remote_frees=0` |
| speed | 14.902M | n/a | 72 | 15,005 | `14.902M / NA KB / alloc_fail=72 / remote_frees=15,005` |
| rss | 17.671M | n/a | 5,326 | 12,872 | `17.671M / NA KB / alloc_fail=5,326 / remote_frees=12,872` |
| remote | 24.601M | n/a | 72 | 15,005 | `24.601M / NA KB / alloc_fail=72 / remote_frees=15,005` |

Artifacts: [out_win_mt_remote_hz6_owner](/C:/git/hakozuna-win/out_win_mt_remote_hz6_owner)
