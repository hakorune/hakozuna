# Linux x86_64 HZ6 Remote Backpressure Capacity, 2026-06-19

This records the RUNS=10 median after the Ubuntu selected preload lane raised
the transfer compile-time/profile capacity to 256 for
`RemoteFreeBackpressure-L1`.

## Conditions

| Item | Value |
|---|---|
| Platform | Ubuntu/Linux x86_64 |
| Allocator | HZ6 selected preload |
| Runner | `hakozuna-hz6/linux/run_hz6_preload_remote_median.sh` |
| Repeat rule | RUNS=10 median |
| New flags | `HZ6_TRANSFER_CACHE_CAPACITY=256`, `HZ6_PROFILE_SPEED_TRANSFER_CAPACITY=256`, `HZ6_PROFILE_REMOTE_TRANSFER_CAPACITY=256` |
| Bench | `bench_random_mixed_mt_remote_malloc` |
| Ring slots | 65536 |

## Results

| Row | Shape | Prior selected median ops/s | New median ops/s |
|---|---:|---:|---:|
| `remote50` | `16 10000 100 16 32768 50 65536` | 14351051.48 | 14363938.00 |
| `remote90` | `16 120000 100 16 131072 90 65536` | 422566.06 | 6610576.93 |

## Safety Gate

`hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh` passed before the
RUNS=10 median. The smoke kept the reserve-before-mutation gate at zero:

| Counter | Value |
|---|---:|
| `transfer_reserve_full_after_state_mutation` | 0 |
| `route_rehome_fail` | 0 |
| `route_rehome_directory_transfer_fail` | 0 |
| `free_resolve_result_unresolved_integrity` | 0 |
| `free_route_real_free_unproven` | 0 |

The same smoke still showed backpressure remaining:

| Counter | Value |
|---|---:|
| `remote_free_foreign_candidate` | 33781 |
| `transfer_reserve_success` | 2465 |
| `transfer_reserve_full` | 31316 |
| `remote_free_backpressure` | 31316 |

## Notes

- `remote90` remains high variance, but the median moved from hundreds of
  thousands to millions of ops/s.
- This is a selected Ubuntu preload result only; it is not a Windows or macOS
  promotion.
