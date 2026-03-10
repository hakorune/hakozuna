# Windows Memcached External Client Allocator Matrix

Generated: 2026-03-10 12:30:15 +09:00
Runs per profile/allocator: 1

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec median | drop median | slot median | size | secs | runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced_4x16 | crt | 365,093.97 | 0 | 64 | 64 | 3 | 1 |
| balanced_4x16 | hz3 | 324,667.53 | 0 | 64 | 64 | 3 | 1 |
| balanced_4x16 | hz4 | 544,038.10 | 0 | 64 | 64 | 3 | 1 |
| balanced_4x16 | mimalloc | 376,595.61 | 0 | 64 | 64 | 3 | 1 |
| balanced_4x16 | tcmalloc | 364,078.62 | 0 | 64 | 64 | 3 | 1 |
| larger_payload_4x16 | crt | 381,033.59 | 0 | 64 | 256 | 3 | 1 |
| larger_payload_4x16 | hz3 | 270,263.94 | 0 | 64 | 256 | 3 | 1 |
| larger_payload_4x16 | hz4 | 282,511.77 | 0 | 64 | 256 | 3 | 1 |
| larger_payload_4x16 | mimalloc | 384,963.81 | 0 | 64 | 256 | 3 | 1 |
| larger_payload_4x16 | tcmalloc | 349,382.40 | 0 | 64 | 256 | 3 | 1 |
| scale_8x16 | crt | 420,185.10 | 0 | 128 | 64 | 5 | 1 |
| scale_8x16 | hz3 | 333,151.05 | 0 | 128 | 64 | 5 | 1 |
| scale_8x16 | hz4 | 432,461.52 | 0 | 128 | 64 | 5 | 1 |
| scale_8x16 | mimalloc | 316,326.96 | 0 | 128 | 64 | 5 | 1 |
| scale_8x16 | tcmalloc | 293,959.88 | 0 | 128 | 64 | 5 | 1 |

Per-run detail:
- `balanced_4x16 / crt`: ops/sec [365,093.97] drop [0] slots [64]
- `balanced_4x16 / hz3`: ops/sec [324,667.53] drop [0] slots [64]
- `balanced_4x16 / hz4`: ops/sec [544,038.10] drop [0] slots [64]
- `balanced_4x16 / mimalloc`: ops/sec [376,595.61] drop [0] slots [64]
- `balanced_4x16 / tcmalloc`: ops/sec [364,078.62] drop [0] slots [64]
- `larger_payload_4x16 / crt`: ops/sec [381,033.59] drop [0] slots [64]
- `larger_payload_4x16 / hz3`: ops/sec [270,263.94] drop [0] slots [64]
- `larger_payload_4x16 / hz4`: ops/sec [282,511.77] drop [0] slots [64]
- `larger_payload_4x16 / mimalloc`: ops/sec [384,963.81] drop [0] slots [64]
- `larger_payload_4x16 / tcmalloc`: ops/sec [349,382.40] drop [0] slots [64]
- `scale_8x16 / crt`: ops/sec [420,185.10] drop [0] slots [128]
- `scale_8x16 / hz3`: ops/sec [333,151.05] drop [0] slots [128]
- `scale_8x16 / hz4`: ops/sec [432,461.52] drop [0] slots [128]
- `scale_8x16 / mimalloc`: ops/sec [316,326.96] drop [0] slots [128]
- `scale_8x16 / tcmalloc`: ops/sec [293,959.88] drop [0] slots [128]

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\20260310_123015_memcached_external_client_allocator_matrix.log`
