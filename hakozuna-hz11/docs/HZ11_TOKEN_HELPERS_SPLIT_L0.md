# HZ11TokenHelpersSplit-L0

Status: cleanup box.

## Motivation

`src/hz11_thread_cache.h` contains build flags, cache layout structs, token-table
types, token hashing helpers, and cache pop/push helpers. This box moves the
token-table type and inline helpers to `hz11_token_table.h`.

## Boundary

Moved:

```text
HZ11_TOKEN_COUNT
H11Token
hz11_token_hash
hz11_token_set
hz11_token_lookup
hz11_token_invalidate
```

The helpers now take `H11Token* tokens` instead of `H11ThreadCache* tc`, so the
token module does not need to know the full thread-cache layout.

## Risk

Low to medium. The helpers remain `static inline`, but token call sites in
`hz11_public_entry.c` now pass `tc->tokens`. Check objdump/perf for accidental
codegen movement.

## Verification

```text
make clean
make smoke hz11-standalone-check
RUNS=3 ITERS=10000000 SLOTS=64 ./scripts/run_hz11_frontend_attribution.sh
git diff --check
```

## Result

Verdict: cleanup GO.

`RUNS=3 ITERS=10000000 SLOTS=64` kept the measured instruction counts stable:

```text
hz11-token-nobytes  101.4 instr/op
hz11-token-soa       98.4 instr/op
hz11-span-nobytes   103.5 instr/op
hz11-span-soa       100.4 instr/op
```

Objdump auxiliary counts moved slightly in token lanes after the include/call-site
shape changed, but the perf-derived `instr/op` did not regress. Keep the split.
