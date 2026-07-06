# HZ10MallocFastLeafSplit-L0 Full Guard

RUNS=5 full allocator macro guard after `HZ10FreeFastLeafSplit-L0` plus
`HZ10MallocFastLeafSplit-L0`.

Median highlights:

```text
sh6bench:
  glibc    0.940s / 424,448 KiB
  hz10     0.420s / 320,256 KiB
  tcmalloc 0.320s / 271,104 KiB
  mimalloc 0.280s / 271,632 KiB

python_alloc:
  glibc    1.180s / 92,032 KiB
  hz10     0.840s / 106,688 KiB
  tcmalloc 0.820s / 104,448 KiB
  mimalloc 0.700s / 102,540 KiB

larson:
  glibc    4.146s / 272,512 KiB
  hz10     4.184s / 282,368 KiB
  tcmalloc 4.156s / 278,912 KiB
  mimalloc 4.154s / 284,252 KiB
```

Read:

- HZ10 remains competitor-band on python/redis/larson/cache/xmalloc/mstress.
- sh6bench moved from 0.480s after internal binding to 0.440s after free split,
  then 0.420s after malloc split.
- The next speed step should not be another leaf split; this shape family has
  paid out. Open a design box for route/class-state instruction reduction.
