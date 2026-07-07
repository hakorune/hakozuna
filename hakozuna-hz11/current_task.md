# HZ11 Current Task

```text
Active box:
  HZ11MacroSpeedLaneGate-L1 (measured NO-GO)

Goal:
  keep HZ11 as a speed-first research line. The transfer lane is now the
  recommended speed-lane candidate on the micro remote/mixed matrix, but macro
  promotion is blocked by correctness and RSS failures.

Do not do yet:
  claim HZ11 generally beats tcmalloc
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
docs/HZ11_TRANSFER_CACHE_CENTRAL_SPAN_L1.md
docs/HZ11_TRANSFER_PROMOTION_MATRIX_L1.md
docs/HZ11_MACRO_SPEED_LANE_GATE_L1.md
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

HZ11TransferPromotionMatrix-L1:
  gate script added and default gate measured:
    hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh
  Default gate output:
    bench_results/hz11_transfer_promotion_20260707T220243Z/summary.md
  Conditions:
    RUNS=10 THREADS=16 ITERS=100000
    rows: main_local0, main_r50, main_r90, small_remote90, medium_r50, medium_r90
    allocators: tcmalloc, hz11-span-soa, hz11-span-transfer
  Verdict:
    GO. libhz11_span_transfer.so is the recommended HZ11 speed-lane candidate.
  Key results:
    main_r50:       transfer 83.95M vs tcmalloc 44.68M ops/s
    main_r90:       transfer 59.24M vs tcmalloc 24.29M ops/s
    small_remote90: transfer 65.39M vs tcmalloc 57.99M ops/s
    medium_r50:     transfer 76.74M vs tcmalloc 15.52M ops/s
    medium_r90:     transfer 53.80M vs tcmalloc 8.56M ops/s
    RSS is below tcmalloc on every measured row.

HZ11MacroSpeedLaneGate-L1:
  script added and RUNS=5 measured:
    hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh
  Output:
    bench_results/hz11_macro_speed_lane_20260707T222023Z/summary.md
  Verdict:
    NO-GO for macro speed-lane promotion.
  Key results:
    xmalloc_test: transfer 2.025s vs tcmalloc 2.054s, much lower RSS
    cache_scratch: transfer 1.177s vs tcmalloc 1.174s, lower RSS
    python_alloc: transfer aborts 5/5, rc=134
    mstress: transfer aborts 5/5, rc=134
    larson: transfer wall near tcmalloc, but RSS 2.344x tcmalloc
    sh6bench: transfer 13.048x tcmalloc wall and 1.336x max RSS
  Abort attribution:
    gdb confirms python_alloc and mstress both abort in
    hz11_central_stack_insert_range().
    This is intentional fail-fast on central stack capacity overflow, not a
    silent heap corruption.
```

## Next Step

```text
Candidate speed boxes:
  HZ11MacroFailureAttribution-L1:
    diagnose central stack overflow policy/cap first, then larson/sh6bench RSS.
  Keep transfer as remote/mixed microbench lane only until macro correctness is stable.

Candidate cleanup:
  no active cleanup box

Recently completed cleanup:
  split system allocator resolver
  split token table helpers
  consolidate duplicated malloc/free with-thread-cache bodies in
  hz11_public_entry.c
```
