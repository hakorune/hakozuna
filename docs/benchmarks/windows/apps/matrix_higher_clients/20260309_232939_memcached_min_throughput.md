# Windows Memcached Min Throughput

Generated: 2026-03-09 23:29:41 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| phase | total ops | elapsed sec | ops/sec |
| --- | ---: | ---: | ---: |
| set | 16000 | 0.221 | 72,245.49 |
| get | 16000 | 0.135 | 118,556.46 |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11422`
- server_threads: `4`
- clients: `8`
- requests_per_client: `2000`
- payload_size: `32`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\matrix\matrix_higher_clients\20260309_232939_memcached_min_throughput.log`
