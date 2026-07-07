# HZ11PublicEntryFastSlowHelpers-L0

Status: GO cleanup box.

## Motivation

`src/hz11_public_entry.c` duplicated the malloc/free body:

```text
HZ11_TLS_FASTPATH=1:
  hz11_malloc_fast_with_tc / hz11_free_fast_with_tc helpers

HZ11_TLS_FASTPATH=0:
  near-identical body directly inside hz11_malloc / hz11_free
```

This makes future public-entry work harder to review. This cleanup makes the
`with_tc` helpers common to all lanes, so `hz11_malloc` and `hz11_free` are mostly
routing:

```text
resolver / TLS lookup / slow init
  -> common malloc/free body with H11ThreadCache*
```

## Risk

Medium. This touches public malloc/free hot-path code generation. The helper
stays `static inline`, but LTO may still choose different register allocation or
layout. Treat any significant attribution movement as a reason to revert or
split more conservatively.

## Verification

```text
make clean
make smoke preload preload-span preload-token-tlsfast preload-span-tlsfast \
  preload-token-nobytes preload-span-nobytes preload-token-soa preload-span-soa \
  hz11-standalone-check
RUNS=5 ITERS=10000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
git diff --check
```

## Result

The cleanup preserved the existing speed lanes. The fixed64 attribution lane
remained in line with the pre-cleanup SOA result:

```text
row      lane              ops/s median   instr/op   cycles/op
fixed64  hz11-token-soa    159.14M        98.4       25.6
fixed64  hz11-span-soa     164.91M        100.4      24.6
```

The source change is therefore accepted as cleanup only. It does not claim a
new optimization result.
