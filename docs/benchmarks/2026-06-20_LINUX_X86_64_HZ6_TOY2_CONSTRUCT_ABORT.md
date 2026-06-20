# Linux x86_64 HZ6 Toy2 Construct Abort, 2026-06-20

## Box

`Toy2UnretainedSourceBlockAbort-L1`

Default-off construction rollback for the explicit
`hz6-cross128-toy2-route-before-maps-target` profile.  The behavior is limited
to newly-created Toy class2 source blocks whose prefill produced zero slots.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy2_construct_abort_r3_20260620_225457
hakozuna-hz6/private/raw-results/linux/hz6_toy2_construct_abort_r5_20260620_225549
```

## Implementation

Normal `hz6_allocator_release_source_block()` still treats `ref_count == 0` as a
no-op success.  The new path adds:

```text
hz6_allocator_abort_unretained_source_block()
```

The abort path requires:

```text
active block
backing pointer present
ref_count == 0
run_used_count == 0
no live descriptor pointing at the block
no exact live route pointing at the block descriptor
```

On success it uses the same source-block finalizer as last-ref release, so range
index unregister, invalid-range route unregister, backing release, metadata
reset, and `active = 0` stay shared.

## Production R5

`RUNS=5 ROWS=cross128_r90,remote90`

| row | variant | median ops/s | p25 ops/s | `alloc_fail` | `descriptor_exhausted` | `source_block_exhausted` | abort success | hard zero gates |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `cross128_r90` | route-before-maps | 11.13M | 9.98M | 4,768 | 10,496 | 3,319 | 0 | 0 |
| `cross128_r90` | route-before-maps + abort | 17.34M | 8.64M | 520 | 1,561 | 0 | 520 | 0 |
| `remote90` | route-before-maps | 11.20M | 11.08M | 0 | 0 | 0 | 0 | 0 |
| `remote90` | route-before-maps + abort | 11.15M | 11.11M | 0 | 0 | 0 | 0 | 0 |

Hard zero gates:

```text
toy2_construct_abort_ref_nonzero = 0
toy2_construct_abort_live_descriptor = 0
toy2_construct_abort_live_route = 0
toy2_construct_abort_pending_reference = 0
toy2_construct_abort_double_release = 0
toy2_construct_abort_release_fail = 0
toy2_unretained_active_block_after_abort = 0
```

## Read

The suspected cascade exists in production-shaped route-before-maps runs.  When
descriptor pressure prevents the first Toy2 slot from materializing, the old
`filled == 0` cleanup could leave an active zero-ref source block because normal
release is refcounted and returns success for `ref_count == 0`.

The abort path removes the secondary source-block exhaustion in this R5 batch:

```text
source_block_exhausted: 3319 -> 0
alloc_fail:             4768 -> 520
descriptor_exhausted:   10496 -> 1561
```

The remaining failure mode is descriptor pressure, not source-block record
pressure.  This keeps the next step narrow: add Toy2 descriptor pressure
diagnostics first, then consider lazy Toy2 descriptor segments if ACTIVE
descriptor pressure is confirmed.

## Decision

```text
Toy2UnretainedSourceBlockAbort-L1:
  GO(profile research)
  HOLD(default)
```

Do not increase source-block capacity from this evidence.  The next box should
be `Toy2DescriptorPressureAudit-L1`, focused on Toy class2 descriptor failure
composition and high-water accounting.
