# Windows Memcached External Client Allocator Matrix

Generated: 2026-05-12 22:28:37 +09:00
Runs per profile/allocator: 5

References:
- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)

| profile | allocator | ops/sec median | peak rss kb median | final rss kb median | steady rss kb median | private kb median | peak va kb median | current va kb median | drop median | slot median | size | secs | runs |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| larger_payload_r90_4x16 | hz3 | 304,579.35 | 15272 | 15272 | 15272 | 130372 | 71519928 | 71519928 | 0 | 0 | 256 | 3 | 5 |
| larger_payload_r90_4x16 | hz4 | 293,095.18 | 13204 | 13204 | 13204 | 21288 | 4322228 | 4322228 | 0 | 0 | 256 | 3 | 5 |

Per-run detail:
- `larger_payload_r90_4x16 / hz3`: ops/sec [465,106.65, 304,579.35, 445,073.17, 209,607.84, 304,515.49] peak rss kb [15276, 15256, 15272, 15272, 15284] final rss kb [15276, 15256, 15272, 15272, 15284] steady rss kb [15276, 15256, 15272, 15272, 15284] private kb [130380, 130356, 130360, 130372, 130380] peak va kb [71519928, 71519928, 71519928, 71519928, 71519928] current va kb [71519928, 71519928, 71519928, 71519928, 71519928] drop [0, 0, 0, 0, 0] slots [0, 0, 0, 0, 0]
- `larger_payload_r90_4x16 / hz4`: ops/sec [301,139.09, 293,095.18, 324,112.43, 291,258.90, 225,071.09] peak rss kb [13196, 13204, 13220, 13220, 13204] final rss kb [13196, 13204, 13220, 13220, 13204] steady rss kb [13196, 13204, 13220, 13220, 13204] private kb [21272, 21288, 21308, 21300, 21284] peak va kb [4322228, 4322228, 4322228, 4322228, 4322228] current va kb [4322228, 4322228, 4322228, 4322228, 4322228] drop [0, 0, 0, 0, 0] slots [0, 0, 0, 0, 0]

Raw log archive: `private/raw-results/windows/memcached/external_client_allocator_matrix/20260512_222837_memcached_external_client_allocator_matrix.log`
