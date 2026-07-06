# HZ10OwnerLocalPageIndex-L0

Verdict: NO-GO; prototype reverted.

Prototype:

- Added an opt-in `libhz10_owner_index.so` lane.
- `hz10_free()` tried a guarded per-owner small-page cache before the existing
  pagemap route.
- On hit, it still checked owner token, generation, span, alignment, and
  interior slot boundary before local free.
- Added a stats artifact for attribution.

Correctness:

- owner-index focused smoke: green
- default public-entry smoke: green
- shim API / foreign-free smoke: green
- standalone check: green

Stats probe:

```text
short sh6bench with stats artifact:
  owner_index_hit=183666044
  owner_index_miss=141985
  owner_index_guard_fail_owner=0
  owner_index_guard_fail_generation=0
  owner_index_guard_fail_interior=0
  owner_index_insert=5289
  owner_index_collision=654
```

RUNS=5 A/B:

```text
python_alloc:
  hz10             0.840s / 106,688 KiB
  hz10-owner-index 0.880s / 106,648 KiB

sh6bench:
  hz10             0.450s / 318,336 KiB
  hz10-owner-index 0.520s / 320,000 KiB

larson:
  hz10             4.175s / 281,728 KiB current
  hz10-owner-index 4.127s / 285,032 KiB current
```

Read:

- The cache hits, but the hit path is slower than the current pagemap route
  shape in the macro target.
- Likely cost: the pre-route branch and owner-index table load plus larger
  owner footprint/cache pressure.
- Do not keep the lane. The code was reverted and only this record remains.
