# Linux x86_64 HZ6 Toy2 Descriptor Pressure Audit, 2026-06-20

## Box

`Toy2DescriptorPressureAudit-L1`

Behavior-free audit after `Toy2UnretainedSourceBlockAbort-L1`.  The goal is to
split remaining descriptor failures for Toy class2 before deciding whether lazy
Toy2 descriptor segments are justified.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy2_descriptor_pressure_audit_r5_20260620_231121
```

## Counters

The audit records Toy class2 descriptor-failure snapshots only at call sites
that know both `front_id` and `class_id` and only after descriptor acquisition,
spill retry, and reuse dry-run have failed.

```text
toy2_descriptor_fail
toy2_descriptor_fail_active_max
toy2_descriptor_fail_local_free_max
toy2_descriptor_fail_transfer_free_max
toy2_descriptor_fail_remote_pending_max
toy2_descriptor_fail_frontcache_max
toy2_descriptor_fail_external_storage_max
toy2_source_blocks_ref_zero_max
toy2_source_blocks_low_ref_max
toy2_source_blocks_full_max
toy2_source_blocks_partial_max
```

## Production R5

Variant:

```text
p1_external_toy2_route_before_maps_abort
```

| row | median ops/s | `alloc_fail` | `descriptor_exhausted` | `source_block_exhausted` | `toy2_descriptor_fail` | Toy2 active max | Toy2 local free max | Toy2 transfer free max | Toy2 remote pending max | Toy2 frontcache max | Toy2 source block ref-zero/full/partial |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `cross128_r90` | 9.68M | 1,421 | 4,264 | 0 | 2,843 | 7,748 | 64 | 137 | 0 | 64 | 0 / 0 / 0 |
| `remote90` | 4.16M | 272 | 283 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 / 0 / 0 |

`remote90` was a weak batch for the route-before-maps profile, but the Toy2
descriptor audit did not fire there.  The row's residual descriptor failures are
not Toy class2 failures in this snapshot.

## Read

After construction abort, `cross128_r90` no longer shows source-block
exhaustion.  The remaining Toy2 failures are dominated by ACTIVE descriptors:

```text
toy2_descriptor_fail_active_max = 7748
toy2_descriptor_fail_local_free_max = 64
toy2_descriptor_fail_transfer_free_max = 137
toy2_descriptor_fail_remote_pending_max = 0
toy2_source_blocks_ref_zero/full/partial = 0
```

This does not support source-block record expansion or Toy2 source-run reuse as
the next narrow fix.  It supports `Toy2AdaptiveDescriptorSegment-L1`: descriptor
capacity should grow only for allocators that hit true Toy2 active-pressure,
with route headroom gates and segment high-water accounting.

## Decision

```text
Toy2DescriptorPressureAudit-L1:
  GO(observation)
```

Proceed to `Toy2AdaptiveDescriptorSegment-L1` before touching source-block
capacity.
