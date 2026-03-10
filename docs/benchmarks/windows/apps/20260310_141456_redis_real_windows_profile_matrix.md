# Windows Real Redis Profile Matrix

Generated: 2026-03-10 14:14:56 +09:00

References:
- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)
- [win/run_win_redis_real_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_matrix.ps1)
- [win/run_win_redis_real_profile_matrix.ps1](/C:/git/hakozuna-win/win/run_win_redis_real_profile_matrix.ps1)
- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)

Source root used: `C:\git\hakozuna-win\private\bench-assets\windows\redis\source`
Runs per profile/allocator: `1`
- statistic: median requests/sec
- raw logs: private only

| profile | allocator | test | median requests/sec | clients | pipeline | requests | runs |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| balanced | jemalloc | set | 1,162,790.62 | 50 | 16 | 100000 | 1 |
| balanced | jemalloc | get | 1,351,351.38 | 50 | 16 | 100000 | 1 |
| balanced | jemalloc | lpush | 1,020,408.19 | 50 | 16 | 100000 | 1 |
| balanced | jemalloc | lpop | 1,111,111.12 | 50 | 16 | 100000 | 1 |

Per-run detail:
- `balanced / jemalloc / set`: [1,162,790.62]
- `balanced / jemalloc / get`: [1,351,351.38]
- `balanced / jemalloc / lpush`: [1,020,408.19]
- `balanced / jemalloc / lpop`: [1,111,111.12]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_141456_redis_real_windows_profile_matrix.log`
