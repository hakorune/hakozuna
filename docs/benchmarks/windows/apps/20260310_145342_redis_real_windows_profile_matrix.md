# Windows Real Redis Profile Matrix

Generated: 2026-03-10 14:53:42 +09:00

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
| lowpipe | jemalloc | set | 131,578.95 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | get | 133,333.33 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | lpush | 132,626.00 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | lpop | 132,978.73 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | set | 127,877.23 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | get | 133,689.83 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | lpush | 134,048.27 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | lpop | 133,689.83 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | set | 128,205.13 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | get | 133,333.33 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | lpush | 134,048.27 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | lpop | 132,626.00 | 40 | 1 | 50000 | 3 |

Per-run detail:
- `lowpipe / jemalloc / set`: [131,578.95, 127,551.02, 128,534.70]
- `lowpipe / jemalloc / get`: [133,333.33, 133,333.33, 132,626.00]
- `lowpipe / jemalloc / lpush`: [130,548.30, 132,626.00, 132,275.14]
- `lowpipe / jemalloc / lpop`: [131,578.95, 132,978.73, 132,978.73]
- `lowpipe / tcmalloc / set`: [125,313.29, 127,877.23, 124,069.48]
- `lowpipe / tcmalloc / get`: [132,978.73, 131,926.12, 133,689.83]
- `lowpipe / tcmalloc / lpush`: [133,333.33, 131,578.95, 134,048.27]
- `lowpipe / tcmalloc / lpop`: [130,548.30, 132,626.00, 133,689.83]
- `lowpipe / mimalloc / set`: [124,688.28, 124,688.28, 128,205.13]
- `lowpipe / mimalloc / get`: [132,626.00, 133,333.33, 132,978.73]
- `lowpipe / mimalloc / lpush`: [131,233.59, 132,978.73, 134,048.27]
- `lowpipe / mimalloc / lpop`: [132,626.00, 131,926.12, 131,926.12]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_145342_redis_real_windows_profile_matrix.log`
