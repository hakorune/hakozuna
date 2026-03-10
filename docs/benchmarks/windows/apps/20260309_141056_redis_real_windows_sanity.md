# Windows Real Redis Sanity

Generated: 2026-03-09 14:10:56 +09:00

References:
- [private/hakmem/hakozuna/bench/windows/README.md](/C:/git/hakozuna-win/private/hakmem/hakozuna/bench/windows/README.md)
- [private/hakmem/hakozuna/bench/windows/redis/redis-server.exe](/C:/git/hakozuna-win/private/hakmem/hakozuna/bench/windows/redis/redis-server.exe)
- [private/hakmem/hakozuna/bench/windows/redis/redis-benchmark.exe](/C:/git/hakozuna-win/private/hakmem/hakozuna/bench/windows/redis/redis-benchmark.exe)

Sanity lane note:
- lane: prebuilt Windows Redis baseline
- params: tests=set,get,lpush,lpop n=100000 clients=50 pipeline=16 port=6389
- runs: 3
- statistic: median requests/sec
- note: this is an app-level baseline lane; allocator matrix still splits into real Redis and synthetic Redis-style workload

| test | median requests/sec | runs |
| --- | ---: | --- |
| set | 1,176,470.62 | `1,149,425.38, 1,149,425.38, 1,176,470.62` |
| get | 1,333,333.25 | `1,219,512.12, 1,265,822.75, 1,333,333.25` |
| lpush | 1,000,000.00 | `980,392.19, 1,000,000.00, 990,099.00` |
| lpop | 1,052,631.62 | `1,030,927.81, 1,052,631.62, 1,041,666.69` |

Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)
