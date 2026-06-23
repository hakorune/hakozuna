# FreeLookupInlineP2-L1

Date: 2026-06-23

## Change

`h8_free_inner()` now performs the initial HZ8 span lookup inline after the
existing ready and arena checks:

```text
span table acquire load
retired-state check
slot alignment / range check
```

`h8_span_from_ptr_checked()` remains available for other callers.  Remote
publish and owner protocol are unchanged.

## Code Shape

Before this box, the free hot path called:

```text
h8_span_from_ptr_checked@PLT
```

After this box, that call is gone.  Remaining calls in `h8_free_inner()` are
remote/slow paths:

```text
h8_remote_free_publish_known
sched_yield
h8_remote_free_publish
h8_thread_ctx_get_slow
h8_orphan_owner
```

Raw code-shape log:

```text
bench_results/20260623T103100Z_free_lookup_inline_code_shape.log
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
  median ~= 345.8M ops/s
  p25 ~= 341.5M ops/s
  steady median ~= 406.8M ops/s

small_interleaved_remote90 RUNS=5:
  median ~= 57.0M ops/s
  p25 ~= 56.9M ops/s
  steady median ~= 59.4M ops/s
  zero gates clean
```

Raw logs:

```text
bench_results/20260623T103100Z_free_lookup_inline_local_r5.log
bench_results/20260623T103100Z_free_lookup_inline_interleaved_r5.log
```

## Decision

`FreeLookupInlineP2-L1` is accepted.  It removes a free-leaf call without
changing pointer identity semantics or remote ownership protocol.
