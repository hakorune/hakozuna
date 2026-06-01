# HZ6 Owner-Aware MT Remote

Generated: 2026-06-01 13:50:47 +09:00

Benchmark:
benchmark: `bench_random_mixed_mt_remote_hz6_owner_compare`
- owner-aware remote frees use per-thread HZ6 allocators and owner-slot switching
- peak working set is sampled from Windows process counters
runs: `3` threads=`16` iters=`2000000` ws=`400` size=`16..2048` remote_pct=`90` ring_slots=`65536` timeout=`900s`

| allocator | median ops/s | median peak_kb | median alloc_failures | median remote_frees | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| strict | 19.471M | 7,936 | 21,906,683 | 0 | `19.471M / 7900 KB / alloc_fail=21,906,683 / remote_frees=0, 17.049M / 7916 KB / alloc_fail=21,906,683 / remote_frees=0, 16.699M / 7936 KB / alloc_fail=21,906,683 / remote_frees=0` |
| speed | 18.668M | 7,992 | 21,906,451 | 4,542,153 | `16.453M / 7864 KB / alloc_fail=21,906,451 / remote_frees=4,542,153, 16.633M / 7816 KB / alloc_fail=21,906,451 / remote_frees=4,542,153, 18.668M / 7992 KB / alloc_fail=21,906,451 / remote_frees=4,542,153` |
| rss | 17.388M | 8,120 | 21,906,451 | 4,542,153 | `17.306M / 7880 KB / alloc_fail=21,906,451 / remote_frees=4,542,153, 17.388M / 8012 KB / alloc_fail=21,906,451 / remote_frees=4,542,153, 16.742M / 8120 KB / alloc_fail=21,906,451 / remote_frees=4,542,153` |
| remote | 18.772M | 7,968 | 21,906,451 | 4,542,153 | `16.756M / 7928 KB / alloc_fail=21,906,451 / remote_frees=4,542,153, 16.392M / 7788 KB / alloc_fail=21,906,451 / remote_frees=4,542,153, 18.772M / 7968 KB / alloc_fail=21,906,451 / remote_frees=4,542,153` |

Artifacts: [out_win_mt_remote_hz6_owner](/C:/git/hakozuna-win/out_win_mt_remote_hz6_owner)
