# Hakozuna HZ11

HZ11 is a speed-first allocator research line.

It is not a faster HZ8/HZ10 variant. HZ11 deliberately explores a
tcmalloc-style allocator shape: front-end caches, transfer caches, central
spans, and a pageheap. HZ8 remains the public low-RSS/correctness line, and
HZ10 remains the route/ownership/reclaim research line.

```text
state:
  speed-first research line

default allocator:
  not HZ11

remote/mixed microbench speed lane:
  libhz11_span_transfer.so
  promoted by HZ11TransferPromotionMatrix-L1 on the micro remote/mixed matrix;
  remains the clean remote/mixed result; not default

recommended opt-in macro speed-lane candidate:
  libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
  backed by macro gate reclassification, current-RSS semantics, and RUNS=10
  remote/mixed final confirmation; not default

sh6bench/macro-churn specialist lane:
  libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap1024_bytes.so
  or libhz11_span_transfer_thread_exit_cap_batch32_fine128_cachecap768_bytes.so
  reaches near tcmalloc sh6bench wall on the synthetic macro gate, but regresses
  remote/mixed versus fine128; not the general recommendation and not default

identity:
  tcmalloc-speed competitor
  RSS-capped, not low-RSS-first
  diagnostics and fail-closed checks belong to checked/debug lanes

first box:
  HZ11ThreadCacheFastPath-L0

non-goals:
  do not preserve HZ8/HZ10 owner-return remote-free on the hot path
  do not route every local free through pagemap metadata
  do not claim HZ11 generally beats tcmalloc
  do not claim HZ11 replaces HZ8
```

## Read First

```text
docs/HZ11_POSITIONING_L0.md
docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md
docs/HZ11_FINE128_CANDIDATE_POSITIONING_L1.md
docs/HZ11_CAP1024_BYTES_CANDIDATE_POSITIONING_L1.md
docs/HZ11_THREAD_CACHE_CAPACITY_MIDDLE_LANE_L1.md
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

## Implementation Ladder

```text
1. HZ11ThreadCacheFastPath-L0
   per-thread cache, size-class table, token fast path, route fallback

2. HZ11TransferCacheCentralSpan-L1
   batch refill/return, transfer cache, central spans, minimal pageheap

3. HZ11PerCpuRseqCachePrototype-L2
   NO-GO as a locked per-CPU diagnostic lane; do not pursue lock-free rseq
   without new evidence

4. HZ11ThreadCacheCapacityByteCap-L1
   confirms the sh6bench wall lever is thread-cache capacity with byte-capped
   retention; cap1024-bytes is a specialist lane, not the general candidate
```

## Check

```bash
make hz11-standalone-check
```
