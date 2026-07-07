# HZ11RemainingBodyAttribution-L0

Status: attribution-only; no code changes.

## Context

`HZ11TLSFastPath-L1` and `HZ11CacheByteAccountingGate-L1` both landed as GO
speed-ceiling cleanups. The current best fixed64 local row is:

```text
tcmalloc              86.7 instr/op
hz11-token-nobytes   101.3 instr/op
hz11-span-nobytes    103.2 instr/op
```

The remaining gap is about 14 to 16 instructions per alloc/free pair. This note
records the objdump attribution that motivates the next implementation box.

## Instruction Reconciliation

For `hz11-token-nobytes`, the measured `101.3 instr/op` is consistent with the
static instruction path:

```text
13  bench loop body (alloc + touch + free)
 4  PLT calls (malloc@plt + free@plt)
 4  shim wrappers (endbr64 + jmp, twice)
44  hz11_malloc body
36  hz11_free body
---
97  static hot-path total, close to measured 101.3
```

The mismatch is small enough that the attribution is useful for choosing the
next box.

## Remaining Cost Ranking

```text
rank  component                                  cost/op     next-box value
1     class-cache address calculation             ~13        high
2     token hash on free                            ~9        structural in token lane
3     size_table_ready runtime check                ~4        medium
4     malloc callee-save frame                      ~6        partial
5     TLS guard / null / size lower-bound checks    ~9        mostly structural
6     redundant movzbl / boundary checks            ~3        small
```

The largest actionable component is the class-cache address calculation. The
current layout is array-of-structs:

```c
H11ClassCache class_cache[HZ11_CLASS_COUNT];
```

`H11ClassCache` contains `items[32]` plus `count`, making the stride 264 bytes in
the count-indexed lane. Addressing `class_cache[class_id]` therefore lowers to a
`*33` / `*264` calculation chain. That chain appears in both malloc and free.

## Token vs Span After No-Bytes

Token and span lanes are now close:

```text
hz11-token-nobytes  101.3 instr/op
hz11-span-nobytes   103.2 instr/op
```

The malloc side is effectively the same. The span free side pays arena-base load
and 4GiB range checks, while avoiding the token `0xff` class check. Net
difference is small. Classification is no longer the first lever.

## Next Box

Open:

```text
HZ11CacheLayout-L1
```

Test a structure-of-arrays layout behind a sibling flag:

```text
HZ11_CACHE_SOA=1
```

Target shape:

```c
void* class_items[HZ11_CLASS_COUNT][HZ11_CACHE_CAP];
uint32_t class_counts[HZ11_CLASS_COUNT];
```

Expected effect:

```text
class-cache address chain: *264 / *33 -> scaled index addressing
expected recovery:         ~5-6 instr/op
target:                    101.3 -> ~95-96 instr/op on token-nobytes
```

This should be measured as another speed-ceiling sibling, not promoted to
default policy automatically.

## Follow-On Ladder

If `HZ11CacheLayout-L1` lands near expectation, the remaining known medium
levers are:

```text
1. HZ11SizeTableStaticInit-L1
   Remove the runtime size_table_ready check from the hit path.

2. HZ11RefillTailCall-L1
   Reduce malloc callee-save pressure caused by refill followed by token_set.
```

Together with cache layout, those could bring the instruction count into the
high-80s / low-90s range, near the current tcmalloc fixed64 instruction count.
