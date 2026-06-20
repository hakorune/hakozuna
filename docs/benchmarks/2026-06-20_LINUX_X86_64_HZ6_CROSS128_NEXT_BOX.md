# Linux x86_64 HZ6 Cross128 Next Box, 2026-06-20

Follow-up direction after `Cross128RouteCostAudit-L1`.

## Current Evidence

- `cross128_r90` is a fixed 128B Toy-size remote-heavy row.
- HZ6 is far behind HZ4/tcmalloc/HZ3 on this row in the allocator compare.
- `transfer_presence` helped the transfer-cache negative lookup side, but the
  remaining small-object weakness is now in owner-inbox maintenance/reuse.
- Broad follow-up knobs have been tested and rejected:
  class-shard tuning, source-boundary DirectReuse, front external-only
  maintenance, and larger pending drain budgets.
- The useful split-maintenance signal is not "drain more".  Split changes where
  inline maintenance is attempted, and the benefit is unstable when expressed
  through broad flags.

## Next Box

```text
ToyClass2FrontMaintenanceGate-L1
```

Boundary:

- Toy front, class 2 only.  `128B` requests map to Toy class 2 because the Toy
  class size is 256B.
- Gate front-side inline pending maintenance explicitly instead of relying on
  broad split/source/external-only flags.
- Do not change DirectReuse ordering.
- Do not increase pending drain budget.
- Do not touch MidPage or large paths.
- Default-off flag, production A/B safe.
- Keep the implementation in a small helper boundary; avoid editing the large
  allocator core files unless they only receive one-line hooks.

First implementation candidate:

```text
ToyClass2FrontInlineMaintenanceGate-L1
```

Goal:

- Reduce unstable front-side inline maintenance cost on fixed 128B remote-heavy
  rows while preserving the owner-inbox sink.
- Keep correctness authority unchanged.
- Avoid remote-thread frontcache mutation and broad draining.

Promotion evidence:

- `cross128_r90` production R3/R10 improves.
- `remote50` and `remote90` do not regress materially.
- Integrity gates stay zero.
- `toy_source_alloc` and pending backlog do not increase materially.
- Split-maintenance variance improves rather than just producing occasional
  high outliers.

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

## Owner-Maintenance Required Probe

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_owner_inbox_maintenance_prod_r10_20260620_172854`

Production R10 on `cross128_r90`:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p1_external_no_maintenance` | 0.52M | 0.42M | 0.66M | 87.5 MiB |
| `p1_external` | 3.29M | 2.25M | 14.47M | 76.6 MiB |

Decision:

```text
Disable owner-local pending maintenance:
  NO-GO
```

The owner-inbox sink needs owner-local maintenance to turn pending inventory
back into reusable objects.  Producer-side sink-only behavior removes
backpressure but leaves cross128 too slow and increases RSS.  The next box
should keep maintenance enabled and reduce either storage footprint or
maintenance scheduling cost.

## Owner-Inbox RSS Attribution Plan

Code inspection found that the lazy owner-inbox storage path still touches the
full pending storage block at first publish:

```text
hz6_owner_inbox_storage_alloc(sizeof(Hz6RemotePendingStorage))
  -> provider allocation
  -> hz6_remote_pending_storage_reset_block()
```

The storage provider also zeroes the block, but the larger RSS source is likely
`reset_block()`: it initializes all descriptor-capacity side arrays, external
ticket arrays, duplicate indexes, and counters before the first pending object
needs most of those fields.  Removing the provider zero-fill alone is unlikely
to move RSS while `reset_block()` still touches the full block.

Next observation box:

```text
OwnerInboxStorageTaxAudit-L1
```

Compare:

| Variant | Purpose |
| --- | --- |
| `p1_metadata` | inbox code/fields compiled in, producer and consumer off |
| `p1_external_no_maintenance` | producer sink on, storage allocated, consumer off |
| `p1_external` | producer sink and owner-local maintenance on |

Rows:

```text
local0
remote50
remote90
cross128_r90
```

Decision target:

```text
metadata-only tax:
  if low, keep compile-in path

producer allocation/reset tax:
  if high, next implementation must split or lazily initialize
  Hz6RemotePendingStorage

maintenance tax:
  if high, next implementation should tune owner-local consumer scheduling
```

Implementation note:

```text
Do not edit hz6_allocator_remote_pending_inbox.c in this box.
```

That file is currently a large ownership/state-machine module.  The next code
change should stay boxed and either add a small storage-shape helper or a narrow
runner/doc observation before touching the pending core.

## Owner-Inbox Storage Tax Audit

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_owner_inbox_storage_tax_prod_r3_20260620_173512`

Production R3:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_metadata` | `local0` | 14.85M | 14.50M | 14.97M | 71.9 MiB |
| `p1_external_no_maintenance` | `local0` | 15.46M | 11.91M | 16.15M | 67.4 MiB |
| `p1_external` | `local0` | 17.08M | 16.77M | 17.25M | 67.4 MiB |
| `p1_metadata` | `remote50` | 13.26M | 13.11M | 13.78M | 74.0 MiB |
| `p1_external_no_maintenance` | `remote50` | 13.04M | 11.96M | 13.98M | 74.9 MiB |
| `p1_external` | `remote50` | 13.84M | 13.56M | 14.81M | 74.9 MiB |
| `p1_metadata` | `remote90` | 10.75M | 10.08M | 10.99M | 76.7 MiB |
| `p1_external_no_maintenance` | `remote90` | 3.38M | 3.14M | 4.22M | 83.8 MiB |
| `p1_external` | `remote90` | 10.82M | 10.71M | 10.97M | 77.5 MiB |
| `p1_metadata` | `cross128_r90` | 3.33M | 0.41M | 15.56M | 75.0 MiB |
| `p1_external_no_maintenance` | `cross128_r90` | 0.42M | 0.31M | 0.49M | 87.3 MiB |
| `p1_external` | `cross128_r90` | 9.12M | 4.45M | 15.06M | 73.0 MiB |

Decision:

```text
OwnerInboxStorageTaxAudit-L1:
  observed

p1_external_no_maintenance:
  NO-GO

p1_external:
  keep as the working owner-inbox sink baseline
```

Notes:

- `p1_external_no_maintenance` confirms that accumulating pending inventory
  without owner-local consumption is both slow and RSS-heavy on high-remote
  rows.
- `p1_external` is better than both metadata-only and no-maintenance variants
  on this R3 across `local0`, `remote50`, `remote90`, and `cross128_r90`.
- The next small-object optimization should not disable maintenance or chase
  provider zero-fill first.  The likely next box is a narrow owner-local
  maintenance shape for Toy class 2 that consumes pending objects with less
  scan/staging work while keeping the same sink semantics.

## Split-Maintenance Recheck

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_split_maintenance_prod_r3_20260620_173718`

Production R3:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external` | `remote50` | 13.60M | 12.48M | 14.11M | 74.9 MiB |
| `p1_external_split_maintenance` | `remote50` | 12.81M | 11.66M | 13.77M | 74.8 MiB |
| `p1_external` | `remote90` | 10.77M | 10.58M | 11.09M | 77.3 MiB |
| `p1_external_split_maintenance` | `remote90` | 10.79M | 10.51M | 10.92M | 77.4 MiB |
| `p1_external` | `cross128_r90` | 3.47M | 1.37M | 6.98M | 79.4 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 19.12M | 5.03M | 24.75M | 72.4 MiB |

Decision:

```text
p1_external_split_maintenance:
  GO for R10 confirmation on cross128_r90
```

This existing variant matches the emerging shape: keep the owner-inbox sink and
owner-local consumption, but separate the maintenance path so cross128 does not
pay the broad pending consumer cost.  The R3 result is high-variance, so the
next step is a focused `cross128_r90` R10 before turning this into a selected
candidate.

Focused R10 raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_split_maintenance_prod_r10_20260620_173807`

Production R10 on `cross128_r90`:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p1_external` | 4.66M | 1.64M | 6.91M | 74.7 MiB |
| `p1_external_split_maintenance` | 5.15M | 3.95M | 7.90M | 75.4 MiB |

Decision:

```text
p1_external_split_maintenance:
  HOLD / promising but not enough as-is
```

The R10 confirms a modest median and lower-tail improvement, but not the large
R3 jump.  This keeps the maintenance-shape direction alive while ruling out a
simple selected promotion of the existing split variant.  The next code box
should be narrower than the current split maintenance flag: target Toy class 2
pending consumption directly and keep guard rows for `remote50` and `remote90`.

## Split-Maintenance Class-Shard Probe

Raw outputs:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_split_class_shard_prod_r3_20260620_174125`
- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_split_class2_prod_r10_20260620_174204`

New runner variants:

```text
p1_external_split_maintenance_class2
p1_external_split_maintenance_small_class
```

These combine the owner-inbox split-maintenance shape with Toy class-id transfer
shard selection.

Production R3:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external_split_maintenance` | `remote50` | 13.78M | 13.57M | 13.88M | 74.9 MiB |
| `p1_external_split_maintenance_class2` | `remote50` | 13.17M | 11.59M | 14.15M | 74.8 MiB |
| `p1_external_split_maintenance_small_class` | `remote50` | 13.78M | 13.27M | 13.95M | 74.8 MiB |
| `p1_external_split_maintenance` | `remote90` | 10.65M | 10.58M | 10.71M | 77.5 MiB |
| `p1_external_split_maintenance_class2` | `remote90` | 10.66M | 10.37M | 10.75M | 77.6 MiB |
| `p1_external_split_maintenance_small_class` | `remote90` | 10.86M | 10.80M | 11.07M | 77.4 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 3.93M | 1.57M | 7.30M | 78.1 MiB |
| `p1_external_split_maintenance_class2` | `cross128_r90` | 15.37M | 7.74M | 15.44M | 72.1 MiB |
| `p1_external_split_maintenance_small_class` | `cross128_r90` | 3.45M | 3.03M | 6.23M | 75.9 MiB |

Production R10 on `cross128_r90`:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p1_external_split_maintenance` | 11.68M | 5.59M | 17.34M | 73.4 MiB |
| `p1_external_split_maintenance_class2` | 4.46M | 2.48M | 6.44M | 75.4 MiB |

Decision:

```text
Split maintenance + Toy class shard:
  NO-GO for the next selected candidate
```

The class2 combination looked promising in R3, but failed the focused R10 and
was worse than split maintenance alone.  The runner variants remain useful for
reproduction, but the next implementation box should not combine owner-inbox
maintenance with transfer class-shard placement.

## Split-Maintenance Counter Attribution

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_split_maintenance_diag_r3_20260620_174537`

Runner update:

```text
inline_toy / external_toy
c0..c4 inline/external maintenance drains
```

These counters are extracted from `HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL` so
future maintenance-shape boxes can see whether a result changes the actual Toy
class 2 consumption path.

Diagnostic R3 on `cross128_r90`:

| Counter | `p1_external` | `p1_external_split_maintenance` |
| --- | ---: | ---: |
| `remote_free_origin_pending_commit` | 549.1K | 586.7K |
| `remote_pending_enqueue_success` | 517.1K | 521.5K |
| `remote_pending_external_ticket_success` | 34.7K | 47.7K |
| `remote_pending_maintenance_check` | 420.0K | 457.9K |
| `remote_pending_maintenance_entry_gate_miss` | 349.2K | 613.9K |
| `remote_pending_maintenance_inline_gate_hit` | 419.8K | 401.4K |
| `remote_pending_maintenance_external_gate_hit` | 29.0K | 40.7K |
| `remote_pending_maintenance_inline_deferred` | 0 | 454.7K |
| `remote_pending_maintenance_inline_pop_success` | 391.7K | 400.3K |
| `remote_pending_maintenance_external_success` | 29.0K | 40.7K |
| `c2_inline` | 391.7K | 400.3K |
| `c2_external` | 29.0K | 40.7K |
| `remote_pending_batch_items` | 420.0K | 457.9K |
| `remote_pending_current` | 123.4K | 122.1K |
| `remote_pending_external_ticket_current` | 5.7K | 7.0K |

Decision:

```text
SplitMaintenance attribution:
  observed

Toy class 2 drain volume:
  not the primary differentiator

Next box:
  reduce or specialize front-side maintenance gating,
  not transfer class-shard placement and not more drain volume
```

Both variants drain roughly the same Toy class 2 inventory.  The split variant
mainly changes where inline work is attempted: many front-side inline checks
are deferred and the source-gate path performs the actual consumption.  This
explains why the existing split flag can improve lower-tail behavior without
being a stable selected candidate.  A future implementation should make this
boundary explicit for Toy class 2 instead of relying on the broad split flag.

## Split Source-Gate DirectReuse Probe

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_split_source_gate_prod_r3_20260620_175026`

New runner variants:

```text
p1_external_split_source_gate
p1_external_split_source_gate_observe
```

These combine the split-maintenance shape with existing source-boundary
DirectReuse controls.  This tests whether the useful part of split maintenance
can be made more explicit by using the pending direct reuse source-demand gate.

Production R3:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external_split_maintenance` | `remote50` | 13.81M | 13.13M | 13.93M | 74.6 MiB |
| `p1_external_split_source_gate` | `remote50` | 12.85M | 12.70M | 13.37M | 74.8 MiB |
| `p1_external_split_maintenance` | `remote90` | 11.01M | 10.96M | 11.03M | 77.5 MiB |
| `p1_external_split_source_gate` | `remote90` | 4.07M | 3.95M | 4.30M | 80.1 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 7.69M | 5.42M | 19.35M | 74.5 MiB |
| `p1_external_split_source_gate` | `cross128_r90` | 2.13M | 1.72M | 4.17M | 77.9 MiB |

Decision:

```text
Split maintenance + source-boundary DirectReuse:
  NO-GO
```

The source-boundary DirectReuse gate sharply regresses `remote90` and also
regresses `cross128_r90`.  The next implementation should not stack DirectReuse
onto split maintenance.  The remaining useful direction is still narrower:
make the front-side maintenance gate explicit for Toy class 2 without changing
activation/reuse ordering through the DirectReuse path.

## Front External-Only Maintenance Probe

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_front_external_only_prod_r3_20260620_175329`

New runner variant:

```text
p1_external_front_external_only
```

This isolates one half of split maintenance: front-side maintenance consumes
only external tickets and does not run inline pending maintenance there.

Production R3:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external` | `remote50` | 13.17M | 11.76M | 13.83M | 74.9 MiB |
| `p1_external_front_external_only` | `remote50` | 13.71M | 12.92M | 13.97M | 75.0 MiB |
| `p1_external_split_maintenance` | `remote50` | 13.39M | 12.73M | 13.97M | 75.0 MiB |
| `p1_external` | `remote90` | 10.49M | 10.45M | 10.92M | 77.4 MiB |
| `p1_external_front_external_only` | `remote90` | 10.78M | 10.67M | 10.88M | 77.6 MiB |
| `p1_external_split_maintenance` | `remote90` | 10.84M | 10.81M | 10.88M | 77.5 MiB |
| `p1_external` | `cross128_r90` | 5.79M | 2.33M | 10.06M | 75.0 MiB |
| `p1_external_front_external_only` | `cross128_r90` | 1.76M | 0.76M | 2.94M | 81.3 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 2.74M | 2.20M | 8.64M | 74.6 MiB |

Decision:

```text
Front external-only maintenance:
  NO-GO for cross128
```

Disabling inline maintenance on the front side without a stable replacement
regresses `cross128_r90`.  The next implementation should not simply skip
front inline maintenance.  It needs a narrow Toy class 2 policy that decides
when front-side inline maintenance is worth doing, instead of a broad
external-only rule.

## Pending Drain Budget Probe

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_pending_budget_prod_r3_20260620_175718`

New runner variants:

```text
p1_external_budget2
p1_external_budget4
p1_external_split_maintenance_budget2
p1_external_split_maintenance_budget4
```

These test whether simply draining more pending objects per maintenance call
helps the small-object remote-heavy row.  The run used production mode with
`remote50`, `remote90`, and `cross128_r90`.

Production R3:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external` | `remote50` | 13.25M | 13.09M | 13.81M | 74.8 MiB |
| `p1_external_budget2` | `remote50` | 13.93M | 13.41M | 14.14M | 74.8 MiB |
| `p1_external_budget4` | `remote50` | 11.71M | 11.26M | 12.85M | 74.8 MiB |
| `p1_external_split_maintenance` | `remote50` | 12.90M | 12.76M | 13.28M | 74.9 MiB |
| `p1_external_split_maintenance_budget2` | `remote50` | 13.52M | 11.29M | 14.10M | 74.9 MiB |
| `p1_external_split_maintenance_budget4` | `remote50` | 13.29M | 12.99M | 13.79M | 75.0 MiB |
| `p1_external` | `remote90` | 11.10M | 10.73M | 11.21M | 77.6 MiB |
| `p1_external_budget2` | `remote90` | 10.69M | 10.63M | 10.79M | 77.5 MiB |
| `p1_external_budget4` | `remote90` | 10.83M | 10.75M | 10.96M | 77.4 MiB |
| `p1_external_split_maintenance` | `remote90` | 10.95M | 10.83M | 10.98M | 77.4 MiB |
| `p1_external_split_maintenance_budget2` | `remote90` | 10.82M | 10.79M | 11.21M | 77.3 MiB |
| `p1_external_split_maintenance_budget4` | `remote90` | 10.59M | 10.58M | 11.05M | 77.4 MiB |
| `p1_external` | `cross128_r90` | 5.01M | 2.97M | 5.43M | 76.1 MiB |
| `p1_external_budget2` | `cross128_r90` | 1.08M | 0.86M | 1.57M | 91.4 MiB |
| `p1_external_budget4` | `cross128_r90` | 1.45M | 1.31M | 1.47M | 94.0 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 4.11M | 2.20M | 37.05M | 76.1 MiB |
| `p1_external_split_maintenance_budget2` | `cross128_r90` | 3.27M | 2.77M | 9.06M | 77.8 MiB |
| `p1_external_split_maintenance_budget4` | `cross128_r90` | 5.82M | 2.29M | 6.40M | 75.1 MiB |

Decision:

```text
Larger pending drain budget:
  NO-GO as the next cross128 optimization
```

The plain owner-inbox budget increases sharply regress `cross128_r90` and raise
RSS.  Split plus budget 4 has a slightly higher R3 median than `p1_external`,
but the p25 remains weak and the prior split evidence already showed high
variance.  This confirms that the next box should not be "drain more".  It
should express the useful part of split maintenance as a narrow Toy class 2
front-side gate.

R10 follow-up raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_pending_budget_r10_20260620_180145`

Production R10 on `cross128_r90` only:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p1_external` | 4.27M | 3.16M | 7.94M | 75.0 MiB |
| `p1_external_split_maintenance` | 16.68M | 8.45M | 35.48M | 72.1 MiB |
| `p1_external_split_maintenance_budget4` | 2.17M | 1.49M | 4.50M | 81.6 MiB |

The R10 follow-up closes the remaining ambiguity from the R3 probe:
`split_maintenance` is a real cross128 signal in this batch, but increasing the
drain budget destroys it.  The next behavior box should preserve split's
placement of inline work while keeping drain budget 1.

## Toy Class 2 Split-Maintenance Gate

Raw outputs:

- smoke: `hakozuna-hz6/private/raw-results/linux/hz6_toy2_split_smoke_20260620_180624`
- R3 guards: `hakozuna-hz6/private/raw-results/linux/hz6_toy2_split_prod_r3_20260620_180635`
- R10 cross128: `hakozuna-hz6/private/raw-results/linux/hz6_toy2_split_cross128_r10_20260620_180717`

Implementation box:

```text
ToyClass2FrontMaintenanceGate-L1
```

The box adds a small policy helper and a tax-runner variant:

```text
p1_external_toy2_split_maintenance
```

Behavior:

- Front-side maintenance uses external-only maintenance only for Toy class 2.
- Source-gate maintenance is enabled only for Toy class 2.
- Pending drain budget stays at 1.
- DirectReuse remains off.

Production R3 guards:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external` | `remote50` | 13.63M | 13.38M | 13.79M | 74.8 MiB |
| `p1_external_split_maintenance` | `remote50` | 12.41M | 12.12M | 13.88M | 74.8 MiB |
| `p1_external_toy2_split_maintenance` | `remote50` | 13.54M | 12.19M | 14.22M | 74.9 MiB |
| `p1_external` | `remote90` | 10.86M | 10.78M | 11.06M | 77.6 MiB |
| `p1_external_split_maintenance` | `remote90` | 10.98M | 10.83M | 11.08M | 77.5 MiB |
| `p1_external_toy2_split_maintenance` | `remote90` | 11.47M | 11.14M | 11.55M | 77.4 MiB |
| `p1_external` | `cross128_r90` | 7.35M | 3.23M | 39.54M | 74.0 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 12.59M | 7.86M | 36.93M | 72.6 MiB |
| `p1_external_toy2_split_maintenance` | `cross128_r90` | 7.86M | 1.55M | 41.03M | 74.8 MiB |

Production R10 on `cross128_r90`:

| Variant | Median ops/s | p25 | p75 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `p1_external` | 3.69M | 2.98M | 6.13M | 75.9 MiB |
| `p1_external_split_maintenance` | 13.64M | 10.86M | 23.53M | 72.4 MiB |
| `p1_external_toy2_split_maintenance` | 8.04M | 3.29M | 11.30M | 73.8 MiB |

Decision:

```text
ToyClass2FrontMaintenanceGate-L1:
  GO(research implementation)
  HOLD(selected/profile)
```

Toy2-limited split improves `cross128_r90` versus plain `p1_external` and
keeps the `remote50`/`remote90` guard rows healthier than broad split in the R3
batch.  It does not recover the full broad-split cross128 win, and its lower
tail remains weak.  The next step is a paired R10 guard matrix before any
profile promotion decision.

Paired R10 guard raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_toy2_split_guard_r10_20260620_181018`

Production R10:

| Variant | Row | Median ops/s | p25 | p75 | Peak RSS median |
| --- | --- | ---: | ---: | ---: | ---: |
| `p1_external` | `remote50` | 13.52M | 12.77M | 13.83M | 74.9 MiB |
| `p1_external_split_maintenance` | `remote50` | 13.38M | 13.18M | 13.57M | 74.9 MiB |
| `p1_external_toy2_split_maintenance` | `remote50` | 13.52M | 13.16M | 13.81M | 74.9 MiB |
| `p1_external` | `remote90` | 10.95M | 10.84M | 11.12M | 77.4 MiB |
| `p1_external_split_maintenance` | `remote90` | 10.87M | 10.58M | 10.98M | 77.4 MiB |
| `p1_external_toy2_split_maintenance` | `remote90` | 11.06M | 10.88M | 11.34M | 77.4 MiB |
| `p1_external` | `cross128_r90` | 5.53M | 3.04M | 7.59M | 74.9 MiB |
| `p1_external_split_maintenance` | `cross128_r90` | 13.97M | 10.15M | 24.84M | 72.6 MiB |
| `p1_external_toy2_split_maintenance` | `cross128_r90` | 11.55M | 7.04M | 17.87M | 73.1 MiB |

Decision update:

```text
ToyClass2FrontMaintenanceGate-L1:
  GO(profile candidate)
  HOLD(default)
```

The paired R10 guard matrix keeps `remote50` essentially flat, improves
`remote90` slightly, and more than doubles `cross128_r90` versus `p1_external`.
Broad split still wins the cross128 median, but it does not improve the guard
rows.  The Toy2 gate is therefore the better boxed candidate for a cross128
specialist profile; it is not a default candidate yet.
