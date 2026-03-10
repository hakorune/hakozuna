# Windows Allocator Matrix

Generated: 2026-03-09 02:00:56 +09:00

Artifacts: [out_win_suite](/C:/git/hakozuna-win/out_win_suite)

## smoke

- Note: quick sanity and regression check
- Args: threads=2 iters=10000 ws=256 size=16..128

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 20124773.596 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=20124773.596` |
| hz3 | 34193879.296 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=34193879.296` |
| hz4 | 27754648.904 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=27754648.904` |
| mimalloc | 41937513.105 | `threads=2 iters=10000 ws=256 size=16..128 time=0.000 ops/s=41937513.105` |
| tcmalloc | 26395671.110 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=26395671.110` |

## balanced

- Note: larger mixed run for first Windows compare
- Args: threads=8 iters=250000 ws=4096 size=16..2048

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 31183783.185 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.064 ops/s=31183783.185` |
| hz3 | 487009034.018 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.004 ops/s=487009034.018` |
| hz4 | 176984885.491 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.011 ops/s=176984885.491` |
| mimalloc | 279415462.852 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.007 ops/s=279415462.852` |
| tcmalloc | 162895632.768 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.012 ops/s=162895632.768` |

## wide_ws

- Note: wider working-set pressure
- Args: threads=8 iters=200000 ws=16384 size=16..1024

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 11737295.844 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.136 ops/s=11737295.844` |
| hz3 | 313166702.550 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.005 ops/s=313166702.550` |
| hz4 | 142980974.594 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.011 ops/s=142980974.594` |
| mimalloc | 146739180.278 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.011 ops/s=146739180.278` |
| tcmalloc | 95109018.713 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.017 ops/s=95109018.713` |

## larger_sizes

- Note: shift toward larger allocations
- Args: threads=4 iters=150000 ws=4096 size=256..8192

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 8393651.042 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.071 ops/s=8393651.042` |
| hz3 | 88317117.329 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.007 ops/s=88317117.329` |
| hz4 | 47545842.116 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.013 ops/s=47545842.116` |
| mimalloc | 42948254.511 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.014 ops/s=42948254.511` |
| tcmalloc | 60877241.043 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.010 ops/s=60877241.043` |

