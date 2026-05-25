# HZ5 MidPageFront No-Go Log

This file records MidPage diagnostics that were useful as measurements but are
not promoted lanes.

## No-Go Diagnostics

### M6 deferred-free

Raw outputs:

```text
private/raw-results/linux/midpage_m6defer_direct_smoke_20260525_160954
private/raw-results/linux/midpage_m6defer_cap64_direct_smoke_20260525_161054
private/raw-results/linux/midpage_m6batch_cap64_direct_smoke_20260525_161155
private/raw-results/linux/midpage_m6batch_perf_20260525_161204
```

Decision:

```text
No-go. Delaying free validation increased refill/raw-buffer pressure and did
not close the tcmalloc gap.
```

### Tagged-free wrapper

Raw output:

```text
private/raw-results/linux/midpage_tagfree_hakmem_range_sweep_20260525_163526
```

Decision:

```text
No-go. The wrapper still calls page_for_ptr before free_tagged, so it does not
remove the actual ownership lookup.
```

### M4 overflow-array

Raw output:

```text
private/raw-results/linux/midpage_overarray_direct_range_sweep_20260525_164100
```

Decision:

```text
No-go. The TLS secondary array and branch cost outweighed the saved
local_pop/payload-node path.
```

## Diagnostic Only

### wide32k

`wide32k` is not a no-go because it is not meant to be promoted. It is a speed
upper-bound diagnostic that proves class dispersion is a major part of the
mixed-mid gap, but it is too wasteful for a default or broad speed profile.

### class-pattern phase/cycle

Useful for understanding class-locality effects. It does not, by itself, define
a promotable lane.
