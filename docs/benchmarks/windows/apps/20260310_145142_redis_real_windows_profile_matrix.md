# Windows Real Redis Profile Matrix

Generated: 2026-03-10 14:51:42 +09:00

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
| lowpipe | jemalloc | set | 125,313.29 | 40 | 1 | 50000 | 1 |
| lowpipe | jemalloc | get | 132,626.00 | 40 | 1 | 50000 | 1 |
| lowpipe | jemalloc | lpush | 127,877.23 | 40 | 1 | 50000 | 1 |
| lowpipe | jemalloc | lpop | 129,870.13 | 40 | 1 | 50000 | 1 |

Per-run detail:
- `lowpipe / jemalloc / set`: [125,313.29]
- `lowpipe / jemalloc / get`: [132,626.00]
- `lowpipe / jemalloc / lpush`: [127,877.23]
- `lowpipe / jemalloc / lpop`: [129,870.13]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_145142_redis_real_windows_profile_matrix.log`
