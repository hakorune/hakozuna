# ThreadCtxOwnerInvariant-L1

Date: 2026-06-23

## Change

After `h8_thread_ctx_fast()` returns a non-null context, malloc/free now treats
`ctx->owner` as a required invariant instead of falling back to
`h8_orphan_owner()`.

The invariant is established by `h8_thread_ctx_new()`:

```text
owner = h8_owner_free_stack_pop()
if owner is NULL:
  no context is created
ctx->owner = owner
```

During thread shutdown, `h8_tls_ctx` is cleared before owner exit and context
free.

## Code Shape

Before this box, malloc/free leaf carried call edges to:

```text
h8_orphan_owner
abort
```

After this box, those call edges are gone from the release malloc/free leaf.

Raw code-shape log:

```text
bench_results/20260623T104000Z_owner_invariant_code_shape.log
```

## Verification

```text
make bench-release preload smoke safety-stress preload-smoke
./h8_smoke
./h8_safety_stress
LD_PRELOAD=$PWD/libhakozuna_hz8_preload.so /bin/true
```

All passed.

Focused release checks:

```text
guard/local0 RUNS=5:
  median ~= 395.1M ops/s
  p25 ~= 391.0M ops/s
  steady median ~= 462.8M ops/s

small_interleaved_remote90 RUNS=5:
  median ~= 57.4M ops/s
  p25 ~= 57.0M ops/s
  steady median ~= 59.5M ops/s
  zero gates clean
```

Raw logs:

```text
bench_results/20260623T104000Z_owner_invariant_local_r5.log
bench_results/20260623T104000Z_owner_invariant_interleaved_r5.log
```

## Decision

`ThreadCtxOwnerInvariant-L1` is accepted.  It removes defensive owner fallback
call edges without changing owner lifecycle or remote protocol.
