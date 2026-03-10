# Windows Real Redis Source Matrix

Generated: 2026-03-09 19:50:22 +09:00

References:
- [win/prepare_win_redis_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_redis_private_layout.ps1)
- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)
- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)

Source root used: `.\private\bench-assets\windows\redis\source`
Params:
- allocators: `Jemalloc`
- tests: `set,get,lpush,lpop` n=1000 clients=10 pipeline=4 runs=1
- statistic: median requests/sec
- raw logs: private only

| allocator | test | median requests/sec | runs |
| --- | --- | ---: | --- |
| jemalloc | set | 499,999.97 | `499,999.97` |
| jemalloc | get | 499,999.97 | `499,999.97` |
| jemalloc | lpush | 249,999.98 | `249,999.98` |
| jemalloc | lpop | 333,333.34 | `333,333.34` |

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
Raw log: `C:\git\hakozuna-win\private\raw-results\windows\redis\real_matrix\20260309_195022_redis_real_windows_matrix.log`
