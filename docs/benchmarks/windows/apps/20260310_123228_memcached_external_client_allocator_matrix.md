# Windows Memcached External Client Allocator Matrix

Generated: 2026-03-10 12:32:28 +09:00
Runs per profile/allocator: 5

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec median | drop median | slot median | size | secs | runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced_4x16 | crt | 302,875.09 | 0 | 64 | 64 | 3 | 5 |
| balanced_4x16 | hz3 | 342,971.65 | 0 | 64 | 64 | 3 | 5 |
| balanced_4x16 | hz4 | 312,939.85 | 0 | 64 | 64 | 3 | 5 |
| balanced_4x16 | mimalloc | 330,056.11 | 0 | 64 | 64 | 3 | 5 |
| balanced_4x16 | tcmalloc | 336,591.12 | 0 | 64 | 64 | 3 | 5 |
| larger_payload_4x16 | crt | 357,877.47 | 0 | 64 | 256 | 3 | 5 |
| larger_payload_4x16 | hz3 | 340,825.66 | 0 | 64 | 256 | 3 | 5 |
| larger_payload_4x16 | hz4 | 336,877.55 | 0 | 64 | 256 | 3 | 5 |
| larger_payload_4x16 | mimalloc | 344,732.05 | 0 | 64 | 256 | 3 | 5 |
| larger_payload_4x16 | tcmalloc | 346,926.69 | 0 | 64 | 256 | 3 | 5 |
| scale_8x16 | crt | 241,951.42 | 0 | 128 | 64 | 5 | 5 |
| scale_8x16 | hz3 | 289,561.64 | 0 | 128 | 64 | 5 | 5 |
| scale_8x16 | hz4 | 353,966.75 | 0 | 128 | 64 | 5 | 5 |
| scale_8x16 | mimalloc | 331,862.21 | 0 | 128 | 64 | 5 | 5 |
| scale_8x16 | tcmalloc | 254,263.66 | 0 | 128 | 64 | 5 | 5 |

Per-run detail:
- `balanced_4x16 / crt`: ops/sec [282,515.63, 302,875.09, 238,125.15, 319,967.87, 418,754.16] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `balanced_4x16 / hz3`: ops/sec [475,520.75, 342,971.65, 540,985.97, 327,382.68, 263,886.49] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `balanced_4x16 / hz4`: ops/sec [527,119.50, 256,456.41, 262,898.28, 350,501.99, 312,939.85] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `balanced_4x16 / mimalloc`: ops/sec [327,675.53, 463,824.00, 330,056.11, 357,867.68, 298,687.13] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `balanced_4x16 / tcmalloc`: ops/sec [336,591.12, 340,603.67, 334,173.87, 274,173.93, 353,304.02] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `larger_payload_4x16 / crt`: ops/sec [318,499.39, 359,363.16, 357,877.47, 337,717.85, 520,346.53] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `larger_payload_4x16 / hz3`: ops/sec [331,724.55, 447,414.35, 355,574.32, 319,355.13, 340,825.66] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `larger_payload_4x16 / hz4`: ops/sec [336,877.55, 253,457.46, 344,781.38, 258,642.99, 338,190.40] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `larger_payload_4x16 / mimalloc`: ops/sec [370,110.91, 265,825.59, 245,975.53, 347,216.74, 344,732.05] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `larger_payload_4x16 / tcmalloc`: ops/sec [510,767.55, 354,859.18, 250,370.93, 329,536.33, 346,926.69] drop [0, 0, 0, 0, 0] slots [64, 64, 64, 64, 64]
- `scale_8x16 / crt`: ops/sec [285,652.85, 241,951.42, 240,476.69, 289,764.29, 241,242.06] drop [0, 0, 0, 0, 0] slots [128, 128, 128, 128, 128]
- `scale_8x16 / hz3`: ops/sec [232,470.72, 291,807.98, 297,335.95, 238,574.26, 289,561.64] drop [0, 0, 0, 0, 0] slots [128, 128, 128, 128, 128]
- `scale_8x16 / hz4`: ops/sec [288,361.82, 353,966.75, 363,307.20, 278,778.00, 355,639.46] drop [0, 0, 0, 0, 0] slots [128, 128, 128, 128, 128]
- `scale_8x16 / mimalloc`: ops/sec [228,147.49, 334,199.31, 331,862.21, 386,310.10, 306,703.10] drop [0, 0, 0, 0, 0] slots [128, 128, 128, 128, 128]
- `scale_8x16 / tcmalloc`: ops/sec [254,263.66, 250,856.23, 372,688.23, 246,257.15, 379,077.90] drop [0, 0, 0, 0, 0] slots [128, 128, 128, 128, 128]

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\20260310_123228_memcached_external_client_allocator_matrix.log`
