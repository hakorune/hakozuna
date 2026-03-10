# Windows Real Redis Profile Matrix

Generated: 2026-03-10 16:07:40 +09:00

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
| balanced | hz3 | set | 1,219,512.12 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | get | 1,492,537.25 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | lpush | 1,265,822.75 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | lpop | 1,298,701.25 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | set | 1,388,889.00 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | get | 1,492,537.25 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | lpush | 1,176,470.62 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | lpop | 1,219,512.12 | 50 | 16 | 100000 | 1 |

Per-run detail:
- `balanced / hz3 / set`: [1,219,512.12]
- `balanced / hz3 / get`: [1,492,537.25]
- `balanced / hz3 / lpush`: [1,265,822.75]
- `balanced / hz3 / lpop`: [1,298,701.25]
- `balanced / hz4 / set`: [1,388,889.00]
- `balanced / hz4 / get`: [1,492,537.25]
- `balanced / hz4 / lpush`: [1,176,470.62]
- `balanced / hz4 / lpop`: [1,219,512.12]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_160740_redis_real_windows_profile_matrix.log`
