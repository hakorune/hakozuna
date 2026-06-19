# Linux x86_64 HZ6 Preload Foreign Resolved Dispatch, 2026-06-19

This records the first RUNS=10 A/B after enabling
`PreloadForeignResolvedDispatch-L1` in the HZ6 selected preload lane.

## Conditions

| Item | Value |
|---|---|
| Platform | Ubuntu/Linux x86_64 |
| Allocator | HZ6 selected preload |
| Runner | `hakozuna-hz6/linux/run_hz6_preload_remote_median.sh` |
| Repeat rule | RUNS=10 median |
| New flag | `HZ6_PRELOAD_FOREIGN_RESOLVED_DISPATCH_L1=1` |
| Bench | `bench_random_mixed_mt_remote_malloc` |
| Ring slots | 65536 |

## Results

| Row | Shape | Baseline median ops/s | New median ops/s | Ratio |
|---|---:|---:|---:|---:|
| `remote50` | `16 10000 100 16 32768 50 65536` | 763130.83 | 14351051.48 | 18.81x |
| `remote90` | `16 120000 100 16 131072 90 65536` | 148892.40 | 422566.06 | 2.84x |

## Safety Gate

`hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh` passed before the
RUNS=10 median. The smoke also showed the new path being consumed:

| Counter | Value |
|---|---:|
| `free_resolved_foreign_direct_dispatch` | 39715 |
| `free_resolved_duplicate_route_lookup_avoided` | 39715 |
| `free_route_real_free_unproven` | 0 |
| `free_resolve_result_retry` | 0 |
| `free_resolve_result_unresolved_integrity` | 0 |
| `route_compact_remote_path_attempt` | 0 |

## Decision

`PreloadForeignResolvedDispatch-L1` is `GO/default` for the Ubuntu selected
preload lane. It only consumes `FOREIGN_VALID` resolver routes after preload
active maps have already missed; local/mixed rows stay on the existing selected
path.
