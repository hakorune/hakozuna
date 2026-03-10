# Windows Memcached External Client

Generated: 2026-03-10 13:26:57 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_mimalloc.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11478`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `4`
- client_connections: `16`
- ratio: `1:9`
- data_size: `64`
- key_maximum: `10000`
- test_time: `3`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 302437.71 22885.10 249300.51 0.18610 0.01500 1.00700 1.02300 11036.61`

Observed progress line:
- `[RUN #1 106%, 3 secs] 0 threads: 1147514 ops, 382790 (avg: 359351) ops/sec, 15.12MB/sec (avg: 12.81MB/sec), 0.17 (avg: 0.18) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_read_heavy_4x16_mimalloc\20260310_132657_861_memcached_external_client.log`
