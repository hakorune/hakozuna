# HZ11CacheLayout-L1

Status: implemented as SOA sibling lanes; measured NO-GO against the instruction
gate, but kept as an opt-in speed-ceiling probe.

## Motivation

`HZ11RemainingBodyAttribution-L0` identified the class-cache address calculation
as the largest remaining actionable cost after TLSFastPath and no-bytes:

```text
tcmalloc              ~86.7 instr/op
hz11-token-nobytes   ~101.3 instr/op
hz11-span-nobytes    ~103.2 instr/op
```

The default cache layout is array-of-structs:

```c
H11ClassCache class_cache[HZ11_CLASS_COUNT];
```

`H11ClassCache` is 264 bytes in the count-indexed lane, so accessing
`class_cache[class_id]` can lower to an awkward `*33` / `*264` address chain.
This box tested a structure-of-arrays layout.

## Mechanism

Build flag:

```text
HZ11_CACHE_SOA=1
```

SOA lanes are only defined as speed-ceiling siblings on top of TLSFastPath and
no-bytes:

```text
libhz11_soa.so
libhz11_span_soa.so
```

The SOA shape:

```c
void* class_items[HZ11_CLASS_COUNT][HZ11_CACHE_CAP];
uint32_t class_counts[HZ11_CLASS_COUNT];
```

Guardrails:

```text
HZ11_CACHE_SOA && HZ11_CACHE_TOPPTR          -> compile error
HZ11_CACHE_SOA && HZ11_CACHE_BYTE_ACCOUNTING -> compile error
```

## Result

Verdict: NO-GO for the instruction-count gate.

Reported fixed64 result (`RUNS=5`, `ITERS=10M`):

```text
allocator             ops/s       instr/op  cycles/op
tcmalloc              186.8M        87.1      22.4
hz11-token-nobytes    150.3M       101.2      26.4
hz11-span-nobytes     165.2M       103.2      24.4
hz11-token-soa        159.1M        98.3      38.9*
hz11-span-soa         165.5M       100.2      24.4
```

`token-soa` cycles/op looked like a perf outlier; the instruction count was
stable across two measurements (`98.2-98.3 instr/op`).

Confirmation run after wiring SOA into
`scripts/run_hz11_frontend_attribution.sh`:

```text
RUNS=5 ITERS=10000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
```

```text
allocator             ops/s       instr/op  cycles/op
tcmalloc              189.0M        87.3      22.7
hz11-token-nobytes    153.6M       101.4      26.6
hz11-token-soa        159.3M        98.4      25.7
hz11-span-nobytes     163.8M       103.4      24.6
hz11-span-soa         156.1M       100.5      26.0
```

This independently confirms the instruction-count win is about 3 instr/op. It
also shows the span lane can lose throughput despite the lower instruction
count, reinforcing the NO-GO decision for promotion.

Against the no-bytes lane:

```text
token: 101.2 ->  98.3 instr/op  (-2.9)
span : 103.2 -> 100.2 instr/op  (-3.0)
```

The SOA layout does reduce instructions, but it missed both gates:

```text
required: >= 4 instr/op win
actual:   ~3 instr/op win

required: token-soa <= 96 instr/op
actual:   token-soa ~= 98.3 instr/op
```

Throughput moved in the right direction for token, but span was noisy and can
regress. The lane is useful as a speed-ceiling probe. It is not strong enough to
promote or treat as a decisive HZ11 step.

## Decision

Keep the sibling lanes for reproducibility:

```text
HZ11_CACHE_SOA=1
libhz11_soa.so
libhz11_span_soa.so
```

Do not promote to default policy. The next speed box should target a larger or
more reliable remaining cost.

## Next Candidate

Open:

```text
HZ11SizeTableStaticInit-L1
```

Reason: the remaining attribution still includes the runtime
`size_table_ready` check in the malloc hit path. Removing it by static table
initialization or constructor-only initialization is estimated around
`~4 instr/op`, which is now larger than the observed SOA gain.
