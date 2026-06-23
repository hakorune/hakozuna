# MallocOwnerLoadDefer-L1

Date: 2026-06-23

## Scope

Defer `ctx->owner` load on release malloc active-hit success.

Debug builds still load owner before active-hint validation, preserving the
existing debug check:

```text
span class
owner slot
owner generation
span state
```

Release builds now load owner only when the active span misses, is exhausted,
or the slow path is needed.

## Code Shape

Before this box, release `h8_malloc_inner` loaded:

```text
owner = ctx->owner
span = ctx->active_spans[class]
```

before attempting active-hit allocation.

After this box, the active-hit success path is:

```text
ctx TLS load
class lookup
active_spans[class] load
freelist/bump allocation
return
```

The first `ctx->owner` load appears only at the active exhausted / miss path.

## Checks

```text
make bench-release smoke safety-stress
./h8_smoke
./h8_safety_stress
```

Result: pass.

Focused RUNS=3 checks:

```text
guard/local0:
  median 280.00M ops/s
  p25    280.00M ops/s
  min    251.33M ops/s
  steady median 399.29M ops/s

small_interleaved_remote90:
  median 56.28M ops/s
  p25    56.28M ops/s
  min    47.37M ops/s
  steady median 59.00M ops/s
```

Local R3 was noisy, but the code-shape target was achieved and both lanes stay
above bring-up gates.

## Decision

Keep the change.

It removes a fixed active-hit load in release builds without weakening debug
validation or changing ownership / remote protocols.
