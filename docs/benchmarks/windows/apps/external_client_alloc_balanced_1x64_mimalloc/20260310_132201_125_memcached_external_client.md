# Windows Memcached External Client

Generated: 2026-03-10 13:22:01 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_mimalloc.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11463`
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
- `Totals 109535.48 5131.82 49626.93 0.59437 0.50300 1.51100 1.55900 6967.81`

Observed progress line:
- `[RUN #1 102%, 2 secs] 0 threads: 219292 ops, 109791 (avg: 107852) ops/sec, 6.99MB/sec (avg: 6.70MB/sec), 0.58 (avg: 0.59) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_balanced_1x64_mimalloc\20260310_132201_125_memcached_external_client.log`
