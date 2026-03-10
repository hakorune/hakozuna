# Windows Memcached Benchmark Client

Generated: 2026-03-10 00:14:07 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| profile | total ops | get ops | set ops | elapsed sec | ops/sec |
| --- | ---: | ---: | ---: | ---: | ---: |
| write_heavy | 80000 | 16155 | 63845 | 1.049 | 76,257.84 |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11452`
- server_threads: `4`
- clients: `8`
- warmup_keys_per_client: `1000`
- requests_per_client: `10000`
- payload_size: `64`
- get_percent: `20`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\benchmark_client_matrix\benchmark_client_write_heavy\20260310_001404_memcached_benchmark_client.log`
