# Windows Allocator Matrix

Generated: 2026-07-09 17:25:49 +09:00

Artifacts: [out_win_suite](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_suite)
Allocators: hz11-span-diag, hz11-span-cache256-diag

Notes:
- hz5-policy uses the HZ5 Windows policy/API path in this mixed malloc/free runner. It is not the exact 64K/a8192 Local2P microbench lane; use the HZ5 synthetic/Local2P family for that profile.
- hz11-span is the selected Windows HZ11 bring-up row: HZ11_CLASSIFY_SPAN=1 with a VirtualAlloc span arena. It is a Windows matrix connectivity row, not Linux fine128 parity and not a default allocator claim.
- hz11-span-transfer adds HZ11_TRANSFER_CENTRAL_SPAN=1 to the Windows span row as an opt-in L1 pressure lane. It is not the selected Windows row unless a follow-up gate promotes it.
- hz11-span-diag is a diagnostic-only sibling of hz11-span with HZ11_ENABLE_HOT_COUNTERS=1, HZ11_SPAN_RETURNED_DIAG=1, and HZ11 summary printing. Do not use it for speed ranking.
- hz11-span-tlsfast, hz11-span-cache256, and hz11-span-tlsfast-cache256 are Windows A/B rows for the matrix balanced pressure signal. They are not selected rows unless separately promoted. hz11-span-cache256-diag is the diagnostic sibling of the cache256 row.
- hz6-*-broad keeps the same HZ6 policy profile but raises descriptor/route/source/front-cache capacities for broad working-set matrix profiles.
- hz6-*-route4k keeps the non-route capacities at control values while widening only the route table to 4096.
- hz6-*-largerlowrss uses the selected 4K..16K/LargerSizes low-RSS lane: front8k + SourceRunReuse + desc8k + route8k.
- hz6-*-largedirectretain16m-largerlowrss and hz6-*-largedirectretain32m-largerlowrss add >1MiB LargeDirectRetain controls to the LargerLowRSS lane for large_slices follow-up rows.
- hz6-*-sameownerfast-largerlowrss adds SameOwnerFast-L1 to the LargerLowRSS lane for same-owner small/mid fixed-size checks.
- hz6-*-directlocalfreereuse-largerlowrss decomposes the SameOwnerFast win into direct local free + alloc + reuse on the LargerLowRSS lane.
- hz6-*-directlocalfreereuse-small8k-largerlowrss limits DirectLocalFreeReuse to class <= 4 so 16K/32K MidPage rows fall back to the LargerLowRSS path.
- The unqualified hz6-* rows keep the small default R1 capacities as controls.

## balanced

- Note: larger mixed run for first Windows compare
- Args: threads=8 iters=250000 ws=4096 size=16..2048

| allocator | ops/s | peak RSS KB | raw |
| --- | ---: | ---: | --- |
| hz11-span-diag | 10502712.325 | 39768 | `[BENCH_ARGS] threads=8 iters=250000 ws=4096 min=16 max=2048 threads=8 iters=250000 ws=4096 size=16..2048 time=0.190 ops/s=10502712.325 alloc_attempts=1015808 alloc_success=1015808 alloc_fail=0 frees=1015808 hz11_malloc=1015808 hz11_hit=28669 hz11_refill=987139 hz11_free=1015808 hz11_direct_hit=1015808 hz11_direct_miss=0 hz11_overflow=30818 hz11_flush=30818 hz11_flush_items=986176 hz11_cached_bytes=492640 hz11_span_create=590 hz11_returned_push=986176 hz11_returned_pop_hit=960795 hz11_returned_pop_miss=26344 hz11_refill_transfer=0 hz11_refill_central=0 hz11_refill_span=0 hz11_xfer_hit=0 hz11_xfer_miss=0 hz11_xfer_insert=0 hz11_xfer_spill=0 hz11_central_hit=0 hz11_central_miss=0 hz11_central_insert=0` |
| hz11-span-cache256-diag | 15775533.095 | 40532 | `[BENCH_ARGS] threads=8 iters=250000 ws=4096 min=16 max=2048 threads=8 iters=250000 ws=4096 size=16..2048 time=0.127 ops/s=15775533.095 alloc_attempts=1015808 alloc_success=1015808 alloc_fail=0 frees=1015808 hz11_malloc=1015808 hz11_hit=161584 hz11_refill=854224 hz11_free=1015808 hz11_direct_hit=1015808 hz11_direct_miss=0 hz11_overflow=3314 hz11_flush=3314 hz11_flush_items=848384 hz11_cached_bytes=3352096 hz11_span_create=594 hz11_returned_push=848384 hz11_returned_pop_hit=826543 hz11_returned_pop_miss=27681 hz11_refill_transfer=0 hz11_refill_central=0 hz11_refill_span=0 hz11_xfer_hit=0 hz11_xfer_miss=0 hz11_xfer_insert=0 hz11_xfer_spill=0 hz11_central_hit=0 hz11_central_miss=0 hz11_central_insert=0` |

