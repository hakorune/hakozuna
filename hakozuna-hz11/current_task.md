# HZ11 Current Task

```text
Active box:
  none

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
docs/HZ11_REMAINING_BODY_ATTRIBUTION_L0.md
docs/HZ11_CACHE_LAYOUT_L1.md
docs/HZ11_SYS_RESOLVER_SPLIT_L0.md
docs/HZ11_TOKEN_HELPERS_SPLIT_L0.md
docs/HZ11_PUBLIC_ENTRY_HELPERS_L0.md
docs/HZ11_SIZE_TABLE_STATIC_INIT_L1.md
docs/HZ11_STATIC_CONST_SIZE_TABLE_L1.md
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

## Recent Results

```text
HZ11CacheLayout-L1:
  NO-GO for instruction-count gate.
  token no-bytes 101.2 -> SOA 98.3 instr/op (~3 win, target >=4)
  token ops/s improved, so keep as speed-ceiling sibling.

HZ11PublicEntryFastSlowHelpers-L0:
  GO cleanup. Common malloc/free with-thread-cache helper body is shared by
  TLS and non-TLS lanes.
  fixed64 attribution preserved the speed-ceiling lanes:
    token-soa 159.14M ops/s, 98.4 instr/op
    span-soa  164.91M ops/s, 100.4 instr/op

HZ11SizeTableStaticInit-L1:
  NO-GO. The lazy-init guard in hz11_size_class() is load-bearing under
  LD_PRELOAD because ld.so can call malloc before constructors run. Removing
  the guard classifies all sizes as class 0 from the BSS-zeroed table and
  corrupts the heap. Kept exhaustive 1..65536 class-map smoke coverage.

HZ11RefillTailCall-L1:
  NO-GO. Static hz11_malloc body shrank, but fixed64 cache-hit throughput did
  not improve because refill is a miss path and the measured row is hit-heavy.
  Reverted the source change.

HZ11StaticConstSizeClassTable-L1:
  NO-GO. A const .rodata table is correct and avoids loader-time BSS-zero class
  corruption, but fixed64 token-soa regressed about 7%. Reverted the source
  change and kept only the NO-GO record.
```

## Next Step

```text
Candidate speed box:
  structural design box, not another small cleanup.
  Candidate: HZ11DirectEntryOrPerCpuFrontEnd-Design-L0

Candidate cleanup:
  no active cleanup box

Recently completed cleanup:
  split system allocator resolver
  split token table helpers
  consolidate duplicated malloc/free with-thread-cache bodies in
  hz11_public_entry.c
```
