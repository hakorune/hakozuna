# Windows Memcached External Client

Generated: 2026-03-10 12:25:42 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_hz3.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11491`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `8`
- client_connections: `16`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `20000`
- test_time: `5`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 290106.51 22567.20 122481.09 0.38138 0.50300 1.00700 1.51100 19424.24`

Observed progress line:
- `[RUN #1 102%, 5 secs] 0 threads: 1724143 ops, 333985 (avg: 338853) ops/sec, 22.65MB/sec (avg: 22.16MB/sec), 0.38 (avg: 0.38) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `128`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_scale_8x16_hz3\20260310_122542_635_memcached_external_client.log`
