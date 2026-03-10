# Windows Memcached External Client

Generated: 2026-03-10 03:05:23 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11488`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `4`
- client_connections: `16`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `1`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 204114.00 23799.50 78254.00 0.17920 0.01500 1.00700 5.02300 14248.53`

Observed progress line:
- `[RUN #1 109%, 1 secs] 0 threads: 408228 ops, 313262 (avg: 375747) ops/sec, 20.24MB/sec (avg: 25.61MB/sec), 0.18 (avg: 0.17) msec latency`

Runtime note:
- `connection dropped.` count: `893`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_030523_045_memcached_external_client.log`
