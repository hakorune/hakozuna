# Windows Real Redis Profile Matrix

Generated: 2026-03-21 04:48:28 +09:00

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
| balanced | jemalloc | set | 1,149,425.38 | 50 | 16 | 100000 | 3 |
| balanced | jemalloc | get | 1,369,863.00 | 50 | 16 | 100000 | 3 |
| balanced | jemalloc | lpush | 1,052,631.62 | 50 | 16 | 100000 | 3 |
| balanced | jemalloc | lpop | 1,123,595.50 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | set | 1,388,889.00 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | get | 1,492,537.25 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | lpush | 1,282,051.25 | 50 | 16 | 100000 | 3 |
| balanced | tcmalloc | lpop | 1,333,333.25 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | set | 1,388,889.00 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | get | 1,538,461.62 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | lpush | 1,298,701.25 | 50 | 16 | 100000 | 3 |
| balanced | mimalloc | lpop | 1,351,351.38 | 50 | 16 | 100000 | 3 |
| balanced | hz3 | set | 1,369,863.00 | 50 | 16 | 100000 | 3 |
| balanced | hz3 | get | 1,492,537.25 | 50 | 16 | 100000 | 3 |
| balanced | hz3 | lpush | 1,265,822.75 | 50 | 16 | 100000 | 3 |
| balanced | hz3 | lpop | 1,351,351.38 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | set | 1,333,333.25 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | get | 1,470,588.12 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | lpush | 1,149,425.38 | 50 | 16 | 100000 | 3 |
| balanced | hz4 | lpop | 1,190,476.25 | 50 | 16 | 100000 | 3 |
| lowpipe | jemalloc | set | 129,870.13 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | get | 131,233.59 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | lpush | 126,582.27 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | lpop | 123,762.38 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | set | 128,865.98 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | get | 128,534.70 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | lpush | 119,617.22 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | lpop | 114,416.48 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | set | 126,903.55 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | get | 129,198.97 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | lpush | 125,944.58 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | lpop | 122,249.38 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | set | 128,534.70 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | get | 130,548.30 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | lpush | 119,904.08 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | lpop | 120,481.93 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | set | 125,000.00 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | get | 130,208.34 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | lpush | 127,877.23 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | lpop | 124,378.11 | 40 | 1 | 50000 | 3 |
| highpipe | jemalloc | set | 1,694,915.25 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | get | 2,000,000.00 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | lpush | 1,351,351.38 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | lpop | 1,503,759.38 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | set | 1,980,198.00 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | get | 2,298,850.75 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | lpush | 1,769,911.50 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | lpop | 1,869,158.88 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | set | 2,020,202.00 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | get | 2,272,727.25 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | lpush | 1,818,181.88 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | lpop | 1,886,792.50 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | set | 2,040,816.38 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | get | 2,352,941.25 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | lpush | 1,801,801.75 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | lpop | 1,923,076.88 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | set | 1,869,158.88 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | get | 2,247,191.00 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | lpush | 1,562,499.88 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | lpop | 1,639,344.25 | 80 | 32 | 200000 | 3 |
| kv_only | jemalloc | set | 1,219,512.12 | 80 | 16 | 150000 | 3 |
| kv_only | jemalloc | get | 1,363,636.38 | 80 | 16 | 150000 | 3 |
| kv_only | tcmalloc | set | 1,327,433.62 | 80 | 16 | 150000 | 3 |
| kv_only | tcmalloc | get | 1,500,000.00 | 80 | 16 | 150000 | 3 |
| kv_only | mimalloc | set | 1,376,146.88 | 80 | 16 | 150000 | 3 |
| kv_only | mimalloc | get | 1,530,612.25 | 80 | 16 | 150000 | 3 |
| kv_only | hz3 | set | 1,388,888.88 | 80 | 16 | 150000 | 3 |
| kv_only | hz3 | get | 1,546,391.75 | 80 | 16 | 150000 | 3 |
| kv_only | hz4 | set | 1,339,285.62 | 80 | 16 | 150000 | 3 |
| kv_only | hz4 | get | 1,485,148.50 | 80 | 16 | 150000 | 3 |
| list_only | jemalloc | lpush | 1,013,513.50 | 60 | 16 | 150000 | 3 |
| list_only | jemalloc | lpop | 1,119,403.00 | 60 | 16 | 150000 | 3 |
| list_only | tcmalloc | lpush | 1,260,504.12 | 60 | 16 | 150000 | 3 |
| list_only | tcmalloc | lpop | 1,315,789.50 | 60 | 16 | 150000 | 3 |
| list_only | mimalloc | lpush | 1,282,051.25 | 60 | 16 | 150000 | 3 |
| list_only | mimalloc | lpop | 1,351,351.38 | 60 | 16 | 150000 | 3 |
| list_only | hz3 | lpush | 1,271,186.38 | 60 | 16 | 150000 | 3 |
| list_only | hz3 | lpop | 1,376,146.88 | 60 | 16 | 150000 | 3 |
| list_only | hz4 | lpush | 1,094,890.50 | 60 | 16 | 150000 | 3 |
| list_only | hz4 | lpop | 1,200,000.00 | 60 | 16 | 150000 | 3 |

Per-run detail:
- `balanced / jemalloc / set`: [1,149,425.38, 1,136,363.62, 1,136,363.62]
- `balanced / jemalloc / get`: [1,315,789.50, 1,369,863.00, 1,351,351.38]
- `balanced / jemalloc / lpush`: [1,010,101.00, 1,041,666.69, 1,052,631.62]
- `balanced / jemalloc / lpop`: [1,075,268.75, 1,123,595.50, 1,111,111.12]
- `balanced / tcmalloc / set`: [1,388,889.00, 1,250,000.00, 1,265,822.75]
- `balanced / tcmalloc / get`: [1,470,588.12, 1,492,537.25, 1,492,537.25]
- `balanced / tcmalloc / lpush`: [1,265,822.75, 1,282,051.25, 1,282,051.25]
- `balanced / tcmalloc / lpop`: [1,282,051.25, 1,315,789.50, 1,333,333.25]
- `balanced / mimalloc / set`: [1,388,889.00, 1,333,333.25, 1,388,889.00]
- `balanced / mimalloc / get`: [1,492,537.25, 1,538,461.62, 1,515,151.50]
- `balanced / mimalloc / lpush`: [1,282,051.25, 1,265,822.75, 1,298,701.25]
- `balanced / mimalloc / lpop`: [1,315,789.50, 1,351,351.38, 1,298,701.25]
- `balanced / hz3 / set`: [1,369,863.00, 1,315,789.50, 1,250,000.00]
- `balanced / hz3 / get`: [1,492,537.25, 1,388,889.00, 1,492,537.25]
- `balanced / hz3 / lpush`: [1,265,822.75, 1,250,000.00, 1,250,000.00]
- `balanced / hz3 / lpop`: [1,333,333.25, 1,315,789.50, 1,351,351.38]
- `balanced / hz4 / set`: [1,333,333.25, 1,190,476.25, 1,250,000.00]
- `balanced / hz4 / get`: [1,449,275.38, 1,470,588.12, 1,470,588.12]
- `balanced / hz4 / lpush`: [1,111,111.12, 1,149,425.38, 1,136,363.62]
- `balanced / hz4 / lpop`: [1,190,476.25, 1,190,476.25, 1,176,470.62]
- `lowpipe / jemalloc / set`: [129,533.68, 129,198.97, 129,870.13]
- `lowpipe / jemalloc / get`: [129,870.13, 131,233.59, 130,208.34]
- `lowpipe / jemalloc / lpush`: [123,152.71, 125,944.58, 126,582.27]
- `lowpipe / jemalloc / lpop`: [121,951.22, 123,762.38, 107,066.38]
- `lowpipe / tcmalloc / set`: [128,865.98, 125,628.14, 121,951.22]
- `lowpipe / tcmalloc / get`: [120,481.93, 124,688.28, 128,534.70]
- `lowpipe / tcmalloc / lpush`: [114,942.53, 117,924.53, 119,617.22]
- `lowpipe / tcmalloc / lpop`: [110,132.16, 114,416.48, 100,000.00]
- `lowpipe / mimalloc / set`: [124,069.48, 126,903.55, 125,628.14]
- `lowpipe / mimalloc / get`: [129,198.97, 126,262.62, 127,551.02]
- `lowpipe / mimalloc / lpush`: [123,456.79, 125,944.58, 124,069.48]
- `lowpipe / mimalloc / lpop`: [122,249.38, 119,904.08, 120,481.93]
- `lowpipe / hz3 / set`: [128,205.13, 127,551.02, 128,534.70]
- `lowpipe / hz3 / get`: [130,548.30, 128,205.13, 127,226.46]
- `lowpipe / hz3 / lpush`: [116,009.28, 102,880.66, 119,904.08]
- `lowpipe / hz3 / lpop`: [112,359.55, 120,481.93, 113,636.37]
- `lowpipe / hz4 / set`: [122,850.12, 125,000.00, 125,000.00]
- `lowpipe / hz4 / get`: [128,865.98, 130,208.34, 128,865.98]
- `lowpipe / hz4 / lpush`: [122,249.38, 125,944.58, 127,877.23]
- `lowpipe / hz4 / lpop`: [124,378.11, 122,249.38, 124,378.11]
- `highpipe / jemalloc / set`: [1,694,915.25, 1,652,892.62, 1,600,000.00]
- `highpipe / jemalloc / get`: [1,960,784.38, 2,000,000.00, 1,980,198.00]
- `highpipe / jemalloc / lpush`: [1,351,351.38, 1,351,351.38, 1,342,281.88]
- `highpipe / jemalloc / lpop`: [1,428,571.38, 1,470,588.12, 1,503,759.38]
- `highpipe / tcmalloc / set`: [1,941,747.62, 1,980,198.00, 1,886,792.50]
- `highpipe / tcmalloc / get`: [2,197,802.25, 2,298,850.75, 2,247,191.00]
- `highpipe / tcmalloc / lpush`: [1,724,138.00, 1,754,386.00, 1,769,911.50]
- `highpipe / tcmalloc / lpop`: [1,834,862.38, 1,834,862.38, 1,869,158.88]
- `highpipe / mimalloc / set`: [1,923,076.88, 2,020,202.00, 1,923,076.88]
- `highpipe / mimalloc / get`: [2,061,855.62, 2,247,191.00, 2,272,727.25]
- `highpipe / mimalloc / lpush`: [1,739,130.38, 1,785,714.25, 1,818,181.88]
- `highpipe / mimalloc / lpop`: [1,886,792.50, 1,886,792.50, 1,869,158.88]
- `highpipe / hz3 / set`: [1,960,784.38, 2,040,816.38, 1,923,076.88]
- `highpipe / hz3 / get`: [2,272,727.25, 2,325,581.25, 2,352,941.25]
- `highpipe / hz3 / lpush`: [1,769,911.50, 1,801,801.75, 1,769,911.50]
- `highpipe / hz3 / lpop`: [1,923,076.88, 1,886,792.50, 1,904,762.00]
- `highpipe / hz4 / set`: [1,818,181.88, 1,869,158.88, 1,834,862.38]
- `highpipe / hz4 / get`: [2,197,802.25, 2,197,802.25, 2,247,191.00]
- `highpipe / hz4 / lpush`: [1,550,387.62, 1,515,151.50, 1,562,499.88]
- `highpipe / hz4 / lpop`: [1,639,344.25, 1,612,903.25, 1,639,344.25]
- `kv_only / jemalloc / set`: [1,153,846.25, 1,219,512.12, 1,200,000.00]
- `kv_only / jemalloc / get`: [1,339,285.62, 1,363,636.38, 1,363,636.38]
- `kv_only / tcmalloc / set`: [1,315,789.50, 1,282,051.25, 1,327,433.62]
- `kv_only / tcmalloc / get`: [1,500,000.00, 1,500,000.00, 1,485,148.50]
- `kv_only / mimalloc / set`: [1,293,103.50, 1,376,146.88, 1,304,347.75]
- `kv_only / mimalloc / get`: [1,530,612.25, 1,515,151.50, 1,530,612.25]
- `kv_only / hz3 / set`: [1,351,351.38, 1,351,351.38, 1,388,888.88]
- `kv_only / hz3 / get`: [1,515,151.50, 1,546,391.75, 1,485,148.50]
- `kv_only / hz4 / set`: [1,271,186.38, 1,260,504.12, 1,339,285.62]
- `kv_only / hz4 / get`: [1,485,148.50, 1,428,571.50, 1,485,148.50]
- `list_only / jemalloc / lpush`: [993,377.50, 980,392.19, 1,013,513.50]
- `list_only / jemalloc / lpop`: [1,102,941.12, 1,094,890.50, 1,119,403.00]
- `list_only / tcmalloc / lpush`: [1,260,504.12, 1,239,669.38, 1,171,875.00]
- `list_only / tcmalloc / lpop`: [1,282,051.25, 1,315,789.50, 1,282,051.25]
- `list_only / mimalloc / lpush`: [1,250,000.00, 1,282,051.25, 1,271,186.38]
- `list_only / mimalloc / lpop`: [1,351,351.38, 1,351,351.38, 1,339,285.62]
- `list_only / hz3 / lpush`: [1,219,512.12, 1,271,186.38, 1,200,000.00]
- `list_only / hz3 / lpop`: [1,327,433.62, 1,315,789.50, 1,376,146.88]
- `list_only / hz4 / lpush`: [1,071,428.62, 1,071,428.62, 1,094,890.50]
- `list_only / hz4 / lpop`: [1,190,476.12, 1,200,000.00, 1,190,476.12]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260321_044828_redis_real_windows_profile_matrix.log`
