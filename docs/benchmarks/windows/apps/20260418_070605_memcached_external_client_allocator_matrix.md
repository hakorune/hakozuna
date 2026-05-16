# Windows Memcached External Client Allocator Matrix

Generated: 2026-04-18 07:06:05 +09:00
Runs per profile/allocator: 1

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec median | drop median | slot median | size | secs | runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced_1x64 | crt | 98,788.07 | 0 | 0 | 64 | 2 | 1 |
| balanced_1x64 | hz3 | 97,483.62 | 0 | 0 | 64 | 2 | 1 |
| balanced_1x64 | hz4 | 102,472.82 | 0 | 0 | 64 | 2 | 1 |
| balanced_1x64 | mimalloc | 101,559.69 | 0 | 0 | 64 | 2 | 1 |
| balanced_1x64 | tcmalloc | 101,803.98 | 0 | 0 | 64 | 2 | 1 |
| balanced_2x32 | crt | 201,326.14 | 0 | 0 | 64 | 2 | 1 |
| balanced_2x32 | hz3 | 202,757.18 | 0 | 0 | 64 | 2 | 1 |
| balanced_2x32 | hz4 | 203,671.32 | 0 | 0 | 64 | 2 | 1 |
| balanced_2x32 | mimalloc | 200,880.30 | 0 | 0 | 64 | 2 | 1 |
| balanced_2x32 | tcmalloc | 201,439.14 | 0 | 0 | 64 | 2 | 1 |
| balanced_4x16 | crt | 329,663.35 | 0 | 0 | 64 | 3 | 1 |
| balanced_4x16 | hz3 | 330,588.11 | 0 | 0 | 64 | 3 | 1 |
| balanced_4x16 | hz4 | 249,446.33 | 0 | 0 | 64 | 3 | 1 |
| balanced_4x16 | mimalloc | 248,907.16 | 0 | 0 | 64 | 3 | 1 |
| balanced_4x16 | tcmalloc | 327,391.13 | 0 | 0 | 64 | 3 | 1 |
| read_heavy_4x16 | crt | 246,532.47 | 0 | 0 | 64 | 3 | 1 |
| read_heavy_4x16 | hz3 | 327,570.59 | 0 | 0 | 64 | 3 | 1 |
| read_heavy_4x16 | hz4 | 479,489.11 | 0 | 0 | 64 | 3 | 1 |
| read_heavy_4x16 | mimalloc | 330,033.48 | 0 | 0 | 64 | 3 | 1 |
| read_heavy_4x16 | tcmalloc | 335,654.27 | 0 | 0 | 64 | 3 | 1 |
| write_heavy_4x16 | crt | 325,007.44 | 0 | 0 | 64 | 3 | 1 |
| write_heavy_4x16 | hz3 | 484,020.80 | 0 | 0 | 64 | 3 | 1 |
| write_heavy_4x16 | hz4 | 243,258.45 | 0 | 0 | 64 | 3 | 1 |
| write_heavy_4x16 | mimalloc | 245,932.38 | 0 | 0 | 64 | 3 | 1 |
| write_heavy_4x16 | tcmalloc | 319,155.05 | 0 | 0 | 64 | 3 | 1 |
| larger_payload_4x16 | crt | 244,192.34 | 0 | 0 | 256 | 3 | 1 |
| larger_payload_4x16 | hz3 | 331,187.41 | 0 | 0 | 256 | 3 | 1 |
| larger_payload_4x16 | hz4 | 326,754.84 | 0 | 0 | 256 | 3 | 1 |
| larger_payload_4x16 | mimalloc | 330,261.11 | 0 | 0 | 256 | 3 | 1 |
| larger_payload_4x16 | tcmalloc | 329,070.70 | 0 | 0 | 256 | 3 | 1 |
| long_balanced_4x16 | crt | 304,438.39 | 0 | 0 | 64 | 10 | 1 |
| long_balanced_4x16 | hz3 | 305,236.73 | 0 | 0 | 64 | 10 | 1 |
| long_balanced_4x16 | hz4 | 302,881.60 | 0 | 0 | 64 | 10 | 1 |
| long_balanced_4x16 | mimalloc | 306,147.14 | 0 | 0 | 64 | 10 | 1 |
| long_balanced_4x16 | tcmalloc | 308,190.63 | 0 | 0 | 64 | 10 | 1 |
| scale_8x16 | crt | 528.19 | 61 | 0 | 64 | 5 | 1 |
| scale_8x16 | hz3 | 521.75 | 61 | 0 | 64 | 5 | 1 |
| scale_8x16 | hz4 | 528.83 | 61 | 0 | 64 | 5 | 1 |
| scale_8x16 | mimalloc | 513.96 | 62 | 0 | 64 | 5 | 1 |
| scale_8x16 | tcmalloc | 522.79 | 61 | 0 | 64 | 5 | 1 |

Per-run detail:
- `balanced_1x64 / crt`: ops/sec [98,788.07] drop [0] slots [0]
- `balanced_1x64 / hz3`: ops/sec [97,483.62] drop [0] slots [0]
- `balanced_1x64 / hz4`: ops/sec [102,472.82] drop [0] slots [0]
- `balanced_1x64 / mimalloc`: ops/sec [101,559.69] drop [0] slots [0]
- `balanced_1x64 / tcmalloc`: ops/sec [101,803.98] drop [0] slots [0]
- `balanced_2x32 / crt`: ops/sec [201,326.14] drop [0] slots [0]
- `balanced_2x32 / hz3`: ops/sec [202,757.18] drop [0] slots [0]
- `balanced_2x32 / hz4`: ops/sec [203,671.32] drop [0] slots [0]
- `balanced_2x32 / mimalloc`: ops/sec [200,880.30] drop [0] slots [0]
- `balanced_2x32 / tcmalloc`: ops/sec [201,439.14] drop [0] slots [0]
- `balanced_4x16 / crt`: ops/sec [329,663.35] drop [0] slots [0]
- `balanced_4x16 / hz3`: ops/sec [330,588.11] drop [0] slots [0]
- `balanced_4x16 / hz4`: ops/sec [249,446.33] drop [0] slots [0]
- `balanced_4x16 / mimalloc`: ops/sec [248,907.16] drop [0] slots [0]
- `balanced_4x16 / tcmalloc`: ops/sec [327,391.13] drop [0] slots [0]
- `read_heavy_4x16 / crt`: ops/sec [246,532.47] drop [0] slots [0]
- `read_heavy_4x16 / hz3`: ops/sec [327,570.59] drop [0] slots [0]
- `read_heavy_4x16 / hz4`: ops/sec [479,489.11] drop [0] slots [0]
- `read_heavy_4x16 / mimalloc`: ops/sec [330,033.48] drop [0] slots [0]
- `read_heavy_4x16 / tcmalloc`: ops/sec [335,654.27] drop [0] slots [0]
- `write_heavy_4x16 / crt`: ops/sec [325,007.44] drop [0] slots [0]
- `write_heavy_4x16 / hz3`: ops/sec [484,020.80] drop [0] slots [0]
- `write_heavy_4x16 / hz4`: ops/sec [243,258.45] drop [0] slots [0]
- `write_heavy_4x16 / mimalloc`: ops/sec [245,932.38] drop [0] slots [0]
- `write_heavy_4x16 / tcmalloc`: ops/sec [319,155.05] drop [0] slots [0]
- `larger_payload_4x16 / crt`: ops/sec [244,192.34] drop [0] slots [0]
- `larger_payload_4x16 / hz3`: ops/sec [331,187.41] drop [0] slots [0]
- `larger_payload_4x16 / hz4`: ops/sec [326,754.84] drop [0] slots [0]
- `larger_payload_4x16 / mimalloc`: ops/sec [330,261.11] drop [0] slots [0]
- `larger_payload_4x16 / tcmalloc`: ops/sec [329,070.70] drop [0] slots [0]
- `long_balanced_4x16 / crt`: ops/sec [304,438.39] drop [0] slots [0]
- `long_balanced_4x16 / hz3`: ops/sec [305,236.73] drop [0] slots [0]
- `long_balanced_4x16 / hz4`: ops/sec [302,881.60] drop [0] slots [0]
- `long_balanced_4x16 / mimalloc`: ops/sec [306,147.14] drop [0] slots [0]
- `long_balanced_4x16 / tcmalloc`: ops/sec [308,190.63] drop [0] slots [0]
- `scale_8x16 / crt`: ops/sec [528.19] drop [61] slots [0]
- `scale_8x16 / hz3`: ops/sec [521.75] drop [61] slots [0]
- `scale_8x16 / hz4`: ops/sec [528.83] drop [61] slots [0]
- `scale_8x16 / mimalloc`: ops/sec [513.96] drop [62] slots [0]
- `scale_8x16 / tcmalloc`: ops/sec [522.79] drop [61] slots [0]

Raw log archive: `private/raw-results/windows/memcached/external_client_allocator_matrix/20260418_070605_memcached_external_client_allocator_matrix.log`
