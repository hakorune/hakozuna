# Windows Memcached External Client

Generated: 2026-03-10 12:33:58 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_hz4.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11472`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `4`
- client_connections: `16`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `3`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 262898.28 41162.69 90282.12 0.19955 0.01500 1.00700 1.01500 19286.46`

Observed progress line:
- `[RUN #1 103%, 3 secs] 0 threads: 1031636 ops, 270373 (avg: 333235) ops/sec, 21.44MB/sec (avg: 23.87MB/sec), 0.23 (avg: 0.19) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_balanced_4x16_hz4\20260310_123358_495_memcached_external_client.log`
