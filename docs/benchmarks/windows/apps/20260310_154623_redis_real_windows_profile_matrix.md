# Windows Real Redis Profile Matrix

Generated: 2026-03-10 15:46:23 +09:00

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
| list_only | jemalloc | lpush | 1,006,711.38 | 60 | 16 | 150000 | 3 |
| list_only | jemalloc | lpop | 1,071,428.62 | 60 | 16 | 150000 | 3 |
| list_only | tcmalloc | lpush | 1,250,000.00 | 60 | 16 | 150000 | 3 |
| list_only | tcmalloc | lpop | 1,315,789.50 | 60 | 16 | 150000 | 3 |
| list_only | mimalloc | lpush | 1,271,186.38 | 60 | 16 | 150000 | 3 |
| list_only | mimalloc | lpop | 1,327,433.62 | 60 | 16 | 150000 | 3 |

Per-run detail:
- `list_only / jemalloc / lpush`: [1,006,711.38, 993,377.50, 993,377.50]
- `list_only / jemalloc / lpop`: [1,071,428.62, 1,063,829.75, 1,071,428.62]
- `list_only / tcmalloc / lpush`: [1,250,000.00, 1,171,875.00, 1,181,102.38]
- `list_only / tcmalloc / lpop`: [1,304,347.75, 1,315,789.50, 1,293,103.50]
- `list_only / mimalloc / lpush`: [1,181,102.38, 1,271,186.38, 1,250,000.00]
- `list_only / mimalloc / lpop`: [1,315,789.50, 1,293,103.50, 1,327,433.62]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_154623_redis_real_windows_profile_matrix.log`
