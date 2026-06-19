# Linux x86_64 HZ6 Remote Route Baseline, 2026-06-19

This records the first RUNS=10 median baseline after the HZ6
`RemoteFreeRouteResolve-L1` shared-first lane, integrity smoke gate, and remote
median runner landed.

## Conditions

| Item | Value |
|---|---|
| Platform | Ubuntu/Linux x86_64 |
| Allocator | HZ6 selected preload |
| Runner | `hakozuna-hz6/linux/run_hz6_preload_remote_median.sh` |
| Repeat rule | RUNS=10 median |
| Preload | `LD_PRELOAD=hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so` |
| Bench | `bench_random_mixed_mt_remote_malloc` |
| Ring slots | 65536 |

## Results

| Row | Shape | Median ops/s |
|---|---:|---:|
| `remote50` | `16 10000 100 16 32768 50 65536` | 763130.83 |
| `remote90` | `16 120000 100 16 131072 90 65536` | 148892.40 |

## Safety Gate

`hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh` passed after the median
run. The checked zero counters include:

| Counter | Required |
|---|---:|
| `free_route_real_free_unproven` | 0 |
| `free_resolve_result_retry` | 0 |
| `free_resolve_result_unresolved_integrity` | 0 |
| `route_compact_remote_path_attempt` | 0 |
| `ring_full_fallback` | 0 |
| `overflow_sent` / `overflow_received` | 0 |

## Notes

- `remote90` showed high run-to-run variance, so use this median baseline
  instead of single-run smoke numbers for future A/B decisions.
- This is a Ubuntu/Linux selected-lane baseline, not a Windows or macOS
  promotion result.
