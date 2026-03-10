# Windows Memcached External Client Allocator Matrix

Generated: 2026-03-10 13:20:51 +09:00
Runs per profile/allocator: 3

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec median | drop median | slot median | size | secs | runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced_1x64 | crt | 125,940.97 | 0 | 64 | 64 | 2 | 3 |
| balanced_1x64 | hz3 | 112,373.60 | 0 | 64 | 64 | 2 | 3 |
| balanced_1x64 | hz4 | 120,264.29 | 0 | 64 | 64 | 2 | 3 |
| balanced_1x64 | mimalloc | 136,039.57 | 0 | 64 | 64 | 2 | 3 |
| balanced_1x64 | tcmalloc | 125,669.44 | 0 | 64 | 64 | 2 | 3 |
| balanced_2x32 | crt | 255,461.16 | 0 | 64 | 64 | 2 | 3 |
| balanced_2x32 | hz3 | 227,584.17 | 0 | 64 | 64 | 2 | 3 |
| balanced_2x32 | hz4 | 217,075.04 | 0 | 64 | 64 | 2 | 3 |
| balanced_2x32 | mimalloc | 195,537.99 | 0 | 64 | 64 | 2 | 3 |
| balanced_2x32 | tcmalloc | 250,997.54 | 0 | 64 | 64 | 2 | 3 |
| balanced_4x16 | crt | 367,422.84 | 0 | 64 | 64 | 3 | 3 |
| balanced_4x16 | hz3 | 412,289.11 | 0 | 64 | 64 | 3 | 3 |
| balanced_4x16 | hz4 | 329,115.28 | 0 | 64 | 64 | 3 | 3 |
| balanced_4x16 | mimalloc | 339,212.91 | 0 | 64 | 64 | 3 | 3 |
| balanced_4x16 | tcmalloc | 518,404.07 | 0 | 64 | 64 | 3 | 3 |
| read_heavy_4x16 | crt | 366,806.11 | 0 | 64 | 64 | 3 | 3 |
| read_heavy_4x16 | hz3 | 412,444.43 | 0 | 64 | 64 | 3 | 3 |
| read_heavy_4x16 | hz4 | 420,253.77 | 0 | 64 | 64 | 3 | 3 |
| read_heavy_4x16 | mimalloc | 605,146.89 | 0 | 64 | 64 | 3 | 3 |
| read_heavy_4x16 | tcmalloc | 536,538.35 | 0 | 64 | 64 | 3 | 3 |
| write_heavy_4x16 | crt | 339,538.87 | 0 | 64 | 64 | 3 | 3 |
| write_heavy_4x16 | hz3 | 341,288.80 | 0 | 64 | 64 | 3 | 3 |
| write_heavy_4x16 | hz4 | 526,121.36 | 0 | 64 | 64 | 3 | 3 |
| write_heavy_4x16 | mimalloc | 558,239.97 | 0 | 64 | 64 | 3 | 3 |
| write_heavy_4x16 | tcmalloc | 410,896.23 | 0 | 64 | 64 | 3 | 3 |
| larger_payload_4x16 | crt | 381,344.38 | 0 | 64 | 256 | 3 | 3 |
| larger_payload_4x16 | hz3 | 572,178.24 | 0 | 64 | 256 | 3 | 3 |
| larger_payload_4x16 | hz4 | 372,231.26 | 0 | 64 | 256 | 3 | 3 |
| larger_payload_4x16 | mimalloc | 596,192.63 | 0 | 64 | 256 | 3 | 3 |
| larger_payload_4x16 | tcmalloc | 562,957.37 | 0 | 64 | 256 | 3 | 3 |
| long_balanced_4x16 | crt | 377,573.52 | 0 | 64 | 64 | 10 | 3 |
| long_balanced_4x16 | hz3 | 386,808.08 | 0 | 64 | 64 | 10 | 3 |
| long_balanced_4x16 | hz4 | 318,636.24 | 0 | 64 | 64 | 10 | 3 |
| long_balanced_4x16 | mimalloc | 284,732.32 | 0 | 64 | 64 | 10 | 3 |
| long_balanced_4x16 | tcmalloc | 285,559.86 | 0 | 64 | 64 | 10 | 3 |
| scale_8x16 | crt | 294,699.57 | 0 | 128 | 64 | 5 | 3 |
| scale_8x16 | hz3 | 358,347.81 | 0 | 128 | 64 | 5 | 3 |
| scale_8x16 | hz4 | 270,541.80 | 0 | 128 | 64 | 5 | 3 |
| scale_8x16 | mimalloc | 266,543.85 | 0 | 128 | 64 | 5 | 3 |
| scale_8x16 | tcmalloc | 246,079.79 | 0 | 128 | 64 | 5 | 3 |

Per-run detail:
- `balanced_1x64 / crt`: ops/sec [105,240.08, 125,940.97, 97,813.50] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_1x64 / hz3`: ops/sec [110,008.21, 112,373.60, 99,006.56] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_1x64 / hz4`: ops/sec [120,264.29, 109,664.43, 94,655.40] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_1x64 / mimalloc`: ops/sec [116,098.39, 136,039.57, 109,535.48] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_1x64 / tcmalloc`: ops/sec [109,856.72, 125,669.44, 102,294.10] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_2x32 / crt`: ops/sec [240,890.70, 255,461.16, 194,820.95] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_2x32 / hz3`: ops/sec [158,142.58, 227,584.17, 198,924.85] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_2x32 / hz4`: ops/sec [206,242.53, 217,075.04, 212,591.06] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_2x32 / mimalloc`: ops/sec [195,537.99, 150,062.00, 195,028.14] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_2x32 / tcmalloc`: ops/sec [233,114.13, 250,997.54, 191,370.39] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_4x16 / crt`: ops/sec [272,792.46, 367,422.84, 273,852.05] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_4x16 / hz3`: ops/sec [296,978.18, 412,289.11, 276,267.05] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_4x16 / hz4`: ops/sec [230,858.33, 329,115.28, 262,600.71] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_4x16 / mimalloc`: ops/sec [287,343.06, 339,212.91, 268,124.72] drop [0, 0, 0] slots [64, 64, 64]
- `balanced_4x16 / tcmalloc`: ops/sec [358,550.69, 518,404.07, 362,848.47] drop [0, 0, 0] slots [64, 64, 64]
- `read_heavy_4x16 / crt`: ops/sec [364,177.03, 348,247.78, 366,806.11] drop [0, 0, 0] slots [64, 64, 64]
- `read_heavy_4x16 / hz3`: ops/sec [412,444.43, 324,552.70, 397,801.36] drop [0, 0, 0] slots [64, 64, 64]
- `read_heavy_4x16 / hz4`: ops/sec [306,760.77, 358,066.00, 420,253.77] drop [0, 0, 0] slots [64, 64, 64]
- `read_heavy_4x16 / mimalloc`: ops/sec [302,437.71, 351,513.47, 605,146.89] drop [0, 0, 0] slots [64, 64, 64]
- `read_heavy_4x16 / tcmalloc`: ops/sec [331,720.49, 491,407.23, 536,538.35] drop [0, 0, 0] slots [64, 64, 64]
- `write_heavy_4x16 / crt`: ops/sec [245,857.49, 319,788.59, 339,538.87] drop [0, 0, 0] slots [64, 64, 64]
- `write_heavy_4x16 / hz3`: ops/sec [336,141.88, 334,361.97, 341,288.80] drop [0, 0, 0] slots [64, 64, 64]
- `write_heavy_4x16 / hz4`: ops/sec [341,114.83, 526,121.36, 278,528.01] drop [0, 0, 0] slots [64, 64, 64]
- `write_heavy_4x16 / mimalloc`: ops/sec [373,778.39, 558,239.97, 350,194.70] drop [0, 0, 0] slots [64, 64, 64]
- `write_heavy_4x16 / tcmalloc`: ops/sec [369,184.91, 410,896.23, 267,877.90] drop [0, 0, 0] slots [64, 64, 64]
- `larger_payload_4x16 / crt`: ops/sec [350,279.09, 297,885.23, 381,344.38] drop [0, 0, 0] slots [64, 64, 64]
- `larger_payload_4x16 / hz3`: ops/sec [572,178.24, 389,217.28, 333,217.92] drop [0, 0, 0] slots [64, 64, 64]
- `larger_payload_4x16 / hz4`: ops/sec [372,231.26, 289,307.07, 319,900.50] drop [0, 0, 0] slots [64, 64, 64]
- `larger_payload_4x16 / mimalloc`: ops/sec [359,127.31, 596,192.63, 356,863.89] drop [0, 0, 0] slots [64, 64, 64]
- `larger_payload_4x16 / tcmalloc`: ops/sec [291,375.28, 406,740.75, 562,957.37] drop [0, 0, 0] slots [64, 64, 64]
- `long_balanced_4x16 / crt`: ops/sec [377,573.52, 345,128.89, 376,876.21] drop [0, 0, 0] slots [64, 64, 64]
- `long_balanced_4x16 / hz3`: ops/sec [386,808.08, 347,924.13, 298,387.05] drop [0, 0, 0] slots [64, 64, 64]
- `long_balanced_4x16 / hz4`: ops/sec [292,448.00, 318,636.24, 300,439.80] drop [0, 0, 0] slots [64, 64, 64]
- `long_balanced_4x16 / mimalloc`: ops/sec [284,732.32, 263,811.26, 281,398.68] drop [0, 0, 0] slots [64, 64, 64]
- `long_balanced_4x16 / tcmalloc`: ops/sec [285,559.86, 273,983.93, 267,834.36] drop [0, 0, 0] slots [64, 64, 64]
- `scale_8x16 / crt`: ops/sec [247,752.73, 248,273.43, 294,699.57] drop [0, 0, 0] slots [128, 128, 128]
- `scale_8x16 / hz3`: ops/sec [221,102.83, 228,427.86, 358,347.81] drop [0, 0, 0] slots [128, 128, 128]
- `scale_8x16 / hz4`: ops/sec [270,541.80, 255,564.34, 263,531.21] drop [0, 0, 0] slots [128, 128, 128]
- `scale_8x16 / mimalloc`: ops/sec [226,568.63, 266,543.85, 250,085.68] drop [0, 0, 0] slots [128, 128, 128]
- `scale_8x16 / tcmalloc`: ops/sec [206,982.19, 203,989.08, 246,079.79] drop [0, 0, 0] slots [128, 128, 128]

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\20260310_132051_memcached_external_client_allocator_matrix.log`
