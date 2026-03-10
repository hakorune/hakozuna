# Windows Real Redis Profile Matrix

Generated: 2026-03-10 21:31:30 +09:00

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
| balanced | hz4 | set | 1,351,351.38 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | get | 1,538,461.62 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | lpush | 1,176,470.62 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | lpop | 1,250,000.00 | 50 | 16 | 100000 | 3 |

Per-run detail:
- `balanced / hz4 / set`: [1,333,333.25, 1,282,051.25, 1,351,351.38]
- `balanced / hz4 / get`: [1,470,588.12, 1,538,461.62, 1,470,588.12]
- `balanced / hz4 / lpush`: [1,176,470.62, 1,176,470.62, 1,176,470.62]
- `balanced / hz4 / lpop`: [1,176,470.62, 1,250,000.00, 1,219,512.12]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_213130_redis_real_windows_profile_matrix.log`
