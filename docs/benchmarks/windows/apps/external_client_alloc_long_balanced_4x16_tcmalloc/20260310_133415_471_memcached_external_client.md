# Windows Memcached External Client

Generated: 2026-03-10 13:34:15 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_tcmalloc.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11489`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `4`
- client_connections: `16`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `10`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 285559.86 81254.53 61523.37 0.26165 0.01500 1.00700 1.04700 24192.76`

Observed progress line:
- `[RUN #1 101%, 10 secs] 0 threads: 2534502 ops, 227312 (avg: 250408) ops/sec, 21.59MB/sec (avg: 20.72MB/sec), 0.27 (avg: 0.25) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_long_balanced_4x16_tcmalloc\20260310_133415_471_memcached_external_client.log`
