# Windows Memcached External Client Allocator Matrix

Generated: 2026-05-12 22:26:51 +09:00
Runs per profile/allocator: 5

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec median | drop median | slot median | size | secs | runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| larger_payload_r90_4x16 | hz3 | 309,186.61 | 0 | 0 | 256 | 3 | 5 |
| larger_payload_r90_4x16 | hz4 | 308,004.54 | 0 | 0 | 256 | 3 | 5 |

Per-run detail:
- `larger_payload_r90_4x16 / hz3`: ops/sec [228,417.70, 321,827.60, 324,037.29, 309,186.61, 295,687.21] drop [0, 0, 0, 0, 0] slots [0, 0, 0, 0, 0]
- `larger_payload_r90_4x16 / hz4`: ops/sec [303,170.65, 300,222.31, 308,004.54, 318,285.70, 310,720.48] drop [0, 0, 0, 0, 0] slots [0, 0, 0, 0, 0]

Raw log archive: `private/raw-results/windows/memcached/external_client_allocator_matrix/20260512_222651_memcached_external_client_allocator_matrix.log`
