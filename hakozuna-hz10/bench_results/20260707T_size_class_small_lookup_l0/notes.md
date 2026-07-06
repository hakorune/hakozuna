# HZ10SizeClassSmallLookup-L0

Verdict: NO-GO.

Prototype:

- `hz10_size_class_for(size <= 1024)` used a 64-entry lookup table.
- Larger sizes stayed on the existing arithmetic path.
- `smoke-size-class` stayed green and verified all byte sizes.
- `objdump --disassemble=hz10_malloc` showed the intended shape: small sizes
  used the table and `bsr` stayed only on the larger-size path.

hz10-only RUNS=5 median:

```text
python_alloc   0.840s / 106,700 KiB
redis_setget   0.540s / 8,064 KiB
larson         4.173s / 283,644 KiB current
xmalloc_test   2.000s / 13,568 KiB
cache_scratch  1.090s / 3,968 KiB
mstress        0.210s / 205,188 KiB
sh6bench       0.430s / 318,592 KiB
```

Read:

- Correct and codegen-visible, but not a win.
- sh6bench matched the previous hz10-only reference after
  `HZ10MallocFastLeafSplit-L0` rather than improving it.
- Keep the existing arithmetic path and move the next speed work to a larger
  structural box.
