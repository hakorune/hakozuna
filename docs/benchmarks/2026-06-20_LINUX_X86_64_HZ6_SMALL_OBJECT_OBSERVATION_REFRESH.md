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

## Decision

```text
SmallObjectObservationRefresh-L1:
  GO(observation)
  NEXT(production/interleaved profile pair)
```

Next observation should be production/interleaved throughput on the same
profile pair.  If that still shows a cross128 win, the next design box should
target the remaining `PROVEN_EXTERNAL` fallback without reintroducing the
remote90 ambiguity that made broad external dispatch a no-go.
