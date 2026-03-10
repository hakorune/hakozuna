# Windows Memcached External Client

Generated: 2026-03-10 13:22:51 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_hz3.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11466`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `2`
- client_connections: `32`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `2`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 227584.17 17907.25 95874.90 0.31185 0.50300 1.00700 1.15900 15121.60`

Observed progress line:
- `[RUN #1 104%, 2 secs] 0 threads: 435158 ops, 216572 (avg: 208827) ops/sec, 14.64MB/sec (avg: 13.55MB/sec), 0.29 (avg: 0.31) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_balanced_2x32_hz3\20260310_132251_752_memcached_external_client.log`
