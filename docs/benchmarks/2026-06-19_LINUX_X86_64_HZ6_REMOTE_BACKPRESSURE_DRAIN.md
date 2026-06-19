# Linux x86_64 HZ6 Remote Backpressure Drain, 2026-06-19

This records the opt-in `RemoteFreeBackpressureDrain-L1` A/B after the selected
Ubuntu preload lane moved transfer capacity to 256.

## Conditions

| Item | Value |
|---|---|
| Platform | Ubuntu/Linux x86_64 |
| Allocator | HZ6 selected preload plus opt-in drain flag |
| Runner | `hakozuna-hz6/linux/run_hz6_preload_remote_median.sh` |
| Repeat rule | RUNS=10 median |
| Extra flag | `HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_L1=1` |
| Bench | `bench_random_mixed_mt_remote_malloc` |
| Ring slots | 65536 |

## Results

| Row | Shape | Prior selected median ops/s | Drain median ops/s |
|---|---:|---:|---:|
| `remote50` | `16 10000 100 16 32768 50 65536` | 14363938.00 | 13321713.54 |
| `remote90` | `16 120000 100 16 131072 90 65536` | 6610576.93 | 6969804.00 |

## Safety Gate

`hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh` passed with the drain
flag enabled.

| Counter | Value |
|---|---:|
| `transfer_reserve_full_after_state_mutation` | 0 |
| `remote_free_backpressure_requeue_fail` | 0 |
| `route_rehome_fail` | 0 |
| `route_rehome_directory_transfer_fail` | 0 |
| `free_resolve_result_unresolved_integrity` | 0 |
| `free_route_real_free_unproven` | 0 |

The drain relieved final backpressure but increased committed route movement:

| Counter | Value |
|---|---:|
| `remote_free_foreign_candidate` | 37280 |
| `transfer_reserve_success` | 26512 |
| `transfer_reserve_full` | 10768 |
| `remote_free_backpressure_drain_attempt` | 24347 |
| `remote_free_backpressure_drain_success` | 24206 |
| `remote_free_backpressure_retry_success` | 24206 |
| `route_rehome_success` | 26512 |

## Decision

`GO(remote90-specialist)/HOLD(default)`: keep the implementation and counters,
but leave the flag off in the selected preload lane. The box helps remote90
slightly and proves the backpressure path, but it costs remote50 and adds
committed rehome/tombstone pressure.
