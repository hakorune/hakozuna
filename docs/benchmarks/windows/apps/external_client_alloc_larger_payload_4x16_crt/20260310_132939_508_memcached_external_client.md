# Windows Memcached External Client

Generated: 2026-03-10 13:29:39 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_crt.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11482`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `4`
- client_connections: `16`
- ratio: `1:1`
- data_size: `256`
- key_maximum: `8000`
- test_time: `3`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 297885.23 62258.41 86679.37 0.18882 0.01500 1.00700 1.01500 63036.25`

Observed progress line:
- `[RUN #1 107%, 3 secs] 0 threads: 1138299 ops, 374165 (avg: 353844) ops/sec, 88.02MB/sec (avg: 73.12MB/sec), 0.17 (avg: 0.18) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_larger_payload_4x16_crt\20260310_132939_508_memcached_external_client.log`
