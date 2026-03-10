# Windows Memcached External Client

Generated: 2026-03-10 04:00:08 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11494`
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
- `Totals 284640.56 25024.67 117283.62 0.25491 0.01500 1.00700 6.52700 19144.79`

Observed progress line:
- `[RUN #1 110%, 1 secs] 0 threads: 284940 ops, 223259 (avg: 258872) ops/sec, 14.09MB/sec (avg: 17.00MB/sec), 0.25 (avg: 0.25) msec latency`

Runtime note:
- `connection dropped.` count: `2517`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_040008_448_memcached_external_client.log`
