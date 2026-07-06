# HZ10FreeFastLeafSplit-L0 Full Guard

RUNS=5 full allocator macro guard after the `hz10_free()` fast leaf split.

Median highlights:

```text
sh6bench:
  glibc    0.960s / 424,576 KiB
  hz10     0.440s / 321,280 KiB
  tcmalloc 0.320s / 271,360 KiB
  mimalloc 0.270s / 271,664 KiB

python_alloc:
  glibc    1.220s / 91,916 KiB
  hz10     0.860s / 106,712 KiB
  tcmalloc 0.830s / 104,576 KiB
  mimalloc 0.700s / 102,488 KiB

larson:
  glibc    4.148s / 272,512 KiB
  hz10     4.186s / 284,016 KiB
  tcmalloc 4.158s / 278,912 KiB
  mimalloc 4.154s / 284,004 KiB

mstress:
  hz10     0.210s / 203,760 KiB
  tcmalloc 0.160s / 218,496 KiB
  mimalloc 0.230s / 320,460 KiB
```

Guard read:

- HZ10 remains competitor-band on python/redis/larson/cache/xmalloc/mstress.
- sh6bench improves versus the post-internal-binding full guard
  (`0.480s -> 0.440s`) while RSS remains in the same band.
- Remaining sh6bench gap is still instruction quantity versus tcmalloc and
  mimalloc; next low-risk box is the same leaf split strategy for
  `hz10_malloc()`.
