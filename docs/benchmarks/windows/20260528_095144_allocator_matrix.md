# Windows Allocator Matrix

Generated: 2026-05-28 09:51:44 +09:00

Artifacts: [out_win_suite](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_suite)

## smoke

- Note: quick sanity and regression check
- Args: threads=2 iters=10000 ws=256 size=16..128

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 23100023.100 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=23100023.100` |
| hz3 | 24319066.148 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=24319066.148` |
| hz4 | 28133352.089 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=28133352.089` |
| hz6-strict | 19275250.578 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=19275250.578` |
| hz6-speed | 17346053.773 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=17346053.773` |
| hz6-rss | 18071744.827 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=18071744.827` |
| mimalloc | 42408821.035 | `threads=2 iters=10000 ws=256 size=16..128 time=0.000 ops/s=42408821.035` |
| tcmalloc | 19575217.774 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=19575217.774` |

## balanced

- Note: larger mixed run for first Windows compare
- Args: threads=8 iters=250000 ws=4096 size=16..2048

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 27552421.927 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.073 ops/s=27552421.927` |
| hz3 | 397266804.386 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.005 ops/s=397266804.386` |
| hz4 | 158114016.017 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.013 ops/s=158114016.017` |
| hz6-strict | 180266253.256 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.011 ops/s=180266253.256` |
| hz6-speed | 121624908.781 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.016 ops/s=121624908.781` |
| hz6-rss | 163469476.162 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.012 ops/s=163469476.162` |
| mimalloc | 222719629.395 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.009 ops/s=222719629.395` |
| tcmalloc | 151523186.836 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.013 ops/s=151523186.836` |

## wide_ws

- Note: wider working-set pressure
- Args: threads=8 iters=200000 ws=16384 size=16..1024

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 9110885.743 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.176 ops/s=9110885.743` |
| hz3 | 297768596.579 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.005 ops/s=297768596.579` |
| hz4 | 112572204.516 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.014 ops/s=112572204.516` |
| hz6-strict | 177042069.622 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.009 ops/s=177042069.622` |
| hz6-speed | 151510847.230 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.011 ops/s=151510847.230` |
| hz6-rss | 138267166.733 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.012 ops/s=138267166.733` |
| mimalloc | 146441025.453 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.011 ops/s=146441025.453` |
| tcmalloc | 98784335.274 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.016 ops/s=98784335.274` |

## larger_sizes

- Note: shift toward larger allocations
- Args: threads=4 iters=150000 ws=4096 size=256..8192

| allocator | ops/s | raw |
| --- | ---: | --- |
| crt | 9252976.374 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.065 ops/s=9252976.374` |
| hz3 | 10658727.099 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.056 ops/s=10658727.099` |
| hz4 | 49669696.518 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.012 ops/s=49669696.518` |
| hz6-strict | 628110.587 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.955 ops/s=628110.587` |
| hz6-speed | 629173.320 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.954 ops/s=629173.320` |
| hz6-rss | 638892.994 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.939 ops/s=638892.994` |
| mimalloc | 48314624.837 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.012 ops/s=48314624.837` |
| tcmalloc | 57441003.303 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.010 ops/s=57441003.303` |

