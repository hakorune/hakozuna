# Windows Allocator Matrix

Generated: 2026-07-14 16:46:44 +09:00

Artifacts: [out_win_suite](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_suite)
Allocators: hz8-medium-boundary-diag

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

## medium_slice_4_8k_range

- Note: fixed medium range: 4097..8192 bytes
- Args: threads=4 iters=120000 ws=2048 size=4097..8192

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8-medium-boundary-diag | 1244218.273 | 41164 | `[BENCH_ARGS] threads=4 iters=120000 ws=2048 min=4097 max=8192 trace=lcg threads=4 iters=120000 ws=2048 size=4097..8192 time=0.386 ops/s=1244218.273 alloc_attempts=242432 alloc_success=242432 alloc_fail=0 frees=242432[H8_MEDIUM_BOUNDARY] malloc=[246161,29,0,0] active=[215673,28,0,0] owner=[29464,0,0,0] global=[0,0,0,0] create=[1024,1,0,0] local_free=[246161,29,0,0] active_miss=[30484,0,0,0] [H8_MEDIUM_BOUNDARY_SCAN] owner_scan=30489 owner_steps=3864556 global_scan=1025 global_steps=0 free_lookup=246190 route_lookup=0 route_steps=0 owner_hit=29464 owner_pos=[122,116,116,116,28994] active_switch=[65897,1,0,0] miss_create=1025 miss_detached=0` |

## large_slice_64k

- Note: fixed-size large slice: 64 KiB
- Args: threads=4 iters=50000 ws=128 size=65536..65536

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz8-medium-boundary-diag | 1097791.244 | 9528 | `[BENCH_ARGS] threads=4 iters=50000 ws=128 min=65536 max=65536 trace=lcg threads=4 iters=50000 ws=128 size=65536..65536 time=0.182 ops/s=1097791.244 alloc_attempts=100160 alloc_success=100160 alloc_fail=0 frees=100160[H8_MEDIUM_BOUNDARY] malloc=[0,0,0,100160] active=[0,0,0,51644] owner=[0,0,0,48264] global=[0,0,0,0] create=[0,0,0,252] local_free=[0,0,0,100160] active_miss=[0,0,0,48512] [H8_MEDIUM_BOUNDARY_SCAN] owner_scan=48516 owner_steps=1526936 global_scan=252 global_steps=0 free_lookup=101728 route_lookup=0 route_steps=0 owner_hit=48264 owner_pos=[776,780,780,780,45148] active_switch=[0,0,0,97960] miss_create=252 miss_detached=0` |

