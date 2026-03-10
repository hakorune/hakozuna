# Windows Real Redis Profile Matrix

Generated: 2026-03-10 15:48:07 +09:00

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
| highpipe | jemalloc | set | 1,639,344.25 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | get | 2,000,000.00 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | lpush | 1,360,544.25 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | lpop | 1,492,537.25 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | set | 1,941,747.62 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | get | 2,247,191.00 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | lpush | 1,724,138.00 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | lpop | 1,851,851.75 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | set | 2,040,816.38 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | get | 2,298,850.75 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | lpush | 1,785,714.25 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | lpop | 1,869,158.88 | 80 | 32 | 200000 | 3 |

Per-run detail:
- `highpipe / jemalloc / set`: [1,626,016.25, 1,600,000.00, 1,639,344.25]
- `highpipe / jemalloc / get`: [1,941,747.62, 2,000,000.00, 1,960,784.38]
- `highpipe / jemalloc / lpush`: [1,324,503.38, 1,360,544.25, 1,351,351.38]
- `highpipe / jemalloc / lpop`: [1,492,537.25, 1,481,481.38, 1,459,854.12]
- `highpipe / tcmalloc / set`: [1,941,747.62, 1,941,747.62, 1,851,851.75]
- `highpipe / tcmalloc / get`: [2,150,537.50, 2,247,191.00, 2,127,659.75]
- `highpipe / tcmalloc / lpush`: [1,709,401.75, 1,724,138.00, 1,724,138.00]
- `highpipe / tcmalloc / lpop`: [1,851,851.75, 1,785,714.25, 1,652,892.62]
- `highpipe / mimalloc / set`: [2,020,202.00, 2,040,816.38, 1,904,762.00]
- `highpipe / mimalloc / get`: [2,247,191.00, 2,247,191.00, 2,298,850.75]
- `highpipe / mimalloc / lpush`: [1,724,138.00, 1,769,911.50, 1,785,714.25]
- `highpipe / mimalloc / lpop`: [1,869,158.88, 1,851,851.75, 1,818,181.88]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_154807_redis_real_windows_profile_matrix.log`
