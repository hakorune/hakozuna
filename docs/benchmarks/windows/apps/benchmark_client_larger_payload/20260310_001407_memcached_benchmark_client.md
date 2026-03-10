# Windows Memcached Benchmark Client

Generated: 2026-03-10 00:14:09 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| profile | total ops | get ops | set ops | elapsed sec | ops/sec |
| --- | ---: | ---: | ---: | ---: | ---: |
| larger_payload | 64000 | 51344 | 12656 | 0.601 | 106,576.98 |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11453`
- server_threads: `4`
- clients: `8`
- warmup_keys_per_client: `800`
- requests_per_client: `8000`
- payload_size: `256`
- get_percent: `80`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\benchmark_client_matrix\benchmark_client_larger_payload\20260310_001407_memcached_benchmark_client.log`
