# Windows Real Redis Profile Matrix

Generated: 2026-03-10 17:31:20 +09:00

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
| lowpipe | jemalloc | set | 130,208.34 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | get | 132,978.73 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | lpush | 130,208.34 | 40 | 1 | 50000 | 3 |
| lowpipe | jemalloc | lpop | 130,208.34 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | set | 128,534.70 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | get | 131,578.95 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | lpush | 132,626.00 | 40 | 1 | 50000 | 3 |
| lowpipe | tcmalloc | lpop | 131,233.59 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | set | 130,890.05 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | get | 131,926.12 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | lpush | 130,208.34 | 40 | 1 | 50000 | 3 |
| lowpipe | mimalloc | lpop | 122,850.12 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | set | 128,534.70 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | get | 130,890.05 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | lpush | 129,533.68 | 40 | 1 | 50000 | 3 |
| lowpipe | hz3 | lpop | 126,903.55 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | set | 129,533.68 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | get | 131,926.12 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | lpush | 127,877.23 | 40 | 1 | 50000 | 3 |
| lowpipe | hz4 | lpop | 121,654.50 | 40 | 1 | 50000 | 3 |
| highpipe | jemalloc | set | 1,680,672.25 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | get | 1,980,198.00 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | lpush | 1,351,351.38 | 80 | 32 | 200000 | 3 |
| highpipe | jemalloc | lpop | 1,503,759.38 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | set | 1,904,762.00 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | get | 2,272,727.25 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | lpush | 1,754,386.00 | 80 | 32 | 200000 | 3 |
| highpipe | tcmalloc | lpop | 1,834,862.38 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | set | 2,000,000.00 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | get | 2,298,850.75 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | lpush | 1,801,801.75 | 80 | 32 | 200000 | 3 |
| highpipe | mimalloc | lpop | 1,886,792.50 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | set | 2,000,000.00 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | get | 2,352,941.25 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | lpush | 1,818,181.88 | 80 | 32 | 200000 | 3 |
| highpipe | hz3 | lpop | 1,923,076.88 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | set | 1,960,784.38 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | get | 2,272,727.25 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | lpush | 1,626,016.25 | 80 | 32 | 200000 | 3 |
| highpipe | hz4 | lpop | 1,709,401.75 | 80 | 32 | 200000 | 3 |

Per-run detail:
- `lowpipe / jemalloc / set`: [126,582.27, 124,069.48, 130,208.34]
- `lowpipe / jemalloc / get`: [130,208.34, 131,233.59, 132,978.73]
- `lowpipe / jemalloc / lpush`: [128,534.70, 127,877.23, 130,208.34]
- `lowpipe / jemalloc / lpop`: [127,551.02, 128,205.13, 130,208.34]
- `lowpipe / tcmalloc / set`: [127,226.46, 128,534.70, 125,000.00]
- `lowpipe / tcmalloc / get`: [128,534.70, 131,578.95, 129,533.68]
- `lowpipe / tcmalloc / lpush`: [132,626.00, 130,548.30, 129,870.13]
- `lowpipe / tcmalloc / lpop`: [128,865.98, 131,233.59, 129,198.97]
- `lowpipe / mimalloc / set`: [125,313.29, 130,890.05, 128,865.98]
- `lowpipe / mimalloc / get`: [129,533.68, 130,890.05, 131,926.12]
- `lowpipe / mimalloc / lpush`: [104,821.80, 130,208.34, 127,226.46]
- `lowpipe / mimalloc / lpop`: [116,009.28, 104,602.52, 122,850.12]
- `lowpipe / hz3 / set`: [122,549.02, 128,534.70, 128,534.70]
- `lowpipe / hz3 / get`: [126,582.27, 129,533.68, 130,890.05]
- `lowpipe / hz3 / lpush`: [128,205.13, 125,944.58, 129,533.68]
- `lowpipe / hz3 / lpop`: [117,924.53, 120,481.93, 126,903.55]
- `lowpipe / hz4 / set`: [126,582.27, 126,262.62, 129,533.68]
- `lowpipe / hz4 / get`: [129,870.13, 131,926.12, 128,865.98]
- `lowpipe / hz4 / lpush`: [125,313.29, 127,877.23, 125,944.58]
- `lowpipe / hz4 / lpop`: [120,481.93, 112,359.55, 121,654.50]
- `highpipe / jemalloc / set`: [1,587,301.50, 1,652,892.62, 1,680,672.25]
- `highpipe / jemalloc / get`: [1,923,076.88, 1,960,784.38, 1,980,198.00]
- `highpipe / jemalloc / lpush`: [1,307,189.62, 1,351,351.38, 1,333,333.25]
- `highpipe / jemalloc / lpop`: [1,470,588.12, 1,481,481.38, 1,503,759.38]
- `highpipe / tcmalloc / set`: [1,904,762.00, 1,851,851.75, 1,886,792.50]
- `highpipe / tcmalloc / get`: [2,222,222.25, 2,272,727.25, 2,222,222.25]
- `highpipe / tcmalloc / lpush`: [1,709,401.75, 1,754,386.00, 1,724,138.00]
- `highpipe / tcmalloc / lpop`: [1,818,181.88, 1,834,862.38, 1,834,862.38]
- `highpipe / mimalloc / set`: [1,941,747.62, 2,000,000.00, 1,980,198.00]
- `highpipe / mimalloc / get`: [2,247,191.00, 2,247,191.00, 2,298,850.75]
- `highpipe / mimalloc / lpush`: [1,801,801.75, 1,739,130.38, 1,769,911.50]
- `highpipe / mimalloc / lpop`: [1,886,792.50, 1,851,851.75, 1,886,792.50]
- `highpipe / hz3 / set`: [1,980,198.00, 1,904,762.00, 2,000,000.00]
- `highpipe / hz3 / get`: [2,247,191.00, 2,352,941.25, 2,325,581.25]
- `highpipe / hz3 / lpush`: [1,785,714.25, 1,818,181.88, 1,818,181.88]
- `highpipe / hz3 / lpop`: [1,869,158.88, 1,923,076.88, 1,886,792.50]
- `highpipe / hz4 / set`: [1,960,784.38, 1,886,792.50, 1,960,784.38]
- `highpipe / hz4 / get`: [2,173,913.00, 2,247,191.00, 2,272,727.25]
- `highpipe / hz4 / lpush`: [1,600,000.00, 1,587,301.50, 1,626,016.25]
- `highpipe / hz4 / lpop`: [1,694,915.25, 1,709,401.75, 1,694,915.25]

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_profile_matrix\20260310_173120_redis_real_windows_profile_matrix.log`
