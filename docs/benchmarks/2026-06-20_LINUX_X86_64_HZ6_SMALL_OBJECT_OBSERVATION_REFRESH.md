# Linux x86_64 HZ6 Small Object Observation Refresh, 2026-06-20

## Box

`SmallObjectObservationRefresh-L1`

This is an observation-only checkpoint before any new tiny-object optimization.
HZ6 improved materially on the high-remote profile family, but the small
cross-owner row is still far behind HZ4/tcmalloc/HZ3.  Do not add another
behavior box until the next run confirms where the current profile spends work.

## Current Read

The latest public evidence points to instruction/work count on tiny frees, not
source allocation:

- `cross128_r90` is Toy class 2: 128B requests are served by the 256B Toy
  class.
- `Cross128Toy2Observe-L1` showed HZ6 retiring roughly `4.02B`
  instructions in a single production-shaped `cross128_r90` run, versus
  `0.41B` for HZ4 and `0.25B` for tcmalloc.
- `Cross128SmallFreePathCostAudit-L1` showed Toy active-map free hits around
  six percent, with about `0.90M` frees falling through to route lookup after
  active-map probes in the diagnostic row.
- `PreloadForeignRouteBeforeMaps-L1` improves the current HZ6 cross128 profile,
  but the gap to external/legacy allocators remains large.

## Observation Shape

Run the current best HZ6 profile and its control in the same diagnostic batch:

```text
VARIANTS=p1_external_toy2_split_maintenance,p1_external_toy2_route_before_maps
ROWS=cross128_r90
DIAGNOSTIC=1
```

Record:

- throughput median, p25, p75
- Toy active-map free hit/miss
- MidPage active-map free probe count
- route lookup after active maps
- route-before-maps dispatch/fallback split
- owner-inbox pending commits and final pending balance
- remote-free returned backpressure/uncommitted/stale/integrity

## Decision Rule

This box is `GO(observation)` only if it changes no allocator behavior and keeps
integrity counters clean.  The next behavior should be chosen from the observed
dominant cost:

- If active-map miss plus route-after-maps still dominates, target a narrower
  resolved-route dispatch boundary.
- If pending publish/maintenance dominates, target Toy class2 owner-local
  consumption placement.
- If route-before-maps removes most route-after-map work but leaves a large
  `PROVEN_EXTERNAL` fallback, target that fallback only with paired remote90
  guards.

Do not use broad MidPage-skip, larger pending-drain budgets, or transfer-cache
layout rewrites from this evidence; those boxes already failed their guard rows.

## Diagnostic Refresh

Raw result:

```text
hakozuna-hz6/private/raw-results/linux/hz6_small_object_observation_refresh_r3_20260620_195735
```

Run shape:

```text
DIAGNOSTIC=1
RUNS=3
ROWS=cross128_r90
VARIANTS=p1_external_toy2_split_maintenance,p1_external_toy2_route_before_maps
```

Median counter read:

| Variant | diag ops/s | free calls | Toy map hit | route-after-maps | before-maps foreign | before-maps local | before-maps fallback | pending commit | source alloc |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `p1_external_toy2_split_maintenance` | 0.46M | 960,676 | 58,803 | 901,649 | 0 | 0 | 0 | 526,068 | 1,059 |
| `p1_external_toy2_route_before_maps` | 0.74M | 960,676 | 97 | 116,268 | 759,019 | 85,097 | 116,249 | 747,608 | 573 |

Integrity gates stayed clean in both variants:

```text
remote_free_returned_backpressure = 0
remote_free_returned_uncommitted = 0
route_unregister_while_pending = 0
route_replace_while_pending = 0
route_rehome_while_pending = 0
```

Read:

- `route_before_maps` removes most route-after-map work on this tiny row:
  `901,649 -> 116,268`.
- The remaining route-after-map work is essentially the before-maps fallback
  count.  In this run that fallback is `PROVEN_EXTERNAL`, not owner-inbox
  backpressure.
- Toy active-map hit rate collapses under route-before-maps because most frees
  no longer reach the active-map probe path.  That is expected and is not, by
  itself, a regression.
- Source allocation also falls (`1,059 -> 573`), but this remains secondary to
  the free-route dispatch shape.

## Pair Guard Follow-Up

Production/interleaved pair raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_pair_prod_interleave_r5_20260620_213806
```

| Profile | row | ops/s p25 | ops/s median | ops/s p75 |
| --- | --- | ---: | ---: | ---: |
| `toy2_split` | `remote90` | 3.82M | 4.04M | 4.19M |
| `route_before_maps` | `remote90` | 3.97M | 3.98M | 4.03M |
| `toy2_split` | `cross128_r90` | 35.72M | 36.81M | 37.44M |
| `route_before_maps` | `cross128_r90` | 5.66M | 36.18M | 36.44M |

Diagnostic pair raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_pair_diag_r3_20260620_213845
```

| Variant | row | diag ops/s median | route-after-maps | before-maps foreign | before-maps local | before-maps fallback proven external | returned backpressure | uncommitted |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `p1_external_toy2_split_maintenance` | `remote90` | 3.06M | 783,512 | 0 | 0 | 0 | 0 | 94,333 |
| `p1_external_toy2_route_before_maps` | `remote90` | 6.01M | 20 | 863,609 | 96,812 | 0 | 0 | 69,337 |
| `p1_external_toy2_split_maintenance` | `cross128_r90` | 0.42M | 904,844 | 0 | 0 | 0 | 0 | 0 |
| `p1_external_toy2_route_before_maps` | `cross128_r90` | 0.79M | 99,672 | 774,153 | 86,424 | 99,654 | 2,472 | 0 |

Read:

- The diagnostic shape still says route-before-maps removes most route-after-map
  work on both rows.
- The cross128 residual is still almost exactly `PROVEN_EXTERNAL` fallback.
- The production/interleaved guard does not preserve the cross128 win: median is
  slightly lower than `toy2_split`, and the route-before-maps p25 collapses to
  about `5.66M`.
- Because the production tail regressed before adding any new fallback consumer,
  the next behavior should not be another `PROVEN_EXTERNAL` real-free variant.

## Decision

```text
SmallObjectObservationRefresh-L1:
  GO(observation)
  NO-GO(behavior from this evidence)
```

Next work should explain the route-before-maps production tail before changing
behavior: split the low cross128 runs by returned backpressure, origin-transfer
fallback, and pending/external maintenance shape.  Do not add a narrower
`PROVEN_EXTERNAL` fallback consumer until the production pair has a stable
cross128 win with a neutral remote90 guard.
