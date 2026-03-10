# Windows Memcached Benchmark Client Matrix

Generated: 2026-03-10 00:13:55 +09:00

References:
- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)
- [win/run_win_memcached_benchmark_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client_matrix.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| profile | total ops | get ops | set ops | ops/sec | clients | req/client | payload | get % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| read_heavy | 80000 | 72125 | 7875 | 115,345.91 | 8 | 10000 | 64 | 90 |
| balanced | 80000 | 40103 | 39897 | 90,963.18 | 8 | 10000 | 64 | 50 |
| write_heavy | 80000 | 16155 | 63845 | 76,257.84 | 8 | 10000 | 64 | 20 |
| larger_payload | 64000 | 51344 | 12656 | 106,576.98 | 8 | 8000 | 256 | 80 |

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\benchmark_client_matrix\20260310_001355_memcached_benchmark_client_matrix.log`
