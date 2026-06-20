# Linux x86_64 HZ6 Remote Free Overflow, 2026-06-19

This records the opt-in `RemoteFreeOverflow-L1` experiment.  The box adds a
small overflow transfer cache that is reserved only after the normal transfer
cache is full and still before descriptor state/owner mutation.  Reuse checks
the overflow cache only after normal transfer reuse misses.

## Conditions

| Item | Value |
|---|---|
| Platform | Ubuntu/Linux x86_64 |
| Allocator | HZ6 selected preload plus opt-in overflow flag |
| Runner | `hakozuna-hz6/linux/run_hz6_preload_remote_median.sh` |
| Repeat rule | Quick RUNS=3 median |
| Extra flag | `HZ6_REMOTE_FREE_OVERFLOW_L1=1` |
| Bench | `bench_random_mixed_mt_remote_malloc` |
| Ring slots | 65536 |

## Results

| Overflow capacity | `remote50` median ops/s | `remote90` median ops/s |
|---:|---:|---:|
| 64 | 13789256.22 | 6044836.89 |
| 256 | 14399762.89 | 2440102.22 |

## Safety Gate

`hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh` passed with
`HZ6_REMOTE_FREE_OVERFLOW_L1=1` at capacity 64 and 256.

Capacity 256 smoke:

| Counter | Value |
|---|---:|
| `remote_free_overflow_reserve_success` | 3038 |
| `remote_free_overflow_reserve_full` | 32251 |
| `remote_free_overflow_pop` | 2682 |
| `route_rehome_success` | 6668 |
| `route_rehome_fail` | 0 |
| `transfer_reserve_full_after_state_mutation` | 0 |

## Decision

`NO-GO(default)`: overflow reduces some uncommitted remote frees, but it
commits more route rehome work and hurts `remote90`. Keep it as an opt-in
correctness/research box; do not select it for the Ubuntu preload performance
lane.
