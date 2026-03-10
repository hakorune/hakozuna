# Windows Memcached External Client

Generated: 2026-03-10 13:23:55 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_tcmalloc.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11469`
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
- `Totals 191370.39 14467.00 81211.94 0.33962 0.50300 1.01500 1.17500 12662.56`

Observed progress line:
- `[RUN #1 107%, 2 secs] 0 threads: 409979 ops, 201733 (avg: 191700) ops/sec, 13.57MB/sec (avg: 12.38MB/sec), 0.32 (avg: 0.33) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_balanced_2x32_tcmalloc\20260310_132355_153_memcached_external_client.log`
