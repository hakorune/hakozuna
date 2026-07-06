# HZ10 Shim Speed Stack Macro Refresh L0

Status: completed RUNS=5 all-allocator refresh after:

- HZ10ShimTlsModelFix-L0
- HZ10ShimStatsFastGuard-L0
- HZ10ShimOwnerLookupInline-L0

Command:

```text
OUTDIR=bench_results/20260707T_shim_speed_stack_macro_refresh_l0 \
RUNS=5 make -C hakozuna-hz10 bench-macro-matrix
```

## Median Summary

```text
workload        glibc                 hz10                  tcmalloc              mimalloc
python_alloc   1.220s /  92,168 KiB  0.870s / 106,728 KiB  0.820s / 104,448 KiB  0.700s / 102,364 KiB
redis_setget   0.540s /   8,192 KiB  0.540s /   8,064 KiB  0.550s /   8,192 KiB  0.540s /   8,064 KiB
larson         4.140s / 272,384 KiB  4.179s / 283,392 KiB  4.158s / 279,040 KiB  4.161s / 283,872 KiB
xmalloc_test   2.040s / 198,360 KiB  2.000s /  13,440 KiB  2.030s / 196,224 KiB  2.000s /  22,608 KiB
cache_scratch  1.090s /   3,456 KiB  1.100s /   3,968 KiB  1.100s /   7,552 KiB  1.100s /   5,724 KiB
mstress        0.220s / 363,848 KiB  0.210s / 204,012 KiB  0.160s / 224,384 KiB  0.240s / 334,952 KiB
sh6bench       0.950s / 424,576 KiB  0.510s / 319,488 KiB  0.320s / 272,384 KiB  0.250s / 272,844 KiB
```

## Read

HZ10 is now in the product-lane comparison band on most rows:

- python_alloc: faster than glibc, still slower than tcmalloc/mimalloc.
- redis_setget: parity.
- larson: parity with glibc/tcmalloc/mimalloc in wall and near-parity in RSS.
- xmalloc_test: fastest-band wall with much lower RSS than glibc/tcmalloc.
- cache_scratch: parity wall, small RSS.
- mstress: faster than glibc/mimalloc and lower RSS than all but the smallest
  sample lanes, but still slower than tcmalloc.
- sh6bench: much faster than glibc after the shim speed stack, but still the
  clearest remaining wall-time gap vs tcmalloc/mimalloc.

Rollback lanes remain useful:

- `hz10-coarse` keeps current orphan/partial adoption without fine classes.
- `hz10-base` confirms the old no-adoption larson RSS cliff remains isolated
  to the rollback lane.
- `hz10+orphan` remains the idle-only adoption compatibility lane and is not a
  default candidate.

## Verdict

The TLS/stats/owner inline stack should be treated as the new macro baseline.
Do not open another shim routing box until a fresh perf attribution is taken on
the remaining gaps, primarily sh6bench and secondarily mstress-vs-tcmalloc.
