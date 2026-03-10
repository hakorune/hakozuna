# Windows Memcached External Client

Generated: 2026-03-10 13:34:01 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached_allocators\memcached_win_min_main_mimalloc.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11488`
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
- `Totals 281398.68 80683.01 60014.70 0.25794 0.01500 1.01500 1.05500 23894.45`

Observed progress line:
- `[RUN #1 102%, 10 secs] 0 threads: 2591150 ops, 221515 (avg: 254166) ops/sec, 21.09MB/sec (avg: 21.08MB/sec), 0.27 (avg: 0.25) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_allocator_matrix\external_client_alloc_long_balanced_4x16_mimalloc\20260310_133401_039_memcached_external_client.log`
