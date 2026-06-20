# Linux x86_64 HZ6 Cross128 Small Free Path Cost, 2026-06-20

## Box

`Cross128SmallFreePathCostAudit-L1`

Observation-only runner update: add existing preload/allocator free-path
counters to `run_hz6_preload_owner_inbox_tax_ab.sh`.

Raw result:

```text
hakozuna-hz6/private/raw-results/linux/hz6_cross128_small_free_path_cost_r3_20260620_183547
```

Run shape:

```text
DIAGNOSTIC=1
RUNS=3
VARIANTS=p1_external,p1_external_toy2_split_maintenance
ROWS=cross128_r90
```

## Median Counters

| Variant | free calls | Toy map hit | Toy map miss | MidPage map hit | route lookup after maps | foreign direct dispatch | pending commits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `p1_external` | 960,676 | 55,675 | 904,777 | 0 | 904,777 | 504,313 | 496,460 |
| `p1_external_toy2_split_maintenance` | 960,676 | 59,483 | 900,969 | 0 | 900,969 | 540,103 | 530,809 |

## Read

The cross128 row is all Toy class2 allocation size, but the active-map free
fast path only catches about six percent of frees. About `0.90M` frees per
diagnostic run fall through to route lookup after active maps.

Toy2 split improves reuse/pending flow, but it does not remove the dominant
free-path shape:

```text
free()
  -> Toy active-map attempt
  -> mostly miss
  -> MidPage active-map attempt, guaranteed miss in this row
  -> route lookup after maps
  -> foreign direct dispatch / owner pending commit
```

This explains the perf-stat signal from `Cross128Toy2Observe-L1`: HZ6 has high
IPC, but it retires far more instructions than HZ4/tcmalloc.

## Decision

`GO(tooling)/DESIGN checkpoint`.

Do not tune source allocation or pending drain volume from this result. The
next behavior candidates should be scoped to the tiny-object free dispatch:

- avoid guaranteed MidPage free-map probing for known Toy-only rows or pages
- improve Toy active-map hit coverage for remote-owned Toy class2 pointers
- pass the resolved foreign route through the free dispatch without repeated
  map/route work where the preload resolver already proved ownership

Any behavior must stay profile/default-off until paired `remote50`,
`remote90`, and `cross128_r90` guards pass.
