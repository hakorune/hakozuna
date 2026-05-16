# Windows Memcached Benchmark Client Matrix

Generated: 2026-04-18 15:40:41 +09:00

References:
- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)
- [win/run_win_memcached_benchmark_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client_matrix.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| profile | total ops | get ops | set ops | ops/sec | peak rss kb | final rss kb | steady rss kb | private kb | clients | req/client | payload | get % |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| read_heavy | 80000 | 72125 | 7875 | 105,788.77 | 11948 | 11948 | 11948 | 6312 | 8 | 10000 | 64 | 90 |
| balanced | 80000 | 40103 | 39897 | 85,230.57 | 11900 | 11900 | 11900 | 6284 | 8 | 10000 | 64 | 50 |
| write_heavy | 80000 | 16155 | 63845 | 72,537.23 | 11944 | 11944 | 11944 | 6308 | 8 | 10000 | 64 | 20 |
| larger_payload | 64000 | 51344 | 12656 | 101,408.31 | 12948 | 12948 | 12948 | 7308 | 8 | 8000 | 256 | 80 |

Raw log archive: `private/raw-results/windows/memcached/benchmark_client_matrix/20260418_154041_memcached_benchmark_client_matrix.log`
