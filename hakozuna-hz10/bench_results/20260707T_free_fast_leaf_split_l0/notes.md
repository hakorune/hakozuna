# HZ10FreeFastLeafSplit-L0

Verdict: GO.

Change:

- Split `hz10_free()` so the common local-free path no longer carries
  authoritative route fallback, large free, or remote free publication in the
  same function body.
- Added noinline helpers for slow route and remote claim/note/publish.
- Kept validation, marker/accounting, front-cache behavior, and remote ordering
  unchanged.

Codegen:

- Before: `hz10_free` had a stack frame, canary, and direct slow/remote calls.
- After: `hz10_free` has no `push`, no `sub rsp`, no `%fs:0x28` canary, and
  no call on the common local-free path. Slow/remote paths are tail jumps.

hz10-only RUNS=5 median:

```text
python_alloc   0.860s / 106,636 KiB
redis_setget   0.540s / 8,064 KiB
larson         4.176s / 282,368 KiB current
xmalloc_test   2.000s / 13,312 KiB
cache_scratch  1.090s / 3,968 KiB
mstress        0.210s / 202,252 KiB
sh6bench       0.450s / 320,128 KiB
```

Reference baseline from `HZ10ShimInternalBinding-L0` hz10-only RUNS=5:

```text
sh6bench 0.470s
python_alloc 0.850s
larson 4.183s
mstress 0.210s
```

Read:

This validates the "instruction shape, not single instruction" diagnosis. The
single-instruction canary/div probes regressed; moving cold work out of the hot
function reduced the instruction path and improved sh6bench without changing
allocator semantics.
