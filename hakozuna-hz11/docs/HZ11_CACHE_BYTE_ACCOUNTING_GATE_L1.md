# HZ11CacheByteAccountingGate-L1

Status: planned and implemented as an opt-in sibling lane; measure before any
promotion.

## Motivation

After `HZ11TLSFastPath-L1`, the fixed64 gap is smaller but still large:

```text
tcmalloc:           ~87 instr/op
hz11-token-tlsfast: ~114 instr/op
hz11-span-tlsfast:  ~116 instr/op
```

The remaining hot pop/push body still pays per-operation byte accounting:

```text
slot-size calculation
cached_bytes load/add/sub/store
HZ11_MAX_CACHED_BYTES comparison
```

This is useful as a global per-thread cap, but it is not necessary for the
fixed-local speed ceiling. The per-class `HZ11_CACHE_CAP` already bounds each
class cache, and L1 needs to know how much instruction budget the global byte cap
costs.

## Mechanism

Build flag:

```text
HZ11_CACHE_BYTE_ACCOUNTING=0
```

Default remains `1`.

Sibling lanes combine the accounting change with the already-GO TLS fast path:

```text
libhz11_nobytes.so
libhz11_span_nobytes.so
```

When byte accounting is disabled:

```text
pop  : does not subtract slot size from cached_bytes
push : checks only per-class HZ11_CACHE_CAP
push : does not add slot size to cached_bytes
```

Cold flush paths continue to clear per-class caches. `cached_bytes` is not a
meaningful diagnostic in the no-bytes sibling; it remains present only so the API
shape does not change.

## Boundary

In scope:

- `hz11_thread_cache_pop`
- `hz11_thread_cache_push`
- sibling build lanes and attribution columns

Out of scope:

- default promotion
- transfer cache, span reclaim, pageheap
- stats semantics beyond documenting `cached_bytes` as disabled in this lane

## Risk

The no-bytes sibling can retain more than `HZ11_MAX_CACHED_BYTES` across many
classes. The fixed-local benchmark mostly holds one cached object, so this box is
only a speed-ceiling A/B. It is not a production policy.

## Gate

GO:

```text
fixed64 hz11-token-nobytes instr/op drops clearly from tlsfast (~114)
fixed64 hz11-span-nobytes instr/op drops clearly from tlsfast (~116)
ops/s improves or stays within noise while instr/op drops
smoke green in token/span no-bytes lanes
```

NO-GO:

```text
instr/op barely moves
ops/s regresses enough to cancel the instruction-count win
smoke fails in either lane
```

## Verification

```text
make smoke preload-token-nobytes preload-span-nobytes hz11-standalone-check
RUNS=5 ITERS=10000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
git diff --check
```

Record the verdict in `docs/HZ11_NO_GO_LEDGER.md`.

## Result

Verdict: GO as a speed-ceiling cleanup; not promoted to default policy.

Primary confirmation:

```text
RUNS=10 ITERS=20000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
```

Fixed64 result:

```text
allocator             ops/s       instr/op  cycles/op
tcmalloc              188.1M        86.7      21.9
hz11-token-tlsfast    147.8M       114.3      27.4
hz11-token-nobytes    154.1M       101.3      26.3
hz11-span-tlsfast     158.1M       116.3      25.3
hz11-span-nobytes     165.0M       103.2      24.4
```

The instruction-count gate passed:

```text
token: 114.3 -> 101.3 instr/op  (-13.0)
span : 116.3 -> 103.2 instr/op  (-13.1)
```

Throughput also improved:

```text
token: 147.8M -> 154.1M ops/s
span : 158.1M -> 165.0M ops/s
```

The full fixed-size sweep (`RUNS=5 ITERS=10000000`) showed the same shape across
16/64/256/4096. Objdump matched the intended hot-path shrink:

```text
token hz11_malloc: 111 -> 107
token hz11_free  :  59 ->  49
span  hz11_malloc:  77 ->  70
span  hz11_free  :  61 ->  50
```

The remaining fixed64 instruction gap is now roughly:

```text
tcmalloc:            87 instr/op
hz11-token-nobytes: 101 instr/op  (+14)
hz11-span-nobytes:  103 instr/op  (+16)
```

This lane is valuable as a speed ceiling, but it weakens global per-thread cache
capacity control. Any promotion would need a replacement policy such as
low-cost per-class caps, periodic byte sampling, or a transfer-cache design that
restores an RSS bound outside the malloc/free hit path.
