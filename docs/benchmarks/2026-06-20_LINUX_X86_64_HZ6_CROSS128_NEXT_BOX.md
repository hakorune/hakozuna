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

## Owner-Inbox Sink Probe

Raw outputs:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_sink_r3_20260620_171429`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_sink_prod_r3_20260620_171626`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_sink_prod_r10_20260620_171650`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_source_gate_prod_r10_20260620_171821`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_small_class_prod_r10_20260620_171905`
- `hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_small_class_guard_prod_r3_20260620_172016`

Diagnostic R3:

| Variant | Median ops/s | returned backpressure | origin pending commit | pending current | transfer pop |
| --- | ---: | ---: | ---: | ---: | ---: |
| `p0_off` | 0.402M | 124.8K | 0 | 0 | 372.9K |
| `p1_external` | 0.467M | 0 | 542.4K | 123.1K | 9.7K |

Production R10:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p0_off` | 5.43M | 1.19M | 9.10M | 70.0 MiB |
| `p1_external` | 5.56M | 3.50M | 8.38M | 74.1 MiB |

`p1_external` turns the free-completion sink problem into owner-local pending
inventory.  It removes returned backpressure in diagnostic runs and improves
the low-percentile production result, but the median is only slightly better
and RSS rises by about 4 MiB in this row.

Follow-up variants:

| Variant | Median ops/s | p25 | p75 | Decision |
| --- | ---: | ---: | ---: | --- |
| `p1_external_source_gate` | 6.94M | 4.56M | 13.41M | HOLD; p75 improves but lower tail worsens |
| `p1_external_small_class` | 5.99M | 2.82M | 15.23M | HOLD; cross128 variance high, guard rows mixed |

Guard R3 for `p1_external_small_class` versus `p1_external`:

| Row | `p1_external` median | `p1_external_small_class` median | Decision |
| --- | ---: | ---: | --- |
| `remote50` | 13.85M | 14.32M | OK |
| `remote90` | 10.98M | 10.61M | slight regression |
| `cross128_r90` | 7.29M | 3.79M | regression in this batch |

Decision:

```text
OwnerInboxExternal for cross128:
  GO as sink direction

SourceGate / SmallClass on top:
  HOLD
```

The next implementation box should not add another broad placement policy.  It
should either make owner-inbox pending cheaper to consume for Toy class 2 or
reduce the fixed RSS/metadata tax of enabling owner inbox in the selected lane.

## Route-Pin Trust Probe

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_route_pin_prod_r10_20260620_172405`

Production R10 on `cross128_r90`:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p1_external` | 7.75M | 3.72M | 16.76M | 73.1 MiB |
| `p1_external_route_pin` | 0.97M | 0.86M | 1.25M | 112.2 MiB |

Decision:

```text
RemotePendingRoutePinTrust:
  NO-GO for cross128
```

Skipping production inline route validation through route-pin trust does not
improve the Toy class 2 sink path.  It sharply regresses throughput and RSS in
this row, so pending optimization should stay on storage footprint and
consumer scheduling rather than weakening route validation.
