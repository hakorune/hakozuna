# HZ6 Owner-Aware MT Remote

Generated: 2026-05-28 14:00:48 +09:00

Benchmark:
benchmark: `bench_random_mixed_mt_remote_hz6_owner_compare`
- owner-aware remote frees use per-thread HZ6 allocators and owner-slot switching
- peak working set is sampled from Windows process counters
runs: `1` threads=`4` iters=`20000` ws=`64` size=`16..256` remote_pct=`75` ring_slots=`65536` timeout=`60s`

| allocator | median ops/s | median peak_kb | median alloc_failures | median remote_frees | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| strict | 0.097M | n/a | 0 | 0 | `0.097M / NA KB / alloc_fail=0 / remote_frees=0` |
| speed | 26.418M | n/a | 114 | 29,862 | `26.418M / NA KB / alloc_fail=114 / remote_frees=29,862` |
| rss | 0.195M | n/a | 0 | 17 | `0.195M / NA KB / alloc_fail=0 / remote_frees=17` |
| remote | 17.767M | n/a | 114 | 29,862 | `17.767M / NA KB / alloc_fail=114 / remote_frees=29,862` |

Artifacts: [out_win_mt_remote_hz6_owner](/C:/git/hakozuna-win/out_win_mt_remote_hz6_owner)
