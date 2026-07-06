# HZ10MallocFastLeafSplit-L0

Verdict: GO.

Change:

- Split page-layer allocation into an inline active-page fast path and a
  noinline slow helper for first-touch, drain, find, harvest, adoption, and
  fresh page creation.
- Semantics unchanged.

Codegen:

- `hz10_malloc` has no stack frame/canary and no direct call on the common
  active-page pop path.
- Miss paths tail-jump to `hz10_public_entry_alloc_from_page_layer_slow`.

hz10-only RUNS=5 median:

```text
python_alloc   0.860s / 106,772 KiB
redis_setget   0.540s / 8,064 KiB
larson         4.180s / 283,648 KiB current
xmalloc_test   2.000s / 13,184 KiB
cache_scratch  1.090s / 3,968 KiB
mstress        0.210s / 204,852 KiB
sh6bench       0.430s / 319,744 KiB
```

Reference after `HZ10FreeFastLeafSplit-L0`:

```text
sh6bench 0.450s
python_alloc 0.860s
larson 4.176s
mstress 0.210s
```
