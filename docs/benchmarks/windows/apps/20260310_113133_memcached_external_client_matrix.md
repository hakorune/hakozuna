# Windows Memcached External Client Matrix

Generated: 2026-03-10 11:31:33 +09:00

References:
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [win/run_win_memcached_external_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_matrix.ps1)
- [docs/WINDOWS_BUILD.md](/C:/git/hakozuna-win/docs/WINDOWS_BUILD.md)

| profile | ops/sec | drop count | slot count | threads | conns/thread | ratio | size | secs |
| --- | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| balanced_1x64 | 83190.97 | 0 | 64 | 1 | 64 | `1:1` | 64 | 2 |
| balanced_2x32 | 168935.77 | 0 | 64 | 2 | 32 | `1:1` | 64 | 2 |
| balanced_4x16 | 373581.74 | 0 | 64 | 4 | 16 | `1:1` | 64 | 2 |
| read_heavy_4x16 | 322095.91 | 0 | 64 | 4 | 16 | `1:9` | 64 | 3 |
| write_heavy_4x16 | 349574.60 | 0 | 64 | 4 | 16 | `4:1` | 64 | 3 |
| larger_payload_4x16 | 282035.35 | 0 | 64 | 4 | 16 | `1:1` | 256 | 3 |
| long_balanced_4x16 | 245364.44 | 0 | 64 | 4 | 16 | `1:1` | 64 | 10 |
| scale_8x16 | 287084.53 | 0 | 128 | 8 | 16 | `1:1` | 64 | 5 |

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_matrix\20260310_113133_memcached_external_client_matrix.log`
