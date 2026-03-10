# Windows Memcached External Client

Generated: 2026-03-10 11:16:03 +09:00

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
- `Totals 77659.00 11719.14 27109.02 0.83324 0.01500 1.00700 233.47100 5657.68`

Observed progress line:
- `[RUN #1 112%, 2 secs] 0 threads: 173460 ops, 779512 (avg: 77659) ops/sec, 56.52MB/sec (avg: 5.53MB/sec), 0.08 (avg: 0.82) msec latency`

Runtime note:
- `connection dropped.` count: `287`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_111603_976_memcached_external_client.log`
