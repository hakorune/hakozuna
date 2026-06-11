# HZ7 v2 Baseline Snapshot

This folder records the Windows `random_mixed` cross-allocator baseline used to
close the current HZ7 v2 cap64 lane.

Source result:

```text
20260611_174745_paper_random_mixed_windows.md
```

Build/run shape:

```text
benchmark:
  bench_random_mixed_compare

profiles:
  small   16..2048,   ws=400, iters=20,000,000
  medium  4096..32768, ws=400, iters=20,000,000
  mixed   16..32768,  ws=400, iters=20,000,000

runs:
  repeat-3 median

allocators:
  hz3
  hz4
  hz7-v2
  mimalloc
  tcmalloc

HZ7 v2 lane:
  DirectRetainCap-L2 default
  H7_DIRECT_RETAIN_CAP = 64
```

Summary:

| profile | hz7-v2 median ops/s | hz7-v2 peak KB | fastest row | lowest-RSS row |
| --- | ---: | ---: | --- | --- |
| small | 79.741M | 4,576 | hz3 156.548M | hz7-v2 4,576 KB |
| medium | 53.353M | 5,140 | tcmalloc 154.623M | hz7-v2 5,140 KB |
| mixed | 52.911M | 5,664 | tcmalloc 151.538M | hz7-v2 5,664 KB |

Interpretation:

```text
HZ7 v2 is not the throughput winner in this cross-allocator random_mixed
snapshot. Its current strength is tiny/readable shape plus very low RSS.

The cap64 default materially improves the previous medium/mixed HZ7 v2 rows,
but HZ7 v2 remains a local tiny allocator reference, not a tcmalloc/HZ3 speed
replacement and not a remote-throughput allocator.
```

