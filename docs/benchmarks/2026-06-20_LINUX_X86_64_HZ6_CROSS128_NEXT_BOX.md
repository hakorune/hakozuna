# Linux x86_64 HZ6 Cross128 Next Box, 2026-06-20

Follow-up direction after `Cross128RouteCostAudit-L1`.

## Current Evidence

- `cross128_r90` is a fixed 128B Toy-size remote-heavy row.
- HZ6 is far behind HZ4/tcmalloc/HZ3 on this row in the allocator compare.
- Diagnostic counters show the row is dominated by remote transfer pressure and
  transfer-cache scan/full cost, not source allocation.
- `toy_source_alloc` is about 1K, while `returned_backpressure` is about 125K.
- `small_class_shard` reduces `transfer_pop` and scan work, but does not remove
  the full/backpressure tail.

## Next Box

```text
ToyCross128TransferFastPath-L1
```

Boundary:

- Toy/front small only.
- Prefer exact 128B class first; do not affect MidPage or large paths.
- Default-off flag, production A/B safe.
- No profile mixing with `transfer_presence`.

First implementation candidate:

```text
Toy128OriginTransferSpecialize-L1
```

Goal:

- Reduce origin-transfer full/scan cost for fixed 128B remote frees.
- Keep correctness authority unchanged.
- Avoid remote-thread frontcache mutation and broad draining.

Promotion evidence:

- `cross128_r90` production R3/R10 improves.
- `remote50` and `remote90` do not regress materially.
- Integrity gates stay zero.
- `returned_backpressure` and `transfer_reserve_full` decrease on cross128.

## Class-Shard Scope Observation

Raw outputs:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_class_shard_scope_r3_20260620_165039`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_class_shard_scope_prod_r3_20260620_165432`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_class_shard_scope_prod_r10_20260620_165535`

`128B` maps to Toy class `2` because the Toy size table is:

```text
class 0: 16B
class 1: 64B
class 2: 256B
class 3: 1024B
class 4: 4096B
```

Production R10 on `cross128_r90`:

| Variant | Median ops/s | p25 | p75 | Note |
| --- | ---: | ---: | ---: | --- |
| `p0_off` | 7.36M | 6.09M | 8.52M | selected owner-slot shard |
| `p0_transfer_presence_class2` | 3.82M | 0.73M | 5.81M | class shard through Toy <=256B |
| `p0_transfer_presence_small_class` | 2.97M | 1.46M | 4.30M | class shard through Toy <=1024B |

Decision:

```text
Toy128 class-shard scope tuning:
  NO-GO as the next behavior box
```

The class-shard variants reduce some transfer-pop counts, but they do not
stabilize the row and regress the production median.  The next small-object
box should target the remaining transfer full/backpressure sink behavior
instead of widening class-id shard placement.

## Overflow Sink Probe

Raw outputs:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_overflow64_r3_20260620_165920`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_overflow1024_r3_20260620_170030`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_overflow1024_prod_r3_20260620_170204`

Existing default-off overflow sink controls:

```text
HZ6_REMOTE_FREE_OVERFLOW_L1=1
HZ6_REMOTE_FREE_OVERFLOW_CAPACITY=64 or 1024
```

Diagnostic R3:

| Variant | Median ops/s | returned backpressure | transfer reserve full |
| --- | ---: | ---: | ---: |
| `p0_off` baseline | 0.388M | 125.6K | 691.6K |
| overflow64 | 0.435M | 124.0K | 722.0K |
| overflow1024 | 0.449M | 107.2K | 592.1K |

Production R3 for overflow1024:

```text
median cross128_r90 = 5.70M ops/s
```

Decision:

```text
RemoteFreeOverflowCapacity:
  HOLD / not the next behavior box
```

Increasing the overflow sink reduces backpressure, but not enough to close the
row, and the production result is still below the clean `p0_off` R10 median.
This points back to owner-side consumption and transfer pressure shape rather
than a simple reserve-capacity fix.
