# HZ5 Bench Results Index

This file indexes raw benchmark directories and the main decision they support.
Raw logs remain under `private/raw-results/linux/`.

## tcmalloc Target

### `tcmalloc_target_shadow_r3_20260525_033348`

Path:

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
```

Purpose:

```text
Compare region, midpage_region, shadow, and tcmalloc on main/mid_only r0/r50/r90.
```

Decision:

```text
shadow is the current tcmalloc-chase lead.
main rows are tcmalloc-class in this focused run.
mid_only_r0 remains the large gap.
```

Key medians:

```text
main_r0:     shadow 58.03M, tcmalloc  53.11M
main_r50:    shadow 31.85M, tcmalloc  30.91M
main_r90:    shadow 22.74M, tcmalloc  21.64M
mid_only_r0: shadow 71.36M, tcmalloc 139.58M
mid_only_r50:shadow 40.56M, tcmalloc  77.68M
mid_only_r90:shadow 41.27M, tcmalloc  48.23M
```

### `tcmalloc_shadow_frontfirst_r3_20260525_033543`

Path:

```text
private/raw-results/linux/tcmalloc_shadow_frontfirst_r3_20260525_033543
```

Purpose:

```text
Test shadow + MidPageFront-first free dispatch.
```

Decision:

```text
diagnostic only.
It helps pure mid_only slightly but hurts main/cross128.
```

Key medians:

```text
mid_only_r0:  shadow 74.17M, shadow_frontfirst 76.10M, tcmalloc 142.85M
mid_only_r90: shadow 28.77M, shadow_frontfirst 31.78M, tcmalloc  46.18M
cross128_r90: shadow 17.38M, shadow_frontfirst 11.87M, tcmalloc  7.73M
```

### `tcmalloc_tlscache_r3_20260525_033858`

Path:

```text
private/raw-results/linux/tcmalloc_tlscache_r3_20260525_033858
```

Purpose:

```text
Test shadow + TLS region lookup cache.
```

Decision:

```text
no-go for broad tcmalloc chase.
Region lookup is not the main remaining bottleneck.
```

Key medians:

```text
main_r90:     shadow 22.90M, tlscache 29.76M, tcmalloc 22.24M
mid_only_r0:  shadow 72.91M, tlscache 70.22M, tcmalloc 139.80M
mid_only_r50: shadow 40.79M, tlscache 36.47M, tcmalloc  80.02M
mid_only_r90: shadow 40.73M, tlscache 38.43M, tcmalloc  46.92M
cross128_r90: shadow 15.41M, tlscache 12.00M, tcmalloc  7.69M
```

## MidPageFront-M2

### `midpage_region_broad_r5_20260525_031852`

Path:

```text
private/raw-results/linux/midpage_region_broad_r5_20260525_031852
```

Purpose:

```text
Broad r5 comparison for region, midpage_region, HZ4, and tcmalloc.
```

Decision:

```text
midpage_region is a valid remote-heavy mid-size candidate.
It is not a broad default because r0 and large_only_r90 regress.
```

Key medians:

```text
main_r50:      region 30.92M, midpage_region 41.20M, tcmalloc 78.47M
main_r90:      region 29.48M, midpage_region 32.12M, tcmalloc 53.51M
mid_only_r50:  region 28.26M, midpage_region 44.43M, tcmalloc 78.08M
mid_only_r90:  region 29.81M, midpage_region 35.61M, tcmalloc 51.56M
cross128_r90:  region 13.88M, midpage_region 20.83M, tcmalloc  7.61M
large_only_r90:region 10.88M, midpage_region  6.88M, tcmalloc  3.63M
```

### `frontfirst_verify_r5_20260525_032806`

Path:

```text
private/raw-results/linux/frontfirst_verify_r5_20260525_032806
```

Purpose:

```text
Verify MidPageFront-first free dispatch after focused r3.
```

Decision:

```text
frontfirst is diagnostic only.
It helps mid_only_r90 but regresses main/cross128/guard.
```

## Older Results

The full chronological result log remains in:

```text
docs/archive/current_task_2026-05-hz5-linux.md
```

Use that archive for older Local2P, SmallFront, MidFront, LargeFront, OwnerHub,
and paper-main observations that have not yet been promoted into this index.
