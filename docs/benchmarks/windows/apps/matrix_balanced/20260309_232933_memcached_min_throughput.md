# Windows Memcached Min Throughput

Generated: 2026-03-09 23:29:35 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| phase | total ops | elapsed sec | ops/sec |
| --- | ---: | ---: | ---: |
| set | 8000 | 0.225 | 35,487.70 |
| get | 8000 | 0.117 | 68,577.11 |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11421`
- server_threads: `4`
- clients: `4`
- requests_per_client: `2000`
- payload_size: `32`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\matrix\matrix_balanced\20260309_232933_memcached_min_throughput.log`
