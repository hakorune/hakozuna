# Windows Memcached External Client

Generated: 2026-03-10 04:11:56 +09:00

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
- `Totals 346246.13 157482.40 15640.11 0.21482 0.01500 1.00700 31.61500 34566.99`

Observed progress line:
- `[RUN #1 100%, 10 secs] 0 threads: 3118562 ops, 148744 (avg: 311572) ops/sec, 15.07MB/sec (avg: 30.38MB/sec), 0.48 (avg: 0.20) msec latency`

Runtime note:
- `connection dropped.` count: `10404`

Raw client stdout and stderr are kept in the private raw-results lane.

Raw log: `C:\git\hakozuna-win\private\raw-results\windows\memcached\external_client\20260310_041156_022_memcached_external_client.log`
