# Windows Real Redis Profile Matrix

Generated: 2026-03-10 16:10:33 +09:00

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
| balanced | jemalloc | set | 1,190,476.25 | 50 | 16 | 100000 | 1 |
| balanced | jemalloc | get | 1,333,333.25 | 50 | 16 | 100000 | 1 |
| balanced | jemalloc | lpush | 1,030,927.81 | 50 | 16 | 100000 | 1 |
| balanced | jemalloc | lpop | 1,111,111.12 | 50 | 16 | 100000 | 1 |
| balanced | tcmalloc | set | 1,250,000.00 | 50 | 16 | 100000 | 1 |
| balanced | tcmalloc | get | 1,515,151.50 | 50 | 16 | 100000 | 1 |
| balanced | tcmalloc | lpush | 1,250,000.00 | 50 | 16 | 100000 | 1 |
| balanced | tcmalloc | lpop | 1,250,000.00 | 50 | 16 | 100000 | 1 |
| balanced | mimalloc | set | 1,265,822.75 | 50 | 16 | 100000 | 1 |
| balanced | mimalloc | get | 1,492,537.25 | 50 | 16 | 100000 | 1 |
| balanced | mimalloc | lpush | 1,282,051.25 | 50 | 16 | 100000 | 1 |
| balanced | mimalloc | lpop | 1,315,789.50 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | set | 1,408,450.62 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | get | 1,492,537.25 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | lpush | 1,265,822.75 | 50 | 16 | 100000 | 1 |
| balanced | hz3 | lpop | 1,315,789.50 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | set | 1,315,789.50 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | get | 1,408,450.62 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | lpush | 1,149,425.38 | 50 | 16 | 100000 | 1 |
| balanced | hz4 | lpop | 1,204,819.38 | 50 | 16 | 100000 | 1 |

Per-run detail:
- `balanced / jemalloc / set`: [1,190,476.25]
- `balanced / jemalloc / get`: [1,333,333.25]
- `balanced / jemalloc / lpush`: [1,030,927.81]
- `balanced / jemalloc / lpop`: [1,111,111.12]
- `balanced / tcmalloc / set`: [1,250,000.00]
- `balanced / tcmalloc / get`: [1,515,151.50]
- `balanced / tcmalloc / lpush`: [1,250,000.00]
- `balanced / tcmalloc / lpop`: [1,250,000.00]
- `balanced / mimalloc / set`: [1,265,822.75]
- `balanced / mimalloc / get`: [1,492,537.25]
- `balanced / mimalloc / lpush`: [1,282,051.25]
- `balanced / mimalloc / lpop`: [1,315,789.50]
- `balanced / hz3 / set`: [1,408,450.62]
- `balanced / hz3 / get`: [1,492,537.25]
- `balanced / hz3 / lpush`: [1,265,822.75]
- `balanced / hz3 / lpop`: [1,315,789.50]
- `balanced / hz4 / set`: [1,315,789.50]
- `balanced / hz4 / get`: [1,408,450.62]
- `balanced / hz4 / lpush`: [1,149,425.38]
- `balanced / hz4 / lpop`: [1,204,819.38]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_161033_redis_real_windows_profile_matrix.log`
