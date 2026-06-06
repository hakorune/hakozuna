# HZ6 LargeDirectRetain Cap Ladder

Source:

```text
results/hz6-large-direct-retain-cap-ladder-repeat3/
  20260606_235616_hz6_capacity_matrix_windows.md
```

Scope:

```text
family: mixed_ws
profiles: speed, rss
lanes:
  largerlowrss-front8k-sourcerun-desc8k-route8k
  largedirectretain-largerlowrss-front8k-sourcerun-desc8k-route8k
  largedirectretain16m-largerlowrss-front8k-sourcerun-desc8k-route8k
  largedirectretain32m-largerlowrss-front8k-sourcerun-desc8k-route8k
benchmarks:
  large_direct_slice_2m
  large_direct_slice_4m
  large_direct_slice_8m
  large_slice_1m
  large_slice_512k
runs: 3
```

Median read:

```text
large_direct_slice_2m:
  base: 0.296M speed / 0.333M rss, source_alloc 16000
  8M:   19.406M speed / 19.178M rss, source_alloc 16
  16M:  21.999M speed / 29.509M rss, source_alloc 16
  32M:  25.555M speed / 22.698M rss, source_alloc 16

large_direct_slice_4m:
  base: 0.290M speed / 0.293M rss, source_alloc 10004
  8M:   2.610M speed / 2.826M rss, source_alloc 985
  16M:  14.933M speed / 19.843M rss, source_alloc 12
  32M:  15.614M speed / 17.400M rss, source_alloc 12

large_direct_slice_8m:
  base: 0.300M speed / 0.302M rss, source_alloc 6000
  8M:   0.581M speed / 0.592M rss, source_alloc 3004
  16M:  10.946M speed / 11.568M rss, source_alloc 8
  32M:  11.498M speed / 11.421M rss, source_alloc 8
```

Guard rows:

```text
large_slice_1m:
  source_alloc stays 32 for all lanes.

large_slice_512k:
  source_alloc stays 64 for all lanes.
```

Safety:

```text
All lanes and guard rows:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  alloc_fail = 0
  descriptor_exhausted = 0
  source_block_exhausted = 0
```

Conclusion:

```text
16M is the clean practical candidate.  The 8M default cap is enough for 2M but
not enough for 4M/8M; it leaves visible source churn.  16M removes the churn for
2M/4M/8M, and 32M provides only small extra speed variance rather than a clear
new behavior tier.  Keep 32M as an upper-bound/control row.
```
