# Hakozuna HZ11

HZ11 is a speed-first allocator research line.

It is not a faster HZ8/HZ10 variant. HZ11 deliberately explores a
tcmalloc-style allocator shape: front-end caches, transfer caches, central
spans, and a pageheap. HZ8 remains the public low-RSS/correctness line, and
HZ10 remains the route/ownership/reclaim research line.

```text
state:
  design-only speed-first research line

default allocator:
  not HZ11

identity:
  tcmalloc-speed competitor
  RSS-capped, not low-RSS-first
  diagnostics and fail-closed checks belong to checked/debug lanes

first box:
  HZ11ThreadCacheFastPath-L0

non-goals:
  do not preserve HZ8/HZ10 owner-return remote-free on the hot path
  do not route every local free through pagemap metadata
  do not claim HZ11 replaces HZ8
```

## Read First

```text
docs/HZ11_POSITIONING_L0.md
docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md
docs/HZ11_NO_GO_LEDGER.md
```

## Core Split

```text
HZ8:
  public recommendation
  low post-workload RSS
  fail-closed ownership boundaries

HZ10:
  active speed/RSS-aware research candidate
  keeps explicit route, ownership, and reclaim boundaries

HZ11:
  speed-first line
  front-end cache is the product
  transfer cache and central spans are the refill/flush boundary
  checked diagnostics are optional lanes, not default hot-path work
```

## Initial Implementation Ladder

```text
1. HZ11ThreadCacheFastPath-L0
   per-thread cache, size-class table, token fast path, route fallback

2. HZ11TransferCacheCentralSpan-L1
   batch refill/return, transfer cache, central spans, minimal pageheap

3. HZ11PerCpuRseqCache-L2
   Linux rseq per-CPU cache for the real tcmalloc-class contest
```

## Check

```bash
make hz11-standalone-check
```
