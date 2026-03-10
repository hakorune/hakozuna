# Windows Real Redis Profile Matrix

Generated: 2026-03-10 21:34:53 +09:00

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
| balanced | hz4 | set | 1,408,450.62 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | get | 1,515,151.50 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | lpush | 1,176,470.62 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | lpop | 1,250,000.00 | 50 | 16 | 100000 | 3 |

Per-run detail:
- `balanced / hz4 / set`: [1,369,863.00, 1,408,450.62, 1,315,789.50]
- `balanced / hz4 / get`: [1,428,571.38, 1,515,151.50, 1,515,151.50]
- `balanced / hz4 / lpush`: [1,111,111.12, 1,162,790.62, 1,176,470.62]
- `balanced / hz4 / lpop`: [1,190,476.25, 1,190,476.25, 1,250,000.00]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_213453_redis_real_windows_profile_matrix.log`
