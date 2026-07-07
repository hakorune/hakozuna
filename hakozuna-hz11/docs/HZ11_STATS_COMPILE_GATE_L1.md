# HZ11 Stats Compile Gate L1

Status: implemented + measured. Small GO as a cleanup, NO-GO as the main
tcmalloc-gap lever.

## Why this box

`HZ11FrontEndAttribution-L0` measured the fixed64 gap as instruction count:

```text
tcmalloc      87.3 instr/op
hz11-token   129.4 instr/op
hz11-span    135.4 instr/op
```

The attribution ranked hot-path counter increments as the largest single
candidate. This box tests that by compiling hot counters out of the default
speed lane while keeping diagnostic sibling builds with counters enabled.

## Mechanism

```text
HZ11_ENABLE_HOT_COUNTERS:
  0 default, speed lane
  1 diagnostic stats lane

speed libraries:
  libhz11.so
  libhz11_span.so

stats libraries:
  libhz11_stats.so
  libhz11_span_stats.so
```

Counter macros are compile-time only:

```text
HZ11_COUNT_INC(x)
HZ11_COUNT_ADD(x, n)
```

When `HZ11_ENABLE_HOT_COUNTERS=0`, the macros compile to `((void)0)`. There is
no runtime branch in the hot path.

## Dump Contract

`HZ11_DUMP_STATS=1` still works in every lane.

```text
speed lane:
  hot counters dump as zero by design
  cold/global counters such as span_create_count may remain meaningful

stats lane:
  hot counters dump normally
```

Smoke tests assert both contracts.

## Measurement

Fixed64, `ITERS=10000000`, ops/s as 3-run median, perf counters as 3-run
average:

```text
allocator             ops/s      instr/op   cycles/op   IPC
tcmalloc              186.4M       87.3       22.6      3.86
hz11-token speed      138.2M      125.4       30.0      4.18
hz11-token stats      136.7M      129.4       29.6      4.36
hz11-span speed       143.9M      131.4       27.6      4.77
hz11-span stats       139.7M      135.4       28.6      4.74
```

Delta:

```text
token: -4 instr/op, +1% ops/s
span : -4 instr/op, +3% ops/s
```

Objdump showed the expected static drop:

```text
hz11_malloc: -4 instructions
hz11_free  : -6 instructions
```

## Verdict

The compile gate works and should stay: the speed lane no longer pays for
diagnostic hot counters.

It is not the main tcmalloc-gap lever. The attribution estimate of 12-15
instr/op was too high because GCC/LTO had already lowered each counter update
to compact `incq`-style code. The measured recovery is about 4 instr/op.

Remaining fixed64 gap:

```text
tcmalloc         87.3 instr/op
hz11-token speed 125.4 instr/op  (+38.1)
hz11-span speed  131.4 instr/op  (+44.1)
```

Next target: `HZ11CacheShape-L1`, because pop/push and cache layout are now the
largest named cost. `HZ11TLSFastPath-L1` is the next candidate after cache
shape.

## GO / NO-GO

```text
GO:
  speed and stats smoke pass
  hot counters compile out without runtime branches
  instr/op drops by about 4
  ops/s improves modestly

NO-GO as primary lever:
  target 115 instr/op was not reached
  most of the tcmalloc gap remains
```

