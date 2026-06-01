# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 10:13:45 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `redis`
- HZ6 profiles: `rss`
- capacity lanes: `source512-route4k, front1k-desc4k-source512-route4k, appcap`
- diagnostic probes: `False`

## redis / redis_workload

- Note: paper-aligned Redis-like workload
- Args: `4 500 2000 16 256`
- Runs: `1`

| allocator | pattern | median ops | median peak_kb | runs |
| --- | --- | ---: | ---: | --- |
| hz6-rss-source512-route4k | SET | failed | n/a | `failed:rc124` |
| hz6-rss-source512-route4k | GET | failed | n/a | `failed:rc124` |
| hz6-rss-source512-route4k | LPUSH | failed | n/a | `failed:rc124` |
| hz6-rss-source512-route4k | LPOP | failed | n/a | `failed:rc124` |
| hz6-rss-source512-route4k | RANDOM | failed | n/a | `failed:rc124` |
| hz6-rss-front1k-desc4k-source512-route4k | SET | failed | n/a | `failed:rc124` |
| hz6-rss-front1k-desc4k-source512-route4k | GET | failed | n/a | `failed:rc124` |
| hz6-rss-front1k-desc4k-source512-route4k | LPUSH | failed | n/a | `failed:rc124` |
| hz6-rss-front1k-desc4k-source512-route4k | LPOP | failed | n/a | `failed:rc124` |
| hz6-rss-front1k-desc4k-source512-route4k | RANDOM | failed | n/a | `failed:rc124` |
| hz6-rss-appcap | SET | 12.06 | 597,428 | `[BENCH_ARGS] threads=4 cycles=500 ops=2000 min=16 max=256 Pattern: SET Throughput: 12.06 M ops/sec Ops: 4000000 --- Pattern: GET Throughput: 14.44 M ops/sec Ops: 4000000 --- Pattern: LPUSH Throughput: 6.40 M ops/sec Ops: 4000000 --- Pattern: LPOP Throughput: 15.98 M ops/sec Ops: 4000000 --- Pattern: RANDOM Throughput: 7.63 M ops/sec Ops: 4000000 ---` |
| hz6-rss-appcap | GET | 14.44 | 597,428 | `[BENCH_ARGS] threads=4 cycles=500 ops=2000 min=16 max=256 Pattern: SET Throughput: 12.06 M ops/sec Ops: 4000000 --- Pattern: GET Throughput: 14.44 M ops/sec Ops: 4000000 --- Pattern: LPUSH Throughput: 6.40 M ops/sec Ops: 4000000 --- Pattern: LPOP Throughput: 15.98 M ops/sec Ops: 4000000 --- Pattern: RANDOM Throughput: 7.63 M ops/sec Ops: 4000000 ---` |
| hz6-rss-appcap | LPUSH | 6.40 | 597,428 | `[BENCH_ARGS] threads=4 cycles=500 ops=2000 min=16 max=256 Pattern: SET Throughput: 12.06 M ops/sec Ops: 4000000 --- Pattern: GET Throughput: 14.44 M ops/sec Ops: 4000000 --- Pattern: LPUSH Throughput: 6.40 M ops/sec Ops: 4000000 --- Pattern: LPOP Throughput: 15.98 M ops/sec Ops: 4000000 --- Pattern: RANDOM Throughput: 7.63 M ops/sec Ops: 4000000 ---` |
| hz6-rss-appcap | LPOP | 15.98 | 597,428 | `[BENCH_ARGS] threads=4 cycles=500 ops=2000 min=16 max=256 Pattern: SET Throughput: 12.06 M ops/sec Ops: 4000000 --- Pattern: GET Throughput: 14.44 M ops/sec Ops: 4000000 --- Pattern: LPUSH Throughput: 6.40 M ops/sec Ops: 4000000 --- Pattern: LPOP Throughput: 15.98 M ops/sec Ops: 4000000 --- Pattern: RANDOM Throughput: 7.63 M ops/sec Ops: 4000000 ---` |
| hz6-rss-appcap | RANDOM | 7.63 | 597,428 | `[BENCH_ARGS] threads=4 cycles=500 ops=2000 min=16 max=256 Pattern: SET Throughput: 12.06 M ops/sec Ops: 4000000 --- Pattern: GET Throughput: 14.44 M ops/sec Ops: 4000000 --- Pattern: LPUSH Throughput: 6.40 M ops/sec Ops: 4000000 --- Pattern: LPOP Throughput: 15.98 M ops/sec Ops: 4000000 --- Pattern: RANDOM Throughput: 7.63 M ops/sec Ops: 4000000 ---` |
