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

## Follow-up Gates

Two additional controls were added after the first pass:

| Control | Default | Purpose |
|---|---:|---|
| `HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_STRIDE` | 1 | Drain at most one out of N reserve failures |
| `HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_MAX_FRONTCACHE_COUNT` | unlimited | Drain only when the destination class frontcache count is small |

Quick RUNS=3 stride checks were not promising:

| Drain config | `remote50` median ops/s | `remote90` median ops/s | Read |
|---|---:|---:|---|
| `STRIDE=2` | 13672577.62 | 7351493.57 | Quick looked plausible |
| `STRIDE=4` | 12605251.22 | 4230151.73 | No-go |
| `STRIDE=8` | 12203822.86 | 6101161.15 | No-go |

RUNS=10 for `STRIDE=2` did not hold:

| Drain config | `remote50` median ops/s | `remote90` median ops/s | Read |
|---|---:|---:|---|
| `STRIDE=2` | 13208613.68 | 6434072.00 | No-go versus selected capacity-256 baseline |

Frontcache count gating was better but still not enough for default promotion:

| Drain config | RUNS | `remote50` median ops/s | `remote90` median ops/s | Read |
|---|---:|---:|---:|---|
| `MAX_FRONTCACHE_COUNT=0` | 3 | 13811855.47 | 6813806.72 | Narrow, mixed |
| `MAX_FRONTCACHE_COUNT=1` | 3 | 13543466.10 | 7662096.68 | Remote90 specialist candidate |
| `MAX_FRONTCACHE_COUNT=4` | 3 | 14209003.27 | 7084600.33 | Best quick balance |
| `MAX_FRONTCACHE_COUNT=4` | 10 | 14180156.45 | 6926930.38 | Small remote90 win, small remote50 loss |

`MAX_FRONTCACHE_COUNT=4` passed integrity smoke with zero requeue and route
failure gates, but the smoke only drained 45 objects:

| Counter | Value |
|---|---:|
| `remote_free_backpressure_drain_attempt` | 56 |
| `remote_free_backpressure_drain_success` | 45 |
| `remote_free_backpressure_retry_success` | 45 |
| `remote_free_backpressure_requeue_fail` | 0 |

Keep both controls as opt-in A/B knobs. The default-selected lane remains
`HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_L1=0`.

## Occupancy Observation

The follow-up observe counters do not change behavior.  On selected smoke they
showed that reserve failures happen when the 256-entry transfer cache is truly
full:

| Counter | Value |
|---|---:|
| `transfer_reserve_full` | 33465 |
| `transfer_reserve_full_transfer_count_total` | 8567040 |
| `transfer_reserve_full_class_count_total` | 5123433 |
| `transfer_reserve_full_class_count_max` | 203 |

Average transfer occupancy at full was `256`, and the same-class average was
about `153`.  More eager drain is therefore the wrong default direction because
it converts full-cache pressure into extra committed rehome work.  The next
candidate should be a bounded overflow/pending policy that preserves the
reserve-before-mutation invariant.
