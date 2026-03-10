# Windows Memcached External Client

Generated: 2026-03-10 00:13:22 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- client exe: `C:\git\hakozuna-win\private\bench-assets\windows\memcached\client\memtier_benchmark.exe`
- listen: `127.0.0.1:11460`
- server_threads: `4`
- client_threads: `4`
- client_connections: `16`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `10`

Status: `BLOCKED`

Reason:
- external client executable was not found in the canonical private path
- the memtier source snapshot exists, but the native Windows client build box is not finished yet

Expected private path:
- `C:\git\hakozuna-win\private\bench-assets\windows\memcached\client\memtier_benchmark.exe`

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_001322_memcached_external_client.log`
