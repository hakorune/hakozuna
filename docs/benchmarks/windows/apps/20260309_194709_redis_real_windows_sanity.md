# Windows Real Redis Sanity

Generated: 2026-03-09 19:47:09 +09:00

References:
- [win/prepare_win_redis_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_redis_private_layout.ps1)
- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)

Sanity lane note:
- source root: `C:\git\hakozuna-win\private\bench-assets\windows\redis\prebuilt`
- lane: prebuilt Windows Redis baseline
- params: tests=set,get,lpush,lpop n=1000 clients=10 pipeline=4 port=6389
- runs: 1
- statistic: median requests/sec
- note: this is an app-level baseline lane; allocator matrix still splits into real Redis and synthetic Redis-style workload
- raw log path is private-only

| test | median requests/sec | runs |
| --- | ---: | --- |
| set | 333,333.34 | `333,333.34` |
| get | 333,333.34 | `333,333.34` |
| lpush | 200,000.00 | `200,000.00` |
| lpop | 333,333.34 | `333,333.34` |

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_sanity\20260309_194709_redis_real_windows_sanity.log`
