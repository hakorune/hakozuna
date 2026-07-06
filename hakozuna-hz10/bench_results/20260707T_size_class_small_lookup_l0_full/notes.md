# HZ10SizeClassSmallLookup-L0 Full Guard

Verdict: NO-GO; prototype reverted.

RUNS=5 full allocator macro guard:

```text
sh6bench:
  glibc    0.950s / 424,576 KiB
  hz10     0.440s / 318,336 KiB
  tcmalloc 0.320s / 271,488 KiB
  mimalloc 0.280s / 272,380 KiB

python_alloc:
  glibc    1.210s / 92,028 KiB
  hz10     0.840s / 106,644 KiB
  tcmalloc 0.830s / 104,448 KiB
  mimalloc 0.690s / 102,452 KiB

larson:
  glibc    4.143s / 272,640 KiB
  hz10     4.175s / 283,260 KiB
  tcmalloc 4.148s / 278,912 KiB
  mimalloc 4.153s / 283,832 KiB
```

Reference:

- The preceding `HZ10MallocFastLeafSplit-L0` full guard had sh6bench at
  0.420s for hz10.
- This table path did not improve python_alloc or larson and worsened the
  target sh6bench row relative to that latest full reference.

Decision:

- Revert the lookup table.
- Do not reopen this box unless a future layout-controlled experiment shows
  the table can beat the existing arithmetic path in a same-build A/B.
