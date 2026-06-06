# HZ6 LargeDirectRetain32M Direct-Push Repeat-3

Source run:

```text
results/hz6-large-direct-retain32m-directpush-speedrss-repeat3/
  20260606_232222_hz6_capacity_matrix_windows.md
```

Legacy runner connection check:

```text
results/hz6-legacy-large-direct-retain-connect/
  20260606_232854_allocator_matrix.md
```

Scope:

```text
family: mixed_ws
profiles: speed, rss
lanes:
  largerlowrss-front8k-sourcerun-desc8k-route8k
  largedirectretain32m-largerlowrss-front8k-sourcerun-desc8k-route8k
benchmarks:
  large_direct_slice_2m
  large_direct_slice_4m
  large_direct_slice_8m
  large_slice_1m
  large_slice_512k
```

Median read:

```text
large_direct_slice_2m:
  base 0.323M speed / 0.354M rss
  retain32m 20.779M speed / 24.934M rss
  source_alloc 16000 -> 16

large_direct_slice_4m:
  base 0.305M speed / 0.293M rss
  retain32m 15.680M speed / 17.701M rss
  source_alloc 10004 -> 12

large_direct_slice_8m:
  base 0.304M speed / 0.301M rss
  retain32m 11.480M speed / 9.993M rss
  source_alloc 6000 -> 8

large_slice_1m guard:
  base 36.647M speed / 32.352M rss
  retain32m 35.765M speed / 33.296M rss
  source_alloc 32 -> 32

large_slice_512k guard:
  base 48.379M speed / 48.588M rss
  retain32m 45.341M speed / 52.697M rss
  source_alloc 64 -> 64
```

Safety:

```text
route_invalid = 0
route_miss = 0
route_register_fail = 0
alloc_fail = 0
descriptor_exhausted = 0
source_block_exhausted = 0
```

Conclusion:

```text
LargeDirectRetain32M is a strong >1MiB direct-large candidate/control.  It
converts direct OS source churn into exact-size retained reuse without changing
LargeSpan source allocation counts for the 512K/1M guard rows.  Keep it out of
broad selected defaults until a cross-allocator large_slices refresh confirms
the guard rows remain stable.
```

Legacy connection read:

```text
The legacy allocator matrix now has hz6-*-largedirectretain32m-largerlowrss
rows.  The connection run compares only HZ6 base largerlowrss vs retain32m on
2M/4M/8M direct slices and reproduces the HZ6-only signal:

  2M: 0.332M -> 20.533M
  4M: 0.297M -> 19.117M
  8M: 0.307M -> 10.069M

Safety counters remain zero.
```
