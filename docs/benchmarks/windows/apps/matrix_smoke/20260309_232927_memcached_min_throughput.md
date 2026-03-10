# Windows Memcached Min Throughput

Generated: 2026-03-09 23:29:29 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| phase | total ops | elapsed sec | ops/sec |
| --- | ---: | ---: | ---: |
| set | 2000 | 0.104 | 19,223.06 |
| get | 2000 | 0.056 | 35,764.68 |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11420`
- server_threads: `4`
- clients: `2`
- requests_per_client: `1000`
- payload_size: `16`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\matrix\matrix_smoke\20260309_232927_memcached_min_throughput.log`
