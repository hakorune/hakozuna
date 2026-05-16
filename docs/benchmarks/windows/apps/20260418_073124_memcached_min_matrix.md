# Windows Memcached Min Matrix

Generated: 2026-04-18 07:31:24 +09:00

References:
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [win/run_win_memcached_min_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_matrix.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| profile | set ops/sec | get ops/sec | clients | req/client | payload |
| --- | ---: | ---: | ---: | ---: | ---: |
| smoke | 26,717.75 | 49,186.94 | 2 | 1000 | 16 |
| balanced | 46,321.09 | 78,726.67 | 4 | 2000 | 32 |
| higher_clients | 64,865.57 | 121,371.01 | 8 | 2000 | 32 |
| larger_payload | 45,145.50 | 76,074.07 | 4 | 1500 | 256 |

Raw log archive: `private/raw-results/windows/memcached/matrix/20260418_073124_memcached_min_matrix.log`
