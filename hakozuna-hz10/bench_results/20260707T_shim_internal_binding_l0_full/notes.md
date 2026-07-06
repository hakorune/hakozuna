# HZ10ShimInternalBinding-L0 Full Matrix Guard

Status: completed RUNS=5 all-allocator guard.

Command:

```text
OUTDIR=bench_results/20260707T_shim_internal_binding_l0_full \
RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

Median summary for main allocators:

```text
workload        glibc                 hz10                  tcmalloc              mimalloc
python_alloc   1.210s /  92,188 KiB  0.850s / 106,836 KiB  0.830s / 104,448 KiB  0.690s / 102,412 KiB
redis_setget   0.520s /   8,064 KiB  0.540s /   8,064 KiB  0.550s /   8,064 KiB  0.550s /   8,064 KiB
larson         4.147s / 272,640 KiB  4.187s / 284,404 KiB  4.153s / 278,784 KiB  4.154s / 284,148 KiB
xmalloc_test   2.040s / 198,224 KiB  2.000s /  13,056 KiB  2.030s / 195,968 KiB  2.000s /  22,480 KiB
cache_scratch  1.100s /   3,456 KiB  1.100s /   3,968 KiB  1.100s /   7,552 KiB  1.100s /   5,724 KiB
mstress        0.210s / 363,524 KiB  0.210s / 204,416 KiB  0.160s / 218,368 KiB  0.240s / 341,100 KiB
sh6bench       0.950s / 424,576 KiB  0.480s / 318,976 KiB  0.320s / 271,232 KiB  0.250s / 272,700 KiB
```

Read:

- Internal binding preserves the macro comparison band.
- sh6bench improves vs the prior speed-stack full matrix (`0.510s -> 0.480s`
  in full matrix, `0.470s` in hz10-only lane).
- python_alloc slightly improves; larson/cache/xmalloc/mstress are unchanged
  within row noise.
- RSS remains flat.

Verdict: full-matrix guard supports HZ10ShimInternalBinding-L0 GO.
