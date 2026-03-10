# Windows Memcached Min Matrix

Generated: 2026-03-09 23:29:23 +09:00

References:
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [win/run_win_memcached_min_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_matrix.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| profile | set ops/sec | get ops/sec | clients | req/client | payload |
| --- | ---: | ---: | ---: | ---: | ---: |
| smoke | 19,223.06 | 35,764.68 | 2 | 1000 | 16 |
| balanced | 35,487.70 | 68,577.11 | 4 | 2000 | 32 |
| higher_clients | 72,245.49 | 118,556.46 | 8 | 2000 | 32 |
| larger_payload | 40,990.61 | 66,048.38 | 4 | 1500 | 256 |

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\matrix\20260309_232923_memcached_min_matrix.log`
