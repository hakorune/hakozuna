# Windows Memcached External Client

Generated: 2026-03-10 13:21:10 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_hz3.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11461`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `1`
- client_connections: `64`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `2`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 110008.21 5043.73 49953.37 0.67700 0.50300 1.51100 2.00700 6987.79`

Observed progress line:
- `[RUN #1 112%, 2 secs] 0 threads: 212089 ops, 105725 (avg: 94664) ops/sec, 6.73MB/sec (avg: 5.87MB/sec), 0.60 (avg: 0.68) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_balanced_1x64_hz3\20260310_132110_380_memcached_external_client.log`
