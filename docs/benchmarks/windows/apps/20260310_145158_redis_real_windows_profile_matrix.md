# Windows Real Redis Profile Matrix

Generated: 2026-03-10 14:51:58 +09:00

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
| balanced | jemalloc | set | 1,176,470.62 | 50 | 16 | 100000 | 3 |
| balanced | jemalloc | get | 1,408,450.62 | 50 | 16 | 100000 | 3 |
| balanced | jemalloc | lpush | 1,063,829.88 | 50 | 16 | 100000 | 3 |
| balanced | jemalloc | lpop | 1,136,363.62 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | set | 1,298,701.25 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | get | 1,470,588.12 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | lpush | 1,315,789.50 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | lpop | 1,333,333.25 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | set | 1,298,701.25 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | get | 1,515,151.50 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | lpush | 1,298,701.25 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | lpop | 1,351,351.38 | 50 | 16 | 100000 | 3 |

Per-run detail:
- `balanced / jemalloc / set`: [1,162,790.62, 1,176,470.62, 1,162,790.62]
- `balanced / jemalloc / get`: [1,408,450.62, 1,388,889.00, 1,388,889.00]
- `balanced / jemalloc / lpush`: [1,052,631.62, 1,052,631.62, 1,063,829.88]
- `balanced / jemalloc / lpop`: [1,136,363.62, 1,098,901.12, 1,136,363.62]
- `balanced / tcmalloc / set`: [1,282,051.25, 1,250,000.00, 1,298,701.25]
- `balanced / tcmalloc / get`: [1,408,450.62, 1,428,571.38, 1,470,588.12]
- `balanced / tcmalloc / lpush`: [1,298,701.25, 1,265,822.75, 1,315,789.50]
- `balanced / tcmalloc / lpop`: [1,333,333.25, 1,333,333.25, 1,282,051.25]
- `balanced / mimalloc / set`: [1,234,567.88, 1,298,701.25, 1,282,051.25]
- `balanced / mimalloc / get`: [1,470,588.12, 1,449,275.38, 1,515,151.50]
- `balanced / mimalloc / lpush`: [1,282,051.25, 1,298,701.25, 1,282,051.25]
- `balanced / mimalloc / lpop`: [1,315,789.50, 1,351,351.38, 1,315,789.50]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_145158_redis_real_windows_profile_matrix.log`
