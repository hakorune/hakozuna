# Windows Real Redis Profile Matrix

Generated: 2026-03-10 14:55:39 +09:00

References:
- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)
- [win/run_win_redis_real_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_matrix.ps1)
- [win/run_win_redis_real_profile_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_profile_matrix.ps1)
- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)

Source root used: `C:\git\hakozuna-win\private\bench-assets\windows\redis\source`
Runs per profile/allocator: `3`
- statistic: median requests/sec
- raw logs: private only

| profile | allocator | test | median requests/sec | clients | pipeline | requests | runs |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| kv_only | jemalloc | set | 1,209,677.50 | 80 | 16 | 150000 | 3 |
| kv_only | jemalloc | get | 1,415,094.38 | 80 | 16 | 150000 | 3 |
| kv_only | tcmalloc | set | 1,293,103.50 | 80 | 16 | 150000 | 3 |
| kv_only | tcmalloc | get | 1,515,151.50 | 80 | 16 | 150000 | 3 |
| kv_only | mimalloc | set | 1,363,636.38 | 80 | 16 | 150000 | 3 |
| kv_only | mimalloc | get | 1,546,391.75 | 80 | 16 | 150000 | 3 |

Per-run detail:
- `kv_only / jemalloc / set`: [1,209,677.50, 1,181,102.38, 1,171,875.00]
- `kv_only / jemalloc / get`: [1,376,146.88, 1,363,636.38, 1,415,094.38]
- `kv_only / tcmalloc / set`: [1,282,051.25, 1,293,103.50, 1,293,103.50]
- `kv_only / tcmalloc / get`: [1,515,151.50, 1,500,000.00, 1,470,588.25]
- `kv_only / mimalloc / set`: [1,293,103.50, 1,363,636.38, 1,363,636.38]
- `kv_only / mimalloc / get`: [1,515,151.50, 1,515,151.50, 1,546,391.75]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_145539_redis_real_windows_profile_matrix.log`
