# Linux x86_64 HZ6 Remote Free Consumer Rehome, 2026-06-19

This records the opt-in `RemoteFreeConsumerRehome-L1` experiment.  The box
skips free-side route rehome for transfer-based fronts and attempts to rehome
the route when the destination allocator later pops the transfer object.

## Conditions

| Item | Value |
|---|---|
| Platform | Ubuntu/Linux x86_64 |
| Allocator | HZ6 selected preload plus opt-in consumer rehome flag |
| Runner | `hakozuna-hz6/linux/run_hz6_preload_remote_median.sh` |
| Repeat rule | Quick RUNS=3 median |
| Extra flag | `HZ6_REMOTE_FREE_CONSUMER_REHOME_L1=1` |
| Bench | `bench_random_mixed_mt_remote_malloc` |
| Ring slots | 65536 |

## Results

| Row | Median ops/s |
|---|---:|
| `remote50` | 12638884.18 |
| `remote90` | 269255.48 |

## Safety Gate

`hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh` passed with
`HZ6_REMOTE_FREE_CONSUMER_REHOME_L1=1`.

| Counter | Value |
|---|---:|
| `transfer_reuse_rehome_attempt` | 2183 |
| `transfer_reuse_rehome_success` | 2183 |
| `transfer_reuse_rehome_fail` | 330 |
| `route_rehome_success` | 2183 |
| `route_rehome_fail` | 0 |
| `route_tombstone_current` | 868 |
| `route_compact_remote_path_attempt` | 0 |

## Decision

`NO-GO(default)`: moving rehome from free to transfer reuse reduces some route
tombstone pressure, but it puts shared-directory lookup and route transfer on
the malloc reuse path.  That collapses `remote90`, so the selected lane keeps
free-side rehome.
