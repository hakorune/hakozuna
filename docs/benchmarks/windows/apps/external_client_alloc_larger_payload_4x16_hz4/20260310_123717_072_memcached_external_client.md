# Windows Memcached External Client

Generated: 2026-03-10 12:37:17 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_hz4.exe`
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
- `Totals 338190.40 64973.65 104117.24 0.19880 0.01500 1.00700 1.01500 69982.82`

Observed progress line:
- `[RUN #1 102%, 3 secs] 0 threads: 1018850 ops, 268883 (avg: 334549) ops/sec, 62.15MB/sec (avg: 67.61MB/sec), 0.24 (avg: 0.19) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_larger_payload_4x16_hz4\20260310_123717_072_memcached_external_client.log`
