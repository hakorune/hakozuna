# Linux x86_64 HZ6 Cross128 Toy2 Observe, 2026-06-20

## Box

`Cross128Toy2Observe-L1`

This is an observation-only pass after the `toy2_split` profile comparison.
No behavior changed.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy2_split_observe_r3_20260620_182932
hakozuna-hz6/private/raw-results/linux/hz6_cross128_perf_stat_20260620_183301
```

## Diagnostic Counter Read

`DIAGNOSTIC=1`, `RUNS=3`, `cross128_r90` only.

| Variant | Median ops/s | pending commits | transfer_pop | frontcache hits | inline drained | external drained |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `p1_external` | 0.41M | 496,147 | 7,230 | 552,879 | 345,847 | 23,807 |
| `p1_external_toy2_split_maintenance` | 0.39M | 518,560 | 9,286 | 578,102 | 356,907 | 35,232 |

The diagnostic binary is too heavy for speed judgement. Use these rows as
counter attribution only.

Read:

- Toy2 split increases Toy class2 pending reuse work and frontcache hits.
- Source allocation is unchanged (`source_alloc=1059` in both rows), so this is
  not primarily a source-allocation avoidance win.
- The useful production win is a work-placement/reuse effect, not a broad
  allocator supply fix.

## Perf Stat Read

Single production-shaped `cross128_r90` run per allocator.

| Allocator | ops/s | cycles | instructions | IPC | cache misses | branch misses |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `hz4` | 204.58M | 0.51B | 0.41B | 0.81 | 6.84M | 2.27M |
| `toy2_split` | 20.83M | 2.99B | 4.02B | 1.34 | 32.22M | 12.53M |
| `tcmalloc` | 174.90M | 0.52B | 0.25B | 0.49 | 7.20M | 3.03M |

Read:

- `toy2_split` is still roughly an order of magnitude heavier in retired
  instructions than HZ4/tcmalloc on the tiny cross-owner row.
- IPC is not the main problem. HZ6 is doing much more work.
- Cache and branch misses are also much higher, but they track the larger
  routing/pending/free path rather than a single obvious source path.

## Decision

`GO(observation)/DESIGN checkpoint`.

Do not stack more pending-drain or source-allocation policy from this evidence.
The next clean box should target tiny-object cross-owner free path instruction
count: route resolution, pending publish, and active-map/preload dispatch shape.

Candidate next box:

```text
Cross128SmallFreePathCostAudit-L1
```

The goal is to split per-free instruction cost before changing behavior:

- active-map hit/miss and dispatch counts
- resolver/global directory route hits
- owner-inbox publish path counts
- exact route lookup counts
- transfer/pending sink selection counts
