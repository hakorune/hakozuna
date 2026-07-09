# Windows Allocator Matrix

Generated: 2026-07-09 17:03:32 +09:00

Artifacts: [out_win_suite](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_suite)
Allocators: hz11-span, hz11-span-transfer

Notes:
- hz5-policy uses the HZ5 Windows policy/API path in this mixed malloc/free runner. It is not the exact 64K/a8192 Local2P microbench lane; use the HZ5 synthetic/Local2P family for that profile.
- hz11-span is the selected Windows HZ11 bring-up row: HZ11_CLASSIFY_SPAN=1 with a VirtualAlloc span arena. It is a Windows matrix connectivity row, not Linux fine128 parity and not a default allocator claim.
- hz11-span-transfer adds HZ11_TRANSFER_CENTRAL_SPAN=1 to the Windows span row as an opt-in L1 pressure lane. It is not the selected Windows row unless a follow-up gate promotes it.
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
| hz11-span | 22479487.468 | NA | `[BENCH_ARGS] threads=2 iters=10000 ws=256 min=16 max=128 threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=22479487.468 alloc_attempts=10240 alloc_success=10240 alloc_fail=0 frees=10240` |
| hz11-span-transfer | 24052916.416 | NA | `[BENCH_ARGS] threads=2 iters=10000 ws=256 min=16 max=128 threads=2 iters=10000 ws=256 size=16..128 time=0.001 ops/s=24052916.416 alloc_attempts=10240 alloc_success=10240 alloc_fail=0 frees=10240` |

## balanced

- Note: larger mixed run for first Windows compare
- Args: threads=8 iters=250000 ws=4096 size=16..2048

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz11-span | 13944755.064 | 38180 | `[BENCH_ARGS] threads=8 iters=250000 ws=4096 min=16 max=2048 threads=8 iters=250000 ws=4096 size=16..2048 time=0.143 ops/s=13944755.064 alloc_attempts=1015808 alloc_success=1015808 alloc_fail=0 frees=1015808` |
| hz11-span-transfer | 13670222.291 | 1282312 | `[BENCH_ARGS] threads=8 iters=250000 ws=4096 min=16 max=2048 threads=8 iters=250000 ws=4096 size=16..2048 time=0.146 ops/s=13670222.291 alloc_attempts=1015808 alloc_success=1015808 alloc_fail=0 frees=1015808` |

