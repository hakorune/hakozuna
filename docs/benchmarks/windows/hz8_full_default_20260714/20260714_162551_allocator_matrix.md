# Windows Allocator Matrix

Generated: 2026-07-14 16:25:51 +09:00

Artifacts: [out_win_suite](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_suite)
Allocators: hz8, hz3, hz4, mimalloc, tcmalloc

Notes:
- hz5-policy uses the HZ5 Windows policy/API path in this mixed malloc/free runner. It is not the exact 64K/a8192 Local2P microbench lane; use the HZ5 synthetic/Local2P family for that profile.
- hz11-span-cache256 is the selected Windows HZ11 bring-up row: HZ11_CLASSIFY_SPAN=1 plus HZ11_CACHE_CAP=256 with a VirtualAlloc span arena. hz11-span remains the L1 control row. These are Windows matrix connectivity rows, not Linux fine128 parity and not default allocator claims.
- hz11-span-transfer adds HZ11_TRANSFER_CENTRAL_SPAN=1 to the Windows span row as an opt-in L1 pressure lane. It is not the selected Windows row unless a follow-up gate promotes it.
- hz11-span-transfer-fine128-win ports the complete Linux fine128 transfer policy stack with Windows FLS thread-exit salvage. It is a broad-MT opt-in row, not the selected/default Windows row.
- hz11-span-diag is a diagnostic-only sibling of hz11-span with HZ11_ENABLE_HOT_COUNTERS=1, HZ11_SPAN_RETURNED_DIAG=1, and HZ11 summary printing. Do not use it for speed ranking.
- hz11-span-tlsfast and hz11-span-tlsfast-cache256 are Windows A/B rows for the matrix balanced pressure signal. hz11-span-cache256-diag is the diagnostic sibling of the selected cache256 row.
- hz11-span-cache512 is an experimental Windows cap-pressure row used to test whether matrix balanced/wide_ws weakness is caused by returned-object/refill pressure. hz11-span-cache512-diag is its diagnostic sibling and should not be used for speed ranking.
- hz11-span-cache512-classdiag adds HZ11_CLASS_DIAG=1 to print per-class malloc/hit/refill/overflow/returned-pop attribution. It is diagnostic-only and must not be used for speed ranking.
- hz11-span-cache512-classbatch is a Windows L1 behavior probe: for class >= 4 it pops returned objects in a small batch and seeds the front cache. classbatch-diag adds class attribution and must not be used for speed ranking.
- hz11-span-cache512-classbatch16 and classbatch16-4-7 are narrower L2 probes. classbatch16 is candidate-watch / matrix helper; classbatch16-4-7 is balanced/wide_ws specialist evidence. Pressure-gated and 4-6 variants are documented no-go/evidence rows and are not kept in the default runner list.
- hz11-span-*-matrixattrib rows enable HZ11_MATRIX_ATTRIB_DIAG=1 and are diagnostic-only. They split refill into returned-one, returned-batch, current-span, span-new, and sys-fallback paths.
- hz11-span-cache512-classbatch16-coldskip is a Windows L2 returned-empty-lock probe. After a returned-pop miss it briefly skips returned-sink lock attempts when the current span has slots. It is evidence/candidate-watch, not the selected row.
- hz6-*-broad keeps the same HZ6 policy profile but raises descriptor/route/source/front-cache capacities for broad working-set matrix profiles.
- hz6-*-route4k keeps the non-route capacities at control values while widening only the route table to 4096.
- hz6-*-largerlowrss uses the selected 4K..16K/LargerSizes low-RSS lane: front8k + SourceRunReuse + desc8k + route8k.
- hz6-*-largedirectretain16m-largerlowrss and hz6-*-largedirectretain32m-largerlowrss add >1MiB LargeDirectRetain controls to the LargerLowRSS lane for large_slices follow-up rows.
- hz6-*-sameownerfast-largerlowrss adds SameOwnerFast-L1 to the LargerLowRSS lane for same-owner small/mid fixed-size checks.
- hz6-*-directlocalfreereuse-largerlowrss decomposes the SameOwnerFast win into direct local free + alloc + reuse on the LargerLowRSS lane.
- hz6-*-directlocalfreereuse-small8k-largerlowrss limits DirectLocalFreeReuse to class <= 4 so 16K/32K MidPage rows fall back to the LargerLowRSS path.
- The unqualified hz6-* rows keep the small default R1 capacities as controls.

## smoke

- Note: quick sanity and regression check
- Args: threads=2 iters=10000 ws=256 size=16..128

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 25393600.813 | NA | `[BENCH_ARGS] threads=2 iters=10000 ws=256 min=16 max=128 trace=lcg threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=25393600.813 alloc_attempts=10240 alloc_success=10240 alloc_fail=0 frees=10240` |
| hz3 | 22502250.225 | 6788 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=22502250.225 peak_kb=6788 current_kb=0 scavenge_released=0` |
| hz4 | 23253110.103 | NA | `[BENCH_ARGS] threads=2 iters=10000 ws=256 min=16 max=128 threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=23253110.103 alloc_attempts=10240 alloc_success=10240 alloc_fail=0 frees=10240` |
| mimalloc | 26791694.575 | 6340 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=26791694.575 peak_kb=6340 current_kb=0 scavenge_released=0` |
| tcmalloc | 19533157.535 | 9440 | `threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=19533157.535 peak_kb=9440 current_kb=0 scavenge_released=0` |

## balanced

- Note: larger mixed run for first Windows compare
- Args: threads=8 iters=250000 ws=4096 size=16..2048

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 266602682.023 | 53780 | `[BENCH_ARGS] threads=8 iters=250000 ws=4096 min=16 max=2048 trace=lcg threads=8 iters=250000 ws=4096 size=16..2048 time=0.008 ops/s=266602682.023 alloc_attempts=1015808 alloc_success=1015808 alloc_fail=0 frees=1015808` |
| hz3 | 452437507.069 | 65124 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.004 ops/s=452437507.069 peak_kb=65124 current_kb=0 scavenge_released=0` |
| hz4 | 129787537.801 | 90328 | `[BENCH_ARGS] threads=8 iters=250000 ws=4096 min=16 max=2048 threads=8 iters=250000 ws=4096 size=16..2048 time=0.015 ops/s=129787537.801 alloc_attempts=1015808 alloc_success=1015808 alloc_fail=0 frees=1015808` |
| mimalloc | 278287972.394 | 48552 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.007 ops/s=278287972.394 peak_kb=48552 current_kb=0 scavenge_released=0` |
| tcmalloc | 186624613.920 | 43360 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.011 ops/s=186624613.920 peak_kb=43360 current_kb=0 scavenge_released=0` |

## wide_ws

- Note: wider working-set pressure
- Args: threads=8 iters=200000 ws=16384 size=16..1024

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 139685532.944 | 95648 | `[BENCH_ARGS] threads=8 iters=200000 ws=16384 min=16 max=1024 trace=lcg threads=8 iters=200000 ws=16384 size=16..1024 time=0.011 ops/s=139685532.944 alloc_attempts=813568 alloc_success=813568 alloc_fail=0 frees=813568` |
| hz3 | 318782251.798 | 90044 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.005 ops/s=318782251.798 peak_kb=90044 current_kb=0 scavenge_released=0` |
| hz4 | 112668121.963 | 97032 | `[BENCH_ARGS] threads=8 iters=200000 ws=16384 min=16 max=1024 threads=8 iters=200000 ws=16384 size=16..1024 time=0.014 ops/s=112668121.963 alloc_attempts=813568 alloc_success=813568 alloc_fail=0 frees=813568` |
| mimalloc | 155092861.851 | 85308 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.010 ops/s=155092861.851 peak_kb=85308 current_kb=0 scavenge_released=0` |
| tcmalloc | 85920802.500 | 69380 | `threads=8 iters=200000 ws=16384 size=16..1024 time=0.019 ops/s=85920802.500 peak_kb=69380 current_kb=0 scavenge_released=0` |

## larger_sizes

- Note: shift toward larger allocations
- Args: threads=4 iters=150000 ws=4096 size=256..8192

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 30014857.354 | 77516 | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=256 max=8192 trace=lcg threads=4 iters=150000 ws=4096 size=256..8192 time=0.020 ops/s=30014857.354 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| hz3 | 10420882.058 | 69616 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.058 ops/s=10420882.058 peak_kb=69616 current_kb=0 scavenge_released=0` |
| hz4 | 39235945.357 | 82468 | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=256 max=8192 threads=4 iters=150000 ws=4096 size=256..8192 time=0.015 ops/s=39235945.357 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| mimalloc | 42549871.996 | 129696 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.014 ops/s=42549871.996 peak_kb=129696 current_kb=0 scavenge_released=0` |
| tcmalloc | 60770570.838 | 58400 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.010 ops/s=60770570.838 peak_kb=58400 current_kb=0 scavenge_released=0` |

## large_slice_256

- Note: fixed-size large slice: 256 bytes
- Args: threads=4 iters=150000 ws=4096 size=256..256

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 252727349.311 | NA | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=256 max=256 trace=lcg threads=4 iters=150000 ws=4096 size=256..256 time=0.002 ops/s=252727349.311 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| hz3 | 303966766.300 | 12152 | `threads=4 iters=150000 ws=4096 size=256..256 time=0.002 ops/s=303966766.300 peak_kb=12152 current_kb=0 scavenge_released=0` |
| hz4 | 241672372.820 | NA | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=256 max=256 threads=4 iters=150000 ws=4096 size=256..256 time=0.002 ops/s=241672372.820 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| mimalloc | 333481547.354 | 10696 | `threads=4 iters=150000 ws=4096 size=256..256 time=0.002 ops/s=333481547.354 peak_kb=10696 current_kb=0 scavenge_released=0` |
| tcmalloc | 264795445.518 | 13776 | `threads=4 iters=150000 ws=4096 size=256..256 time=0.002 ops/s=264795445.518 peak_kb=13776 current_kb=0 scavenge_released=0` |

## large_slice_512

- Note: fixed-size large slice: 512 bytes
- Args: threads=4 iters=150000 ws=4096 size=512..512

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 235534270.236 | NA | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=512 max=512 trace=lcg threads=4 iters=150000 ws=4096 size=512..512 time=0.003 ops/s=235534270.236 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| hz3 | 313414124.530 | 17044 | `threads=4 iters=150000 ws=4096 size=512..512 time=0.002 ops/s=313414124.530 peak_kb=17044 current_kb=0 scavenge_released=0` |
| hz4 | 211610354.800 | NA | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=512 max=512 threads=4 iters=150000 ws=4096 size=512..512 time=0.003 ops/s=211610354.800 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| mimalloc | 346020761.246 | 14916 | `threads=4 iters=150000 ws=4096 size=512..512 time=0.002 ops/s=346020761.246 peak_kb=14916 current_kb=0 scavenge_released=0` |
| tcmalloc | 191338733.338 | 17964 | `threads=4 iters=150000 ws=4096 size=512..512 time=0.003 ops/s=191338733.338 peak_kb=17964 current_kb=0 scavenge_released=0` |

## large_slice_1k

- Note: fixed-size large slice: 1 KiB
- Args: threads=4 iters=150000 ws=4096 size=1024..1024

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 225436783.769 | NA | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=1024 max=1024 trace=lcg threads=4 iters=150000 ws=4096 size=1024..1024 time=0.003 ops/s=225436783.769 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| hz3 | 263458329.674 | 29588 | `threads=4 iters=150000 ws=4096 size=1024..1024 time=0.002 ops/s=263458329.674 peak_kb=29588 current_kb=0 scavenge_released=0` |
| hz4 | 174484543.578 | 22356 | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=1024 max=1024 threads=4 iters=150000 ws=4096 size=1024..1024 time=0.003 ops/s=174484543.578 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| mimalloc | 270599377.621 | 23176 | `threads=4 iters=150000 ws=4096 size=1024..1024 time=0.002 ops/s=270599377.621 peak_kb=23176 current_kb=0 scavenge_released=0` |
| tcmalloc | 124378109.453 | 26004 | `threads=4 iters=150000 ws=4096 size=1024..1024 time=0.005 ops/s=124378109.453 peak_kb=26004 current_kb=0 scavenge_released=0` |

## large_slice_2k

- Note: fixed-size large slice: 2 KiB
- Args: threads=4 iters=150000 ws=4096 size=2048..2048

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 179635340.259 | 38580 | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=2048 max=2048 trace=lcg threads=4 iters=150000 ws=4096 size=2048..2048 time=0.003 ops/s=179635340.259 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| hz3 | 138504155.125 | 73648 | `threads=4 iters=150000 ws=4096 size=2048..2048 time=0.004 ops/s=138504155.125 peak_kb=73648 current_kb=0 scavenge_released=0` |
| hz4 | 136323359.008 | 39556 | `[BENCH_ARGS] threads=4 iters=150000 ws=4096 min=2048 max=2048 threads=4 iters=150000 ws=4096 size=2048..2048 time=0.004 ops/s=136323359.008 alloc_attempts=305088 alloc_success=305088 alloc_fail=0 frees=305088` |
| mimalloc | 218316777.644 | 39816 | `threads=4 iters=150000 ws=4096 size=2048..2048 time=0.003 ops/s=218316777.644 peak_kb=39816 current_kb=0 scavenge_released=0` |
| tcmalloc | 72087658.593 | 38644 | `threads=4 iters=150000 ws=4096 size=2048..2048 time=0.008 ops/s=72087658.593 peak_kb=38644 current_kb=0 scavenge_released=0` |

## large_slice_4k

- Note: fixed-size large slice: 4 KiB
- Args: threads=4 iters=120000 ws=2048 size=4096..4096

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 147551566.198 | NA | `[BENCH_ARGS] threads=4 iters=120000 ws=2048 min=4096 max=4096 trace=lcg threads=4 iters=120000 ws=2048 size=4096..4096 time=0.003 ops/s=147551566.198 alloc_attempts=242432 alloc_success=242432 alloc_fail=0 frees=242432` |
| hz3 | 7924924.548 | 40380 | `threads=4 iters=120000 ws=2048 size=4096..4096 time=0.061 ops/s=7924924.548 peak_kb=40380 current_kb=0 scavenge_released=0` |
| hz4 | 25362069.968 | 33300 | `[BENCH_ARGS] threads=4 iters=120000 ws=2048 min=4096 max=4096 threads=4 iters=120000 ws=2048 size=4096..4096 time=0.019 ops/s=25362069.968 alloc_attempts=242432 alloc_success=242432 alloc_fail=0 frees=242432` |
| mimalloc | 149635264.044 | 39524 | `threads=4 iters=120000 ws=2048 size=4096..4096 time=0.003 ops/s=149635264.044 peak_kb=39524 current_kb=0 scavenge_released=0` |
| tcmalloc | 54796397.137 | 41280 | `threads=4 iters=120000 ws=2048 size=4096..4096 time=0.009 ops/s=54796397.137 peak_kb=41280 current_kb=0 scavenge_released=0` |

## medium_slice_4_8k_range

- Note: fixed medium range: 4097..8192 bytes
- Args: threads=4 iters=120000 ws=2048 size=4097..8192

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 21398289.028 | 40884 | `[BENCH_ARGS] threads=4 iters=120000 ws=2048 min=4097 max=8192 trace=lcg threads=4 iters=120000 ws=2048 size=4097..8192 time=0.022 ops/s=21398289.028 alloc_attempts=242432 alloc_success=242432 alloc_fail=0 frees=242432` |
| hz3 | 8427232.602 | 40508 | `threads=4 iters=120000 ws=2048 size=4097..8192 time=0.057 ops/s=8427232.602 peak_kb=40508 current_kb=0 scavenge_released=0` |
| hz4 | 57781897.413 | 36340 | `[BENCH_ARGS] threads=4 iters=120000 ws=2048 min=4097 max=8192 threads=4 iters=120000 ws=2048 size=4097..8192 time=0.008 ops/s=57781897.413 alloc_attempts=242432 alloc_success=242432 alloc_fail=0 frees=242432` |
| mimalloc | 81668765.100 | 51420 | `threads=4 iters=120000 ws=2048 size=4097..8192 time=0.006 ops/s=81668765.100 peak_kb=51420 current_kb=0 scavenge_released=0` |
| tcmalloc | 65667067.965 | 43560 | `threads=4 iters=120000 ws=2048 size=4097..8192 time=0.007 ops/s=65667067.965 peak_kb=43560 current_kb=0 scavenge_released=0` |

## large_slice_8k

- Note: fixed-size large slice: 8 KiB
- Args: threads=4 iters=100000 ws=1024 size=8192..8192

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 34371643.394 | 22812 | `[BENCH_ARGS] threads=4 iters=100000 ws=1024 min=8192 max=8192 trace=lcg threads=4 iters=100000 ws=1024 size=8192..8192 time=0.012 ops/s=34371643.394 alloc_attempts=200704 alloc_success=200704 alloc_fail=0 frees=200704` |
| hz3 | 14285051.051 | 24588 | `threads=4 iters=100000 ws=1024 size=8192..8192 time=0.028 ops/s=14285051.051 peak_kb=24588 current_kb=0 scavenge_released=0` |
| hz4 | 34077645.916 | 18956 | `[BENCH_ARGS] threads=4 iters=100000 ws=1024 min=8192 max=8192 threads=4 iters=100000 ws=1024 size=8192..8192 time=0.012 ops/s=34077645.916 alloc_attempts=200704 alloc_success=200704 alloc_fail=0 frees=200704` |
| mimalloc | 88481872.276 | 23544 | `threads=4 iters=100000 ws=1024 size=8192..8192 time=0.005 ops/s=88481872.276 peak_kb=23544 current_kb=0 scavenge_released=0` |
| tcmalloc | 39090749.174 | 32112 | `threads=4 iters=100000 ws=1024 size=8192..8192 time=0.010 ops/s=39090749.174 peak_kb=32112 current_kb=0 scavenge_released=0` |

## large_slice_16k

- Note: fixed-size large slice: 16 KiB
- Args: threads=4 iters=80000 ws=512 size=16384..16384

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 30409579.017 | 14984 | `[BENCH_ARGS] threads=4 iters=80000 ws=512 min=16384 max=16384 trace=lcg threads=4 iters=80000 ws=512 size=16384..16384 time=0.011 ops/s=30409579.017 alloc_attempts=160256 alloc_success=160256 alloc_fail=0 frees=160256` |
| hz3 | 22449681.144 | 16752 | `threads=4 iters=80000 ws=512 size=16384..16384 time=0.014 ops/s=22449681.144 peak_kb=16752 current_kb=0 scavenge_released=0` |
| hz4 | 35821029.183 | 13352 | `[BENCH_ARGS] threads=4 iters=80000 ws=512 min=16384 max=16384 threads=4 iters=80000 ws=512 size=16384..16384 time=0.009 ops/s=35821029.183 alloc_attempts=160256 alloc_success=160256 alloc_fail=0 frees=160256` |
| mimalloc | 148996601.015 | 15980 | `threads=4 iters=80000 ws=512 size=16384..16384 time=0.002 ops/s=148996601.015 peak_kb=15980 current_kb=0 scavenge_released=0` |
| tcmalloc | 43487123.734 | 30244 | `threads=4 iters=80000 ws=512 size=16384..16384 time=0.007 ops/s=43487123.734 peak_kb=30244 current_kb=0 scavenge_released=0` |

## large_slice_32k

- Note: fixed-size large slice: 32 KiB
- Args: threads=4 iters=60000 ws=256 size=32768..32768

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 26608128.783 | 11068 | `[BENCH_ARGS] threads=4 iters=60000 ws=256 min=32768 max=32768 trace=lcg threads=4 iters=60000 ws=256 size=32768..32768 time=0.009 ops/s=26608128.783 alloc_attempts=120192 alloc_success=120192 alloc_fail=0 frees=120192` |
| hz3 | 144291468.767 | 12904 | `threads=4 iters=60000 ws=256 size=32768..32768 time=0.002 ops/s=144291468.767 peak_kb=12904 current_kb=0 scavenge_released=0` |
| hz4 | 29768180.296 | NA | `[BENCH_ARGS] threads=4 iters=60000 ws=256 min=32768 max=32768 threads=4 iters=60000 ws=256 size=32768..32768 time=0.008 ops/s=29768180.296 alloc_attempts=120192 alloc_success=120192 alloc_fail=0 frees=120192` |
| mimalloc | 109464082.098 | 12024 | `threads=4 iters=60000 ws=256 size=32768..32768 time=0.002 ops/s=109464082.098 peak_kb=12024 current_kb=0 scavenge_released=0` |
| tcmalloc | 45521793.559 | 29576 | `threads=4 iters=60000 ws=256 size=32768..32768 time=0.005 ops/s=45521793.559 peak_kb=29576 current_kb=0 scavenge_released=0` |

## large_slice_64k

- Note: fixed-size large slice: 64 KiB
- Args: threads=4 iters=50000 ws=128 size=65536..65536

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 9596744.784 | 9428 | `[BENCH_ARGS] threads=4 iters=50000 ws=128 min=65536 max=65536 trace=lcg threads=4 iters=50000 ws=128 size=65536..65536 time=0.021 ops/s=9596744.784 alloc_attempts=100160 alloc_success=100160 alloc_fail=0 frees=100160` |
| hz3 | 50492299.924 | 10984 | `threads=4 iters=50000 ws=128 size=65536..65536 time=0.004 ops/s=50492299.924 peak_kb=10984 current_kb=0 scavenge_released=0` |
| hz4 | 96814793.300 | NA | `[BENCH_ARGS] threads=4 iters=50000 ws=128 min=65536 max=65536 threads=4 iters=50000 ws=128 size=65536..65536 time=0.002 ops/s=96814793.300 alloc_attempts=100160 alloc_success=100160 alloc_fail=0 frees=100160` |
| mimalloc | 90555102.780 | 10932 | `threads=4 iters=50000 ws=128 size=65536..65536 time=0.002 ops/s=90555102.780 peak_kb=10932 current_kb=0 scavenge_released=0` |
| tcmalloc | 53843047.516 | 28072 | `threads=4 iters=50000 ws=128 size=65536..65536 time=0.004 ops/s=53843047.516 peak_kb=28072 current_kb=0 scavenge_released=0` |

## large_slice_128k

- Note: fixed-size large slice: 128 KiB
- Args: threads=4 iters=40000 ws=64 size=131072..131072

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 1281496.788 | 20928 | `[BENCH_ARGS] threads=4 iters=40000 ws=64 min=131072 max=131072 trace=lcg threads=4 iters=40000 ws=64 size=131072..131072 time=0.125 ops/s=1281496.788 alloc_attempts=80128 alloc_success=80128 alloc_fail=0 frees=80128` |
| hz3 | 8100896.668 | 10728 | `threads=4 iters=40000 ws=64 size=131072..131072 time=0.020 ops/s=8100896.668 peak_kb=10728 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 6500 | `[BENCH_ARGS] threads=4 iters=40000 ws=64 min=131072 max=131072` |
| mimalloc | 37329102.702 | 8444 | `threads=4 iters=40000 ws=64 size=131072..131072 time=0.004 ops/s=37329102.702 peak_kb=8444 current_kb=0 scavenge_released=0` |
| tcmalloc | 45710367.683 | 28012 | `threads=4 iters=40000 ws=64 size=131072..131072 time=0.004 ops/s=45710367.683 peak_kb=28012 current_kb=0 scavenge_released=0` |

## large_slice_256k

- Note: fixed-size large slice: 256 KiB
- Args: threads=4 iters=30000 ws=32 size=262144..262144

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 828123.631 | 29824 | `[BENCH_ARGS] threads=4 iters=30000 ws=32 min=262144 max=262144 trace=lcg threads=4 iters=30000 ws=32 size=262144..262144 time=0.145 ops/s=828123.631 alloc_attempts=60032 alloc_success=60032 alloc_fail=0 frees=60032` |
| hz3 | 7675627.962 | 12156 | `threads=4 iters=30000 ws=32 size=262144..262144 time=0.016 ops/s=7675627.962 peak_kb=12156 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 28268 | `[BENCH_ARGS] threads=4 iters=30000 ws=32 min=262144 max=262144` |
| mimalloc | 29515938.607 | 8960 | `threads=4 iters=30000 ws=32 size=262144..262144 time=0.004 ops/s=29515938.607 peak_kb=8960 current_kb=0 scavenge_released=0` |
| tcmalloc | 33158331.031 | 34168 | `threads=4 iters=30000 ws=32 size=262144..262144 time=0.004 ops/s=33158331.031 peak_kb=34168 current_kb=0 scavenge_released=0` |

## large_slice_512k

- Note: fixed-size large slice: 512 KiB
- Args: threads=4 iters=20000 ws=16 size=524288..524288

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 774110.644 | 9640 | `[BENCH_ARGS] threads=4 iters=20000 ws=16 min=524288 max=524288 trace=lcg threads=4 iters=20000 ws=16 size=524288..524288 time=0.103 ops/s=774110.644 alloc_attempts=40000 alloc_success=40000 alloc_fail=0 frees=40000` |
| hz3 | 5213186.756 | 12192 | `threads=4 iters=20000 ws=16 size=524288..524288 time=0.015 ops/s=5213186.756 peak_kb=12192 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 6112 | `[BENCH_ARGS] threads=4 iters=20000 ws=16 min=524288 max=524288` |
| mimalloc | 18263172.313 | 10972 | `threads=4 iters=20000 ws=16 size=524288..524288 time=0.004 ops/s=18263172.313 peak_kb=10972 current_kb=0 scavenge_released=0` |
| tcmalloc | 5967121.162 | 42256 | `threads=4 iters=20000 ws=16 size=524288..524288 time=0.013 ops/s=5967121.162 peak_kb=42256 current_kb=0 scavenge_released=0` |

## large_slice_1m

- Note: fixed-size large slice: 1 MiB
- Args: threads=4 iters=12000 ws=8 size=1048576..1048576

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 356770.425 | 4956 | `[BENCH_ARGS] threads=4 iters=12000 ws=8 min=1048576 max=1048576 trace=lcg threads=4 iters=12000 ws=8 size=1048576..1048576 time=0.135 ops/s=356770.425 alloc_attempts=24000 alloc_success=24000 alloc_fail=0 frees=24000` |
| hz3 | 2428228.152 | 21260 | `threads=4 iters=12000 ws=8 size=1048576..1048576 time=0.020 ops/s=2428228.152 peak_kb=21260 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 5972 | `[BENCH_ARGS] threads=4 iters=12000 ws=8 min=1048576 max=1048576` |
| mimalloc | 9691096.305 | 15048 | `threads=4 iters=12000 ws=8 size=1048576..1048576 time=0.005 ops/s=9691096.305 peak_kb=15048 current_kb=0 scavenge_released=0` |
| tcmalloc | 4875125.687 | 42460 | `threads=4 iters=12000 ws=8 size=1048576..1048576 time=0.010 ops/s=4875125.687 peak_kb=42460 current_kb=0 scavenge_released=0` |

## large_direct_slice_2m

- Note: fixed-size direct large slice: 2 MiB
- Args: threads=4 iters=8000 ws=4 size=2097152..2097152

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 281795.814 | 4868 | `[BENCH_ARGS] threads=4 iters=8000 ws=4 min=2097152 max=2097152 trace=lcg threads=4 iters=8000 ws=4 size=2097152..2097152 time=0.114 ops/s=281795.814 alloc_attempts=16000 alloc_success=16000 alloc_fail=0 frees=16000` |
| hz3 | 781576.293 | 34328 | `threads=4 iters=8000 ws=4 size=2097152..2097152 time=0.041 ops/s=781576.293 peak_kb=34328 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 5908 | `[BENCH_ARGS] threads=4 iters=8000 ws=4 min=2097152 max=2097152` |
| mimalloc | 5613936.598 | 23120 | `threads=4 iters=8000 ws=4 size=2097152..2097152 time=0.006 ops/s=5613936.598 peak_kb=23120 current_kb=0 scavenge_released=0` |
| tcmalloc | 1354663.641 | 44304 | `threads=4 iters=8000 ws=4 size=2097152..2097152 time=0.024 ops/s=1354663.641 peak_kb=44304 current_kb=0 scavenge_released=0` |

## large_direct_slice_4m

- Note: fixed-size direct large slice: 4 MiB
- Args: threads=4 iters=5000 ws=3 size=4194304..4194304

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 242034.640 | 4844 | `[BENCH_ARGS] threads=4 iters=5000 ws=3 min=4194304 max=4194304 trace=lcg threads=4 iters=5000 ws=3 size=4194304..4194304 time=0.083 ops/s=242034.640 alloc_attempts=10004 alloc_success=10004 alloc_fail=0 frees=10004` |
| hz3 | 820489.258 | 73500 | `threads=4 iters=5000 ws=3 size=4194304..4194304 time=0.024 ops/s=820489.258 peak_kb=73500 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 5912 | `[BENCH_ARGS] threads=4 iters=5000 ws=3 min=4194304 max=4194304` |
| mimalloc | 1195235.790 | 72008 | `threads=4 iters=5000 ws=3 size=4194304..4194304 time=0.017 ops/s=1195235.790 peak_kb=72008 current_kb=0 scavenge_released=0` |
| tcmalloc | 928875.967 | 87268 | `threads=4 iters=5000 ws=3 size=4194304..4194304 time=0.022 ops/s=928875.967 peak_kb=87268 current_kb=0 scavenge_released=0` |

## large_direct_slice_8m

- Note: fixed-size direct large slice: 8 MiB
- Args: threads=4 iters=3000 ws=2 size=8388608..8388608

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 253544.339 | 4844 | `[BENCH_ARGS] threads=4 iters=3000 ws=2 min=8388608 max=8388608 trace=lcg threads=4 iters=3000 ws=2 size=8388608..8388608 time=0.047 ops/s=253544.339 alloc_attempts=6000 alloc_success=6000 alloc_fail=0 frees=6000` |
| hz3 | 184674.480 | 73024 | `threads=4 iters=3000 ws=2 size=8388608..8388608 time=0.065 ops/s=184674.480 peak_kb=73024 current_kb=0 scavenge_released=0` |
| hz4 | failed:-1073741819 | 5756 | `[BENCH_ARGS] threads=4 iters=3000 ws=2 min=8388608 max=8388608` |
| mimalloc | 355161.983 | 72012 | `threads=4 iters=3000 ws=2 size=8388608..8388608 time=0.034 ops/s=355161.983 peak_kb=72012 current_kb=0 scavenge_released=0` |
| tcmalloc | 323766.920 | 107736 | `threads=4 iters=3000 ws=2 size=8388608..8388608 time=0.037 ops/s=323766.920 peak_kb=107736 current_kb=0 scavenge_released=0` |

## heavy_mixed

- Note: heavier mixed run with longer timings
- Args: threads=8 iters=5000000 ws=16384 size=16..4096

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8 | 278231256.082 | 370648 | `[BENCH_ARGS] threads=8 iters=5000000 ws=16384 min=16 max=4096 trace=lcg threads=8 iters=5000000 ws=16384 size=16..4096 time=0.144 ops/s=278231256.082 alloc_attempts=20054016 alloc_success=20054016 alloc_fail=0 frees=20054016` |
| hz3 | 183867806.402 | 385328 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.218 ops/s=183867806.402 peak_kb=385328 current_kb=0 scavenge_released=0` |
| hz4 | 119264566.974 | 332040 | `[BENCH_ARGS] threads=8 iters=5000000 ws=16384 min=16 max=4096 threads=8 iters=5000000 ws=16384 size=16..4096 time=0.335 ops/s=119264566.974 alloc_attempts=20054016 alloc_success=20054016 alloc_fail=0 frees=20054016` |
| mimalloc | 216705032.487 | 312560 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.185 ops/s=216705032.487 peak_kb=312560 current_kb=0 scavenge_released=0` |
| tcmalloc | 172924740.991 | 240560 | `threads=8 iters=5000000 ws=16384 size=16..4096 time=0.231 ops/s=172924740.991 peak_kb=240560 current_kb=0 scavenge_released=0` |

