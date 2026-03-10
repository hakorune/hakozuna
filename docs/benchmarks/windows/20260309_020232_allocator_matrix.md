# Windows Allocator Matrix

Generated: 2026-03-09 02:02:32 +09:00

Artifacts: [out_win_suite](/C:/git/hakozuna-win/out_win_suite)

## smoke

- Note: quick sanity and regression check
- Args: threads=2 iters=10000 ws=256 size=16..128

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 20995171.111 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=20995171.111` |
| hz3 | 30759766.226 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=30759766.226` |
| hz4 | 29416090.602 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=29416090.602` |
| mimalloc | 40346984.063 | `threads=2 iters=10000 ws=256 size=16..128 time=0.000 ops/s=40346984.063` |
| tcmalloc | 27681660.900 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=27681660.900` |

## balanced

- Note: larger mixed run for first Windows compare
- Args: threads=8 iters=250000 ws=4096 size=16..2048

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 29796800.718 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.067 ops/s=29796800.718` |
| hz3 | 550903481.710 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.004 ops/s=550903481.710` |
| hz4 | 188107823.404 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.011 ops/s=188107823.404` |
| mimalloc | 268987128.966 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.007 ops/s=268987128.966` |
| tcmalloc | 170153393.284 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.012 ops/s=170153393.284` |

## wide_ws

- Note: wider working-set pressure
- Args: threads=8 iters=200000 ws=16384 size=16..1024

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 12741298.291 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.126 ops/s=12741298.291` |
| hz3 | 298992768.112 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.005 ops/s=298992768.112` |
| hz4 | 167852122.280 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.010 ops/s=167852122.280` |
| mimalloc | 160145732.617 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.010 ops/s=160145732.617` |
| tcmalloc | 90432777.360 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.018 ops/s=90432777.360` |

## larger_sizes

- Note: shift toward larger allocations
- Args: threads=4 iters=150000 ws=4096 size=256..8192

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 10733855.834 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.056 ops/s=10733855.834` |
| hz3 | 106149600.170 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.006 ops/s=106149600.170` |
| hz4 | 53265153.936 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.011 ops/s=53265153.936` |
| mimalloc | 52359676.068 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.011 ops/s=52359676.068` |
| tcmalloc | 66005148.402 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.009 ops/s=66005148.402` |

## heavy_mixed

- Note: heavier mixed run with longer timings
- Args: threads=8 iters=5000000 ws=16384 size=16..4096

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 52840305.105 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.757 ops/s=52840305.105` |
| hz3 | 242894427.152 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.165 ops/s=242894427.152` |
| hz4 | 121587716.723 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.329 ops/s=121587716.723` |
| mimalloc | 228213850.070 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.175 ops/s=228213850.070` |
| tcmalloc | 154872062.125 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.258 ops/s=154872062.125` |

