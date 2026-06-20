# Linux x86_64 HZ6 Remote Allocator Compare With Route-Before-Maps, 2026-06-20

## Box

`RemoteAllocatorCompareRouteBeforeMapsR10-L1`

This run adds the explicit
`hz6-cross128-toy2-route-before-maps-target` profile to the MT remote allocator
frontier.  The profile is the `toy2_split` shape plus TLS-armed route resolution
before active-map probes and local-valid resolved dispatch.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_remote_allocator_compare_route_before_maps_r10_20260620_192235
```

Command shape:

```text
RUNS=10
ROWS=remote50,remote90,cross128_r90
ALLOCATORS=hz3,hz4,toy2_split,route_before_maps,mimalloc,tcmalloc
```

## Median Results

| Allocator | remote50 | remote90 | cross128_r90 | Peak RSS notes |
| --- | ---: | ---: | ---: | --- |
| `hz3` | 19.33M | 2.57M | 175.65M | remote90 RSS high, 1579.34 MiB |
| `hz4` | 9.53M | 30.91M | 209.01M | strongest remote90/cross128 legacy line |
| `toy2_split` | 14.00M | 11.21M | 7.65M | prior best HZ6 balanced profile |
| `route_before_maps` | 13.63M | 11.39M | 10.89M | best HZ6 remote90/cross128 in this run |
| `mimalloc` | 1.81M | 0.28M | 74.77M | remote90 RSS very high |
| `tcmalloc` | 67.44M | 7.34M | 187.10M | strongest remote50 and strong cross128 |

## Read

`route_before_maps` improves the HZ6 profile frontier versus `toy2_split` on
the two rows it targets:

```text
remote90:
  11.21M -> 11.39M

cross128_r90:
  7.65M -> 10.89M
```

The gain is real but not enough to change the cross-allocator frontier:

- `tcmalloc` still dominates `remote50`.
- `hz4` still dominates `remote90`.
- `hz4`, `tcmalloc`, and `hz3` are still an order of magnitude ahead on
  `cross128_r90`.

The small-object cross-owner row also remains high variance for HZ6 profiles.
The route-before-maps profile improves the median, but the run still contains a
very low tail (`395K ops/s`) and several high outliers.

## Decision

```text
route_before_maps:
  GO(profile candidate)
  HOLD(default)

RemoteAllocatorCompareRouteBeforeMapsR10-L1:
  GO(evidence)
  DESIGN checkpoint
```

Use `route_before_maps` as the current best HZ6 explicit high-remote/cross128
profile.  Do not promote it to selected/default: it remains a boxed profile and
still trails the tiny-object cross-owner frontier badly.

The next useful optimization should not stack more broad route-before-map work.
It should isolate the remaining HZ6 cross128 tail variance and instruction count
against the HZ4/tcmalloc shape.

## Interleaved Follow-Up

The remote rows showed batch-level variance after this comparison.  The compare
runner now has `--interleave-runs` so profile A/B can run in row/run/profile
order instead of grouping all runs by allocator.

Raw result:

```text
hakozuna-hz6/private/raw-results/linux/hz6_route_before_maps_interleave_r5_20260620_195150
```

Command shape:

```text
RUNS=5
ROWS=remote90,cross128_r90
ALLOCATORS=toy2_split,route_before_maps
--interleave-runs
```

| Profile | `remote90` median | `cross128_r90` median | Read |
|---|---:|---:|---|
| `toy2_split` | 4.07M | 4.57M | weak remote90 batch |
| `route_before_maps` | 4.10M | 9.88M | remote90 flat, cross128 improved |

This confirms that the weak `remote90` batch affected both profiles.  In the
same batch, `route_before_maps` still improved `cross128_r90` without a material
remote90 difference.  Use interleaved runs for future narrow HZ6 profile A/B
before attributing a row-wide cliff to a single flag.
