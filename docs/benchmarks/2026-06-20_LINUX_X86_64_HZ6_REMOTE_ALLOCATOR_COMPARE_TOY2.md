# Linux x86_64 HZ6 Remote Allocator Compare With Toy2 Split, 2026-06-20

## Box

`RemoteAllocatorCompareToy2R10-L1`

This run extends the MT remote allocator comparison with the new
`hz6-cross128-toy2-split-target` profile.

Raw results:

```text
hakozuna-hz6/private/raw-results/linux/hz6_remote_allocator_compare_toy2_r10_20260620_182215
```

Command shape:

```text
RUNS=10
ROWS=remote50,remote90,cross128_r90
ALLOCATORS=hz3,hz4,hz6,transfer_presence,small_class_shard,toy2_split,mimalloc,tcmalloc
```

## Median Results

| Allocator | remote50 | remote90 | cross128_r90 | Peak RSS notes |
| --- | ---: | ---: | ---: | --- |
| `hz3` | 17.81M | 2.51M | 175.10M | remote90 RSS high, 1585.95 MiB |
| `hz4` | 9.29M | 29.31M | 207.61M | strongest remote90/cross128 legacy line |
| `hz6` selected | 15.09M | 3.87M | 7.17M | balanced RSS, weak high-remote/cross128 |
| `transfer_presence` | 13.75M | 10.70M | 2.97M | high-remote profile, cross128 weak |
| `small_class_shard` | 14.54M | 4.04M | 8.00M | remote50/cross128 specialist only |
| `toy2_split` | 13.64M | 11.24M | 35.95M | best HZ6 remote90/cross128 balance |
| `mimalloc` | 1.85M | 0.28M | 76.54M | remote90 RSS very high |
| `tcmalloc` | 71.18M | 7.32M | 185.87M | strongest remote50 and strong cross128 |

## Read

`toy2_split` is the strongest HZ6 profile in this comparison for the
combined high-remote and tiny cross-owner row. It keeps the
`transfer_presence` remote90 gain while lifting `cross128_r90` from the
single-digit HZ6 band to `35.95M`.

The external frontier is still not closed:

- `tcmalloc` dominates `remote50` at `71.18M`.
- `hz4` dominates `remote90` at `29.31M`.
- `hz4`, `tcmalloc`, and `hz3` dominate `cross128_r90`.

`mimalloc` is strong on `cross128_r90`, but it is not competitive on
`remote50` or `remote90` in this lane and shows very high remote90 RSS.

## Decision

`toy2_split`: `GO(profile candidate)/HOLD(default)`.

Use `toy2_split` as the current best HZ6 high-remote plus cross128
profile candidate. Do not promote it to selected/default: it is not the
remote50 winner and still trails `hz4`/`tcmalloc`/`hz3` badly on the tiny
cross-owner row.

Next optimization should focus on the small-object cross-owner shape
rather than stacking more broad high-remote policy.
