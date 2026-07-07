# HZ11 Current Task

HZ11 is currently design-only.

```text
Active box:
  HZ11Positioning-L0 / HZ11ThreadCacheFastPath-L0

Goal:
  define a speed-first allocator line that can compete with tcmalloc without
  importing HZ8/HZ10 hot-path ownership/reclaim costs.

Do not do yet:
  implement allocator source
  tune RSS
  add checked-mode diagnostics to the default path
  claim HZ11 replaces HZ8 or HZ10
```

## Restart Pointers

```text
README.md
docs/README.md
docs/HZ11_POSITIONING_L0.md
docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md
docs/HZ11_NO_GO_LEDGER.md
```

## Next Step

```text
Implement the smallest HZ11ThreadCacheFastPath-L0 prototype only after the
positioning and gates are reviewed.

First prototype scope:
  size-class lookup table
  TLS thread cache
  direct-mapped pointer token table
  route fallback on token miss
  fixed cache caps
  smoke and fixed-size local microbench
```
