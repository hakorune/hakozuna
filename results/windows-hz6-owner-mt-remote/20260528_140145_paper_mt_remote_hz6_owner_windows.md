# HZ6 Owner-Aware MT Remote

Generated: 2026-05-28 14:01:45 +09:00

Benchmark:
benchmark: `bench_random_mixed_mt_remote_hz6_owner_compare`
- owner-aware remote frees use per-thread HZ6 allocators and owner-slot switching
- peak working set is sampled from Windows process counters
runs: `1` threads=`4` iters=`20000` ws=`64` size=`16..256` remote_pct=`75` ring_slots=`65536` timeout=`60s`

| allocator | median ops/s | median peak_kb | median alloc_failures | median remote_frees | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| strict | 20.403M | n/a | 25 | 0 | `20.403M / NA KB / alloc_fail=25 / remote_frees=0` |
| speed | 22.162M | n/a | 114 | 29,862 | `22.162M / NA KB / alloc_fail=114 / remote_frees=29,862` |
| rss | 19.879M | n/a | 10,760 | 25,800 | `19.879M / NA KB / alloc_fail=10,760 / remote_frees=25,800` |
| remote | 20.088M | n/a | 114 | 29,862 | `20.088M / NA KB / alloc_fail=114 / remote_frees=29,862` |

Artifacts: [out_win_mt_remote_hz6_owner](/C:/git/hakozuna-win/out_win_mt_remote_hz6_owner)
