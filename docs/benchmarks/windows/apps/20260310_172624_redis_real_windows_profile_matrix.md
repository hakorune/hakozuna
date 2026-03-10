# Windows Real Redis Profile Matrix

Generated: 2026-03-10 17:26:24 +09:00

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
| kv_only | jemalloc | set | 1,229,508.12 | 80 | 16 | 150000 | 3 |
| kv_only | jemalloc | get | 1,376,146.88 | 80 | 16 | 150000 | 3 |
| kv_only | tcmalloc | set | 1,339,285.62 | 80 | 16 | 150000 | 3 |
| kv_only | tcmalloc | get | 1,515,151.50 | 80 | 16 | 150000 | 3 |
| kv_only | mimalloc | set | 1,388,888.88 | 80 | 16 | 150000 | 3 |
| kv_only | mimalloc | get | 1,546,391.75 | 80 | 16 | 150000 | 3 |
| kv_only | hz3 | set | 1,401,869.12 | 80 | 16 | 150000 | 3 |
| kv_only | hz3 | get | 1,562,500.00 | 80 | 16 | 150000 | 3 |
| kv_only | hz4 | set | 1,339,285.62 | 80 | 16 | 150000 | 3 |
| kv_only | hz4 | get | 1,530,612.25 | 80 | 16 | 150000 | 3 |
| list_only | jemalloc | lpush | 1,041,666.69 | 60 | 16 | 150000 | 3 |
| list_only | jemalloc | lpop | 1,127,819.50 | 60 | 16 | 150000 | 3 |
| list_only | tcmalloc | lpush | 1,271,186.38 | 60 | 16 | 150000 | 3 |
| list_only | tcmalloc | lpop | 1,327,433.62 | 60 | 16 | 150000 | 3 |
| list_only | mimalloc | lpush | 1,293,103.50 | 60 | 16 | 150000 | 3 |
| list_only | mimalloc | lpop | 1,339,285.62 | 60 | 16 | 150000 | 3 |
| list_only | hz3 | lpush | 1,304,347.75 | 60 | 16 | 150000 | 3 |
| list_only | hz3 | lpop | 1,376,146.88 | 60 | 16 | 150000 | 3 |
| list_only | hz4 | lpush | 1,200,000.00 | 60 | 16 | 150000 | 3 |
| list_only | hz4 | lpop | 1,250,000.00 | 60 | 16 | 150000 | 3 |

Per-run detail:
- `kv_only / jemalloc / set`: [1,162,790.75, 1,229,508.12, 1,200,000.00]
- `kv_only / jemalloc / get`: [1,315,789.50, 1,376,146.88, 1,351,351.38]
- `kv_only / tcmalloc / set`: [1,315,789.50, 1,339,285.62, 1,229,508.12]
- `kv_only / tcmalloc / get`: [1,515,151.50, 1,515,151.50, 1,442,307.62]
- `kv_only / mimalloc / set`: [1,376,146.88, 1,327,433.62, 1,388,888.88]
- `kv_only / mimalloc / get`: [1,456,310.62, 1,546,391.75, 1,546,391.75]
- `kv_only / hz3 / set`: [1,401,869.12, 1,351,351.38, 1,363,636.38]
- `kv_only / hz3 / get`: [1,485,148.50, 1,500,000.00, 1,562,500.00]
- `kv_only / hz4 / set`: [1,127,819.50, 1,327,433.62, 1,339,285.62]
- `kv_only / hz4 / get`: [1,339,285.62, 1,530,612.25, 1,456,310.62]
- `list_only / jemalloc / lpush`: [1,041,666.69, 1,020,408.19, 1,013,513.50]
- `list_only / jemalloc / lpop`: [1,119,403.00, 1,127,819.50, 1,111,111.12]
- `list_only / tcmalloc / lpush`: [1,271,186.38, 1,250,000.00, 1,271,186.38]
- `list_only / tcmalloc / lpop`: [1,304,347.75, 1,282,051.25, 1,327,433.62]
- `list_only / mimalloc / lpush`: [1,219,512.12, 1,293,103.50, 1,260,504.12]
- `list_only / mimalloc / lpop`: [1,339,285.62, 1,293,103.50, 1,282,051.25]
- `list_only / hz3 / lpush`: [1,260,504.12, 1,304,347.75, 1,271,186.38]
- `list_only / hz3 / lpop`: [1,327,433.62, 1,327,433.62, 1,376,146.88]
- `list_only / hz4 / lpush`: [1,171,875.00, 1,200,000.00, 1,162,790.75]
- `list_only / hz4 / lpop`: [1,250,000.00, 1,229,508.12, 1,209,677.50]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_172624_redis_real_windows_profile_matrix.log`
