# Windows Memcached External Client

Generated: 2026-03-10 11:31:33 +09:00

References:
- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)
- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)
- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Config:
- server exe: `C:\git\hakozuna-win\out_win_memcached\memcached_win_min_main.exe`
- client exe: `C:\git\hakozuna-win\out_win_memtier\memtier_benchmark.exe`
- listen: `127.0.0.1:11460`
- server_threads: `4`
- server_verbose: `1`
- client_threads: `1`
- client_connections: `64`
- ratio: `1:1`
- data_size: `64`
- key_maximum: `10000`
- test_time: `2`

Status: `OK`

Client exit code:
- `0`

Client totals:
- `Totals 83190.97 3317.73 38270.32 0.77017 0.50300 1.51100 2.01500 5240.69`

Observed progress line:
- `[RUN #1 108%, 2 secs] 0 threads: 179008 ops, 86062 (avg: 83191) ops/sec, 5.45MB/sec (avg: 5.12MB/sec), 0.74 (avg: 0.77) msec latency`

Runtime note:
- `connection dropped.` count: `0`
- `server slot count:` `64`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client_matrix\external_client_balanced_1x64\20260310_113133_766_memcached_external_client.log`
