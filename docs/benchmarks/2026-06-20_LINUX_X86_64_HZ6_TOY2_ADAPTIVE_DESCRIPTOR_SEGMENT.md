# Linux x86_64 HZ6 Toy2 Adaptive Descriptor Segment, 2026-06-20

## Decision

`Toy2AdaptiveDescriptorSegment-L1` is a GO profile candidate and remains
default-off research.

It removes the remaining Toy class2 allocation cliff after construction abort by
adding per-allocator lazy descriptor segments.  The implementation is limited to
Toy class2, checks route-table headroom before adding a segment, and keeps
segment storage alive until allocator destruction.

Do not promote it as a default yet.  It is a narrow capacity extension for the
`route_before_maps + Toy2 construction abort` profile, and it still needs a
longer interleaved guard before being folded into a named profile target.

## Implementation Notes

- Flag: `HZ6_TOY2_ADAPTIVE_DESCRIPTOR_SEGMENT_L1=1`.
- Segment size: `512` descriptors.
- Per-allocator segment max: `16`.
- Route headroom gate: exact route table occupied entries below `3/4`.
- Segment state is stored in a side table so `Hz6Allocator` size and hot layout
  do not change when the flag is off.
- Base descriptor prepare/reset fast paths avoid adaptive accounting calls.

## Raw Results

- `hakozuna-hz6/private/raw-results/linux/hz6_toy2_adaptive_descriptor_fastpath_r5_20260620_232941`
- `hakozuna-hz6/private/raw-results/linux/hz6_toy2_adaptive_descriptor_local_remote50_r5_20260620_233109`
- `hakozuna-hz6/private/raw-results/linux/hz6_toy2_adaptive_descriptor_interleave_r10_20260620_235034`
- Earlier failed profile check before the base fast-path guard:
  `hakozuna-hz6/private/raw-results/linux/hz6_toy2_adaptive_descriptor_max16_r5_20260620_232723`

## R5 Summary

Baseline is `p1_external_toy2_route_before_maps_abort`; candidate is
`p1_external_toy2_route_before_maps_adaptive`.

| Row | Baseline median | Adaptive median | Delta | Baseline p25 | Adaptive p25 | Peak RSS median |
|---|---:|---:|---:|---:|---:|---:|
| local0 | 16.21M | 16.71M | +3.1% | 16.13M | 15.68M | 67.38 -> 67.50 MiB |
| remote50 | 14.15M | 14.19M | +0.3% | 13.83M | 14.04M | 74.75 -> 74.75 MiB |
| remote90 | 11.15M | 11.26M | +1.0% | 10.98M | 11.24M | 77.12 -> 77.25 MiB |
| cross128_r90 | 21.47M | 33.23M | +54.8% | 4.08M | 29.92M | 72.75 -> 72.62 MiB |

## Capacity Counters

On the paired R5 cross128 row:

| Counter | Baseline | Adaptive |
|---|---:|---:|
| `alloc_fail` | 385 | 0 |
| `source_block_exhausted` | 0 | 0 |
| `toy2_descriptor_fail` | 771 | 0 |
| `toy2_adaptive_descriptor_alloc` | 0 | 2304 |
| `toy2_adaptive_descriptor_segment_alloc` | 0 | 6 |
| `toy2_adaptive_descriptor_segment_max_block` | 0 | 0 |
| `toy2_adaptive_descriptor_route_headroom_block` | 0 | 0 |
| `toy2_adaptive_descriptor_segment_high_water` | 0 | 3 |

`descriptor_exhausted` remains non-zero in the adaptive row because it records
base-table exhaustion attempts before the adaptive fallback succeeds.  The final
failure counters are the relevant gate here: `alloc_fail=0`,
`source_block_exhausted=0`, and `toy2_descriptor_fail=0`.

## Interleaved R10

The interleaved `RUNS=10` follow-up is the better promotion signal because it
removes batch drift between variants.

| Row | Abort median | Adaptive median | Abort p25 | Adaptive p25 | Note |
|---|---:|---:|---:|---:|---|
| local0 | 16.58M | 16.58M | 16.02M | 16.24M | flat |
| remote50 | 14.21M | 14.22M | 14.12M | 13.86M | flat |
| remote90 | 3.70M | 11.09M | 2.43M | 11.01M | adaptive preserves the good row |
| cross128_r90 | 35.84M | 34.78M | 3.48M | 31.16M | adaptive removes the cliff |

Adaptive counters on the interleaved cross128 row stayed clean:

- `alloc_fail=0`
- `source_block_exhausted=0`
- `toy2_descriptor_fail=0`
- `toy2_adaptive_descriptor_segment_alloc=2`
- `toy2_adaptive_descriptor_segment_high_water=2`
- `toy2_adaptive_descriptor_route_headroom_block=0`
- `toy2_adaptive_descriptor_segment_alloc_fail=0`
- `toy2_adaptive_descriptor_storage_owner_mismatch=0`

`abort` still has the higher median on the cross128 row in this batch, but its
tail is unstable enough that it is not the better profile choice.  The adaptive
variant is the more defensible candidate because it keeps `remote90` stable and
removes the `alloc_fail` / `toy2_descriptor_fail` cliff without extra RSS.

## Hard Gates

The measured rows kept these at zero:

- `toy2_adaptive_descriptor_segment_alloc_fail`
- `toy2_adaptive_descriptor_segment_max_block`
- `toy2_adaptive_descriptor_route_headroom_block`
- `toy2_adaptive_descriptor_double_allocate`
- `toy2_adaptive_descriptor_reuse_while_live`
- `toy2_adaptive_descriptor_storage_owner_mismatch`
- `toy2_construct_abort_ref_nonzero`
- `toy2_construct_abort_live_descriptor`
- `toy2_construct_abort_live_route`
- `toy2_construct_abort_pending_reference`
- `toy2_construct_abort_double_release`
- `toy2_construct_abort_release_fail`
- `toy2_unretained_active_block_after_abort`
- `free_route_real_free_unproven`
- `remote_free_returned_uncommitted`

## Notes

The first max-16 implementation removed cross128 allocation failures but still
regressed `remote90` even though adaptive descriptors did not fire there.  The
cause was code shape: descriptor prepare/reset called adaptive accounting for
base descriptors.  Guarding those calls to non-base descriptors recovered the
remote90 row.

Source-block adaptive capacity should stay separate.  Construction abort already
removed `source_block_exhausted`, and this patch closes the remaining Toy2
descriptor pressure without adding source-block records or payload residency.
