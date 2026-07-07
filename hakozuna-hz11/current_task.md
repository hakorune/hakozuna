# HZ11 Current Task

```text
Active box:
  HZ11CacheByteAccountingGate-L1

Goal:
  keep HZ11 as a speed-first research line and measure the remaining local
  malloc/free fixed cost against tcmalloc with small reversible sibling lanes.

Do not do yet:
  claim HZ11 beats tcmalloc
  promote no-bytes lanes to default without an RSS-cap replacement
  add checked-mode diagnostics to the default path
  claim HZ11 replaces HZ8 or HZ10
```

## Restart Pointers

```text
README.md
docs/README.md
docs/HZ11_TLS_FAST_PATH_L1.md
docs/HZ11_CACHE_BYTE_ACCOUNTING_GATE_L1.md
docs/HZ11_NO_GO_LEDGER.md
```

## Current Result

```text
HZ11TLSFastPath-L1:
  GO. fixed64 token 125.5 -> 114.5 instr/op; span 131.5 -> 116.5.

HZ11CacheByteAccountingGate-L1:
  GO as speed ceiling, not default policy.
  fixed64 RUNS=10/ITERS=20M:
    token tlsfast 114.3 -> nobytes 101.3 instr/op
    span  tlsfast 116.3 -> nobytes 103.2 instr/op
  Remaining gap is about +14 to +16 instr/op vs tcmalloc.
```

## Next Step

Open a new box only after reviewing the no-bytes tradeoff. Candidate directions:

```text
1. Low-cost RSS cap replacement for no-bytes lanes
2. Remaining body attribution after no-bytes
3. Transfer-cache/span backend shape, if speed-first architecture moves forward
```
