# AllocInlineShape-L1

Date: 2026-06-23

## Change

Split the small allocation helper into a pure fast helper:

```text
h8_small_alloc_from_span(span)
```

The helper no longer performs pressure collection internally.  When an active
span is full, `h8_malloc_inner()` performs the same pressure collection before
continuing to the slow scan path.

## Code Shape

Before this box, the active-hit malloc path called:

```text
h8_small_alloc_from_span.constprop.0
```

After this box, the active-hit helper call is gone from `h8_malloc_inner`.
Remaining calls in the malloc leaf are slow-path calls:

```text
h8_thread_ctx_get_slow
h8_pressure_owner_collect
h8_find_active_span
h8_span_commit_for_class
h8_orphan_adopt_span
```

Raw code-shape log:

```text
bench_results/20260623T102400Z_alloc_inline_code_shape.log
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
  median ~= 340.1M ops/s
  p25 ~= 305.2M ops/s
  steady median ~= 385.9M ops/s

small_interleaved_remote90 RUNS=5:
  median ~= 57.1M ops/s
  p25 ~= 55.3M ops/s
  steady median ~= 59.2M ops/s
  zero gates clean
```

Raw logs:

```text
bench_results/20260623T102400Z_alloc_inline_local_r5.log
bench_results/20260623T102400Z_alloc_inline_interleaved_r5.log
```

## Decision

`AllocInlineShape-L1` is accepted.  It removes an active-hit call without
changing remote protocol or ownership semantics.
