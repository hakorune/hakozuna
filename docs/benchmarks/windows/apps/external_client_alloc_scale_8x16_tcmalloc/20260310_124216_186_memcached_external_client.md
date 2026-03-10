# Windows Memcached External Client

Generated: 2026-03-10 12:42:16 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_tcmalloc.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11494`
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
- `Totals 250856.23 17540.76 107882.25 0.43553 0.50300 1.02300 1.51900 16619.75`

Observed progress line:
- `[RUN #1 101%, 5 secs] 0 threads: 1498865 ops, 296748 (avg: 295844) ops/sec, 19.77MB/sec (avg: 19.14MB/sec), 0.43 (avg: 0.43) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `128`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_scale_8x16_tcmalloc\20260310_124216_186_memcached_external_client.log`
