# Windows Memcached External Client

Generated: 2026-03-10 03:03:53 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11487`
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
- `Totals 213255.00 26287.50 80336.50 0.18274 0.01500 1.00700 1.01500 15012.99`

Observed progress line:
- `[RUN #1 113%, 1 secs] 0 threads: 417407 ops, 338956 (avg: 368583) ops/sec, 22.03MB/sec (avg: 25.31MB/sec), 0.16 (avg: 0.17) msec latency`

Runtime note:
- `connection dropped.` count: `1267`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_030353_498_memcached_external_client.log`
