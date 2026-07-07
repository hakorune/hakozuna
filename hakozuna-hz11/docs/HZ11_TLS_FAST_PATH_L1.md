# HZ11TLSFastPath-L1

Status: implemented as an opt-in sibling lane; measure before promoting.

## Motivation

`HZ11CacheShape-L1` closed the pointer-top cache-shape idea as NO-GO. The
remaining fixed64 gap to tcmalloc is still instruction count:

```text
tcmalloc      ~87 instr/op
hz11-token   ~125 instr/op
hz11-span    ~131 instr/op
```

After counters and cache shape, the next named cost is thread-local access and
the resolver guard at public entry. In the default lane, every `hz11_malloc` and
`hz11_free` checks `hz11_resolving` and then calls `hz11_thread_cache_get()`,
which reads `hz11_tls` and may initialize it. On the hot path, the thread cache
already exists and the resolver is already complete.

This box tests whether splitting public entry into a TLS-present fast path and a
resolver/init slow path removes a measurable number of instructions.

## Mechanism

Build flag:

```text
HZ11_TLS_FASTPATH=1
```

Default remains `0`. The default `libhz11.so` and `libhz11_span.so` lanes stay as
the comparator.

Sibling lanes:

```text
libhz11_tlsfast.so
libhz11_span_tlsfast.so
```

The fast path reads `hz11_tls` once:

```c
H11ThreadCache* tc = hz11_tls;
if (__builtin_expect(tc != NULL, 1)) {
  return hz11_malloc_fast_with_tc(tc, size);
}
return hz11_malloc_slow(size);
```

`hz11_free` follows the same shape after the null-pointer check. The slow helper
keeps the existing rules:

```text
resolver reentry -> system allocator
TLS missing      -> initialize cache
init failure     -> system allocator
```

No allocator semantics change. The split is only a code-generation A/B.

## Boundaries

In scope:

- `malloc` and `free` public-entry shape.
- token and span sibling lanes.
- fixed-local attribution table columns.

Out of scope:

- `realloc`, `calloc`, alignment APIs.
- cache layout changes.
- token hash or span classify changes.
- default promotion.

## Gate

GO:

```text
fixed64 hz11-token-tlsfast instr/op drops clearly from ~125
fixed64 hz11-span-tlsfast instr/op drops clearly from ~131
ops/s improves or stays within noise while instr/op drops
token/span smoke green
objdump shows the slow resolver/init block is out of the hot side
```

NO-GO:

```text
instr/op barely moves
ops/s regresses despite lower instruction count
smoke fails in either lane
```

If NO-GO, the next likely direction is not another TLS micro-box; the gap is
probably the remaining pop/push and size-class/body shape.

## Verification

```text
make smoke preload-token-tlsfast preload-span-tlsfast hz11-standalone-check
RUNS=5 ITERS=10000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
git diff --check
```

Record the decision in `docs/HZ11_NO_GO_LEDGER.md`.

## Result

Verdict: GO.

Command:

```text
RUNS=5 ITERS=10000000 ./scripts/run_hz11_frontend_attribution.sh
```

Fixed64 result:

```text
allocator             ops/s       instr/op  cycles/op
tcmalloc              177.8M        87.3      22.7
hz11-token            136.8M       125.5      29.6
hz11-token-tlsfast    147.3M       114.5      27.6
hz11-span             145.6M       131.5      27.4
hz11-span-tlsfast     158.4M       116.5      41.9
```

The instruction-count gate passed decisively:

```text
token: 125.5 -> 114.5 instr/op  (-11.0)
span : 131.5 -> 116.5 instr/op  (-15.0)
```

Throughput also improved:

```text
token: 136.8M -> 147.3M ops/s
span : 145.6M -> 158.4M ops/s
```

All fixed-size rows showed the same instruction-count shape: token-tlsfast stayed
near 114.5 instr/op and span-tlsfast near 116.5 instr/op. Objdump also matched
the intended shape:

```text
token hz11_malloc: 116 -> 111
token hz11_free  :  75 ->  59
span  hz11_malloc:  80 ->  77
span  hz11_free  :  89 ->  61
```

The `fixed64 >= tcmalloc * 0.95` speed target is still not reached; this box is a
GO as a real hot-path cleanup, not as the final tcmalloc-tier solution.

Next box: attribute or reshape the remaining body cost after TLSFastPath. The
remaining fixed64 instruction gap is now roughly:

```text
tcmalloc:           87 instr/op
hz11-token-tlsfast: 114 instr/op  (+27)
hz11-span-tlsfast:  116 instr/op  (+29)
```
