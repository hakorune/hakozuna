# HZ6 LargeDirectRetain Cross-Allocator Slice

Source:

```text
results/hz6-large-direct-retain-crossalloc-slice/
  20260606_233919_allocator_matrix.md
```

Scope:

```text
profiles:
  large_slice_512k
  large_slice_1m
  large_direct_slice_2m
  large_direct_slice_4m
  large_direct_slice_8m

allocators:
  hz3
  hz4
  hz5-policy
  hz6-speed-largerlowrss
  hz6-speed-largedirectretain32m-largerlowrss
  hz6-rss-largedirectretain32m-largerlowrss
  mimalloc
  tcmalloc
```

Single-run read:

```text
large_slice_512k:
  winner hz6-rss-largedirectretain32m-largerlowrss 50.975M / 9540 KB

large_slice_1m:
  winner hz6-rss-largedirectretain32m-largerlowrss 36.328M / 8972 KB

large_direct_slice_2m:
  winner hz6-speed-largedirectretain32m-largerlowrss 27.536M / 8896 KB
  mimalloc baseline 5.195M / 22536 KB

large_direct_slice_4m:
  winner hz6-rss-largedirectretain32m-largerlowrss 16.851M / 8888 KB
  mimalloc baseline 0.960M / 71428 KB

large_direct_slice_8m:
  winner hz6-speed-largedirectretain32m-largerlowrss 13.184M / 8860 KB
  hz4 baseline 12.309M / peak KB unavailable in this run
```

Safety:

```text
HZ6 retain32m rows:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  alloc_fail = 0
  descriptor_exhausted = 0
  source_block_exhausted = 0
```

Conclusion:

```text
LargeDirectRetain32M changes the old direct-large read.  HZ6 is no longer only
a low-RSS direct-release coverage row for 2M/4M/8M; in this single-run slice it
is the fastest row on all three direct-large profiles while staying low RSS.
Treat this as a strong follow-up signal, not a paper median.
```
