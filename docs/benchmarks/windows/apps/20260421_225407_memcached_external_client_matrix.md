# Windows Memcached External Client Matrix

Generated: 2026-04-21 22:54:07 +09:00

References:
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_matrix.ps1)
- [docs/WINDOWS_BUILD.md](/C:/git/hakozuna-win/docs/WINDOWS_BUILD.md)

| profile | ops/sec | peak rss kb | final rss kb | steady rss kb | private kb | drop count | slot count | threads | conns/thread | ratio | size | secs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| balanced_1x64 | 94385.41 | 10968 | 10968 | 10968 | 5284 | 0 | 0 | 1 | 64 | `1:1` | 64 | 2 |
| balanced_2x32 | 183778.84 | 10964 | 10964 | 10964 | 5272 | 0 | 0 | 2 | 32 | `1:1` | 64 | 2 |
| balanced_4x16 | 183921.81 | 10956 | 10956 | 10956 | 5264 | 0 | 0 | 4 | 16 | `1:1` | 64 | 2 |
| read_heavy_4x16 | 293177.53 | 10952 | 10952 | 10952 | 5280 | 0 | 0 | 4 | 16 | `1:9` | 64 | 3 |
| write_heavy_4x16 | 295349.38 | 10968 | 10968 | 10968 | 5272 | 0 | 0 | 4 | 16 | `4:1` | 64 | 3 |
| larger_payload_4x16 | 299464.35 | 11988 | 11988 | 11988 | 6304 | 0 | 0 | 4 | 16 | `1:1` | 256 | 3 |
| long_balanced_4x16 | 287720.12 | 11980 | 11980 | 11980 | 6308 | 0 | 0 | 4 | 16 | `1:1` | 64 | 10 |
| scale_8x16 | 1546.27 | 11988 | 11988 | 11988 | 6308 | 60 | 0 | 8 | 16 | `1:1` | 64 | 5 |

Raw log archive: `private/raw-results/windows/memcached/external_client_matrix/20260421_225407_memcached_external_client_matrix.log`
