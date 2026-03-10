# Windows Memcached External Client

Generated: 2026-03-10 02:20:36 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11483`
- server_threads: `4`
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
- `Totals 176068.00 21680.00 66346.50 0.19223 0.01500 1.00700 3.00700 12392.76`

Observed progress line:
- `[RUN #1 101%, 1 secs] 0 threads: 352136 ops, 0 (avg: 348752) ops/sec, 0.00KB/sec (avg: 23.97MB/sec), 0.00 (avg: 0.18) msec latency`

Runtime note:
- `connection dropped.` count: `214`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_022036_781_memcached_external_client.log`
