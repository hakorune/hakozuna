# Windows Memcached External Client

Generated: 2026-03-10 11:15:25 +09:00

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
- `Totals 66291.19 24326.68 8819.89 0.66569 0.01500 1.00700 44.28700 6100.73`

Observed progress line:
- `[RUN #1 104%, 2 secs] 0 threads: 293432 ops, 366433 (avg: 140405) ops/sec, 32.98MB/sec (avg: 12.68MB/sec), 0.15 (avg: 0.45) msec latency`

Runtime note:
- `connection dropped.` count: `3319`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_111525_664_memcached_external_client.log`
