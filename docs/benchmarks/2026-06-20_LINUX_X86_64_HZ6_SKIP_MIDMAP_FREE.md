# Linux x86_64 HZ6 Skip MidPage Free Map Probe, 2026-06-20

## Box

`SkipMidPageFreeMapProbe-L1`

Default-off research switch:

```text
HZ6_FREE_SKIP_MIDPAGE_ACTIVE_MAP_L1=1
```

The switch skips the allocator-side MidPage active-map probe in `hz6_free()`
and `hz6_free_with_route_prechecked()`. It does not change the preload
resolver contract and does not change selected/default behavior.

Raw result:

```text
hakozuna-hz6/private/raw-results/linux/hz6_toy2_skip_midmap_official_r5_20260620_184844
```

## Median Results

Official-profile shaped build, `HZ6_PRELOAD_STATS=0`, `RUNS=5`.

| Variant | remote50 | remote90 | cross128_r90 | Peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| `toy2_split` | 13.39M | 11.25M | 6.76M | 74-77 MiB |
| `toy2_split + skip_midmap` | 4.04M | 1.53M | 32.92M | 68 MiB cross128, ~2932 MiB remote90 |

## Read

Skipping allocator-side MidPage active-map probing stabilizes the Toy-only
`cross128_r90` row in this small batch, but it badly damages mixed-size rows.

The remote50/remote90 loss is expected: those rows can legitimately benefit
from the MidPage active-map fast path. A global skip moves them into the heavier
route/pending path and remote90 RSS explodes.

This is not a valid high-remote profile change. It is only useful as evidence
that the tiny cross-owner row wants a more precise page/front selector, not a
blanket MidPage bypass.

## Decision

`NO-GO(default/profile)`.

Keep the switch as a research control. The next viable behavior should be more
selective:

```text
ToyOnlyFreeDispatchHint-L1
```

The selector must skip MidPage probing only when the free pointer is proven to
belong to Toy-small ownership/page state. It must preserve MidPage fast paths
for mixed remote50/remote90.
