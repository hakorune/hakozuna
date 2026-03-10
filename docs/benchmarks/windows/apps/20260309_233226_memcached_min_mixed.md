# Windows Memcached Min Mixed

Generated: 2026-03-09 23:32:27 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_min_mixed.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_mixed.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

| total ops | set ops | get ops | elapsed sec | ops/sec |
| ---: | ---: | ---: | ---: | ---: |
| 16000 | 8000 | 8000 | 0.343 | 46,629.65 |

Config:
- exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- listen: `127.0.0.1:11430`
- server_threads: `4`
- clients: `4`
- requests_per_client: `4000`
- payload_size: `32`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\mixed\20260309_233226_memcached_min_mixed.log`
