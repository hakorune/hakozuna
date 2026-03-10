# Windows Memcached External Client

Generated: 2026-03-10 12:24:38 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_crt.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11490`
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
- `Totals 280321.74 20969.91 119185.47 0.42254 0.50300 1.01500 1.51100 18694.57`

Observed progress line:
- `[RUN #1 107%, 5 secs] 0 threads: 1632704 ops, 328260 (avg: 305409) ops/sec, 22.01MB/sec (avg: 19.89MB/sec), 0.39 (avg: 0.42) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `128`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_scale_8x16_crt\20260310_122438_561_memcached_external_client.log`
