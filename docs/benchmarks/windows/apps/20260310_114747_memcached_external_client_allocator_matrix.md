# Windows Memcached External Client Allocator Matrix

Generated: 2026-03-10 11:47:47 +09:00

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec | drop count | slot count | size | secs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| balanced_4x16 | crt | 361072.95 | 0 | 64 | 64 | 3 |
| balanced_4x16 | hz3 | 180645.33 | 0 | 64 | 64 | 3 |
| balanced_4x16 | hz4 | 182510.84 | 0 | 64 | 64 | 3 |
| balanced_4x16 | mimalloc | 233308.83 | 0 | 64 | 64 | 3 |
| balanced_4x16 | tcmalloc | 366099.13 | 0 | 64 | 64 | 3 |
| larger_payload_4x16 | crt | 184432.96 | 0 | 64 | 256 | 3 |
| larger_payload_4x16 | hz3 | 239196.43 | 0 | 64 | 256 | 3 |
| larger_payload_4x16 | hz4 | 233405.75 | 0 | 64 | 256 | 3 |
| larger_payload_4x16 | mimalloc | 183265.67 | 0 | 64 | 256 | 3 |
| larger_payload_4x16 | tcmalloc | 244151.96 | 0 | 64 | 256 | 3 |
| scale_8x16 | crt | 227515.75 | 0 | 128 | 64 | 5 |
| scale_8x16 | hz3 | 236721.10 | 0 | 128 | 64 | 5 |
| scale_8x16 | hz4 | 198506.16 | 0 | 128 | 64 | 5 |
| scale_8x16 | mimalloc | 227141.10 | 0 | 128 | 64 | 5 |
| scale_8x16 | tcmalloc | 195987.68 | 0 | 128 | 64 | 5 |

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\20260310_114747_memcached_external_client_allocator_matrix.log`
