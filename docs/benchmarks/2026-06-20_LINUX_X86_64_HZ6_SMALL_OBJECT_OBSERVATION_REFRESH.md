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

## Tail Split Follow-Up

Production-shaped route-before-maps raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_cross128_tail_prod_counters_r10_20260620_215543
```

This reproduced the tail, but the production build compiles out the hook and
pending phase counters.  The useful production-level counters showed
`returned_backpressure=0`, `returned_uncommitted=0`, `origin_pending_commit=0`,
and `transfer_reserve_full=0` in every run.  That rules out returned
backpressure and owner-inbox pending publish as the visible production tail
cause in this batch.

Phase-counter-only route-before-maps raw:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_cross128_phase_counters_r10_20260620_215725
```

This build preserved phase counters without enabling diagnostic probes.

| run | ops/s | `alloc_fail` | `descriptor_exhausted` | `source_block_exhausted` | route-after-maps | fallback proven external |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | 1.36M | 17,331 | 38,855 | 13,165 | 17,351 | 17,331 |
| 7 | 6.15M | 12,983 | 28,846 | 10,106 | 13,002 | 12,983 |
| 6 | 6.18M | 11,641 | 25,202 | 9,723 | 11,662 | 11,641 |
| 8 | 13.21M | 2,186 | 5,865 | 695 | 2,207 | 2,186 |
| 2 | 18.01M | 677 | 2,032 | 0 | 697 | 677 |
| 1 | 24.56M | 0 | 0 | 0 | 19 | 0 |
| 3 | 27.09M | 0 | 0 | 0 | 20 | 0 |

Read:

- Low route-before-maps `cross128_r90` runs line up with descriptor/source
  supply failures, not returned backpressure or pending publish.
- In the phase-counter build, `alloc_fail` and
  `free_route_before_maps_fallback_proven_external` are equal in the bad runs.
  The later route-after-map count is the same signal plus the small valid-route
  tail.
- Fast runs have no descriptor/source exhaustion and almost no route-after-map
  work.
- The next viable behavior box is not external real-free consumption.  It should
  first prevent or explain the descriptor/source exhaustion that makes valid HZ6
  frees fall through as `PROVEN_EXTERNAL`.

## Capacity Split Follow-Up

Capacity raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_descriptor_hybrid_r10_20260620_220330
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_capacity_split_r5_20260620_220520
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_wide_capacity_prod_r10_20260620_220558
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_xwide_capacity_prod_r5_20260620_220659
```

The first capacity pass applied the existing workload-descriptor-hybrid shape
to `route_before_maps`: route table `40960`, descriptors `10240`, source blocks
`1280`, and elastic descriptor overflow.  It improved phase-counter cross128
median to `22.34M` and held `remote90` at `11.30M`, but still had a low
cross128 run with `alloc_fail=6728`, `descriptor_exhausted=14648`, and
`source_block_exhausted=5537`.

The split pass showed both descriptor and source-block capacity matter:

| Variant | shape | cross128 median | low-run read |
| --- | --- | ---: | --- |
| `desc10240` | descriptors `10240`, source blocks unchanged | 23.60M | still hit `alloc_fail=4310`, `descriptor_exhausted=10508`, `source_block_exhausted=2424` |
| `source1280` | source blocks `1280`, descriptors unchanged | 16.49M | still hit `alloc_fail=14745`, `descriptor_exhausted=33411`, `source_block_exhausted=10855` |
| `wide12288` | descriptors `12288`, source blocks `1536` | 23.98M | all five runs had `alloc_fail=0`, `descriptor_exhausted=0`, `source_block_exhausted=0` |

Production-shaped `wide12288` still had tail runs, and those runs again showed
descriptor/source exhaustion in the production stats:

| run | ops/s | `alloc_fail` | `descriptor_exhausted` | `source_block_exhausted` |
| ---: | ---: | ---: | ---: | ---: |
| 7 | 1.47M | 42,167 | 88,654 | 37,850 |
| 9 | 2.74M | 27,012 | 59,784 | 21,256 |
| 2 | 12.72M | 1,511 | 4,462 | 72 |

An extra-wide production-shaped pass used descriptors `32768`, source blocks
`4096`, and route table `131072`.  This eliminated the exhaustion counters in
all measured runs and stabilized cross128:

| row | runs | ops/s min | ops/s median | ops/s max | peak MiB median |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cross128_r90` | 5 | 30.36M | 34.00M | 35.01M | 190.62 |
| `remote90` | 5 | 3.96M | 4.01M | 4.13M | 194.12 |

Read:

- The cross128 tail is capacity-sensitive.  When descriptor/source exhaustion is
  forced to zero, the route-before-maps cross128 tail disappears in this batch.
- The required static capacity is too large for profile promotion as-is: peak
  RSS rises to about `195 MiB`, and `remote90` remains in the weak 4M band.
- Elastic descriptor overflow did not help in the hybrid pass
  (`elastic_descriptor_overflow_alloc=0`), so the next design should not assume
  the existing overflow depot fixes this path.
- The next box should be a narrow Toy class2 descriptor/source reclamation or
  adaptive capacity design, not a broad static table increase.

## Decision

```text
SmallObjectObservationRefresh-L1:
  GO(observation)
  NO-GO(behavior from this evidence)
```

Next work should target the descriptor/source exhaustion cliff in
route-before-maps `cross128_r90` with a narrow Toy class2 reclamation or adaptive
capacity design.  Do not add a narrower `PROVEN_EXTERNAL` fallback consumer
until those fallthroughs are proven to be true platform external pointers rather
than HZ6 source/descriptor exhaustion artifacts.
