# HZ11 Cache Shape L1

Status: implemented + measured. NO-GO.

## Why this box

`HZ11StatsCompileGate-L1` removed hot counter increments from the default speed
lane, but recovered only about 4 instr/op. The next named candidate was cache
pop/push layout.

This box tests whether changing `H11ClassCache` from count-indexed push/pop to
a moving top pointer reduces fixed-size local-path instruction count.

## Mechanism

Build flag:

```text
HZ11_CACHE_TOPPTR:
  0 default, count-indexed cache
  1 pointer-top cache A/B lane
```

Sibling libraries:

```text
libhz11_top.so
libhz11_span_top.so
```

The default `libhz11.so` and `libhz11_span.so` remain the rollback lanes.

The top-pointer lane keeps the existing `cached_bytes` and cap behavior. This
keeps the A/B about pop/push shape rather than silently changing the RSS/cache
contract.

## Measurement

Fixed64, `RUNS=5`, `ITERS=10000000`:

```text
allocator          ops/s      instr/op   cycles/op
tcmalloc           190.80M      87.4       22.7
hz11-token         138.03M     125.5       29.7
hz11-token-top     140.75M     138.5       30.5
hz11-span          147.76M     131.5       27.4
hz11-span-top      141.38M     143.5       28.8
```

Auxiliary fixed16/256/4096 runs showed the same instruction-count pattern:

```text
token-top: +13 instr/op vs token
span-top : +12 instr/op vs span
```

Objdump also moved in the wrong direction:

```text
token hz11_malloc      116 -> 123
token hz11_free         75 -> 80
span  hz11_malloc       80 -> 86
span  hz11_free         89 -> 95
```

## Verdict

NO-GO.

Pointer-top did not reduce instruction count. It increased static and dynamic
instruction count, likely because it adds a stored top pointer and still must
preserve cached-byte accounting and cap checks. Token-top sometimes improves
wall-clock throughput slightly, but the primary gate for this box was
instruction count, and it moved the wrong way. Span-top regressed both
instruction count and fixed64 throughput.

Do not continue this cache-shape direction without a different design and new
evidence. The next largest remaining candidate is `HZ11TLSFastPath-L1`.

## GO / NO-GO

```text
GO target:
  fixed64 token instr/op 125.5 -> about 115
  ops/s improves
  smoke green
  objdump instruction count drops

Result:
  instr/op increased to 138.5 for token-top and 143.5 for span-top
  objdump instruction count increased
  smoke stayed green
  gate failed
```

