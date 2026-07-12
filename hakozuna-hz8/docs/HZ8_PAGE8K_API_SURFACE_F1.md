# HZ8 Page8K API Surface F1

## Purpose

The R3 page8K sibling originally connected only `malloc` and `free`. Its
benchmarks disabled `realloc`, and the common HZ8 route and usable-size entry
points did not consult the page8K registry. This box closes that API surface
before any class-wide expansion.

## Contract

```text
page8K registry hit + exact allocated slot:
  route       -> VALID
  usable_size -> 8192

page8K registry hit + interior, stale, duplicate, or pending slot:
  route       -> INVALID
  usable_size -> owned=true, failure
  free        -> owned=true, failure

page8K registry miss:
  route       -> MISS
  usable_size/free -> owned=false, normal fallback
```

The page registry is classification evidence. Exact slot alignment and the
atomic slot state remain the authority. An owned-looking invalid pointer must
never reach CRT/system fallback.

## Realloc

An R3 page pointer is never passed to system `realloc`.

```text
new size == 8192:
  return the original pointer

other new size:
  allocate through HZ8
  copy min(old_usable_size, new_size) bytes
  free through HZ8 page adapter
```

Allocation failure leaves the original pointer live. A page-owned invalid
pointer returns `EINVAL` and remains non-reusable.

## Scope and Gates

This is an opt-in R3 safety/completeness fix. It does not change `hz8-v2`,
add production counters, or widen page geometry.

Required checks:

- exact route VALID and usable size 8192;
- interior and duplicate route INVALID;
- same-size realloc preserves the pointer;
- cross-size realloc copies data and releases the old page slot;
- failed realloc preserves the original allocation;
- owner-close/adoption, remote drain, residency, and safety smokes remain zero;
- default and non-R3 builds remain unchanged.

## Verification Snapshot

The F1 API smoke passed on both supported development surfaces:

```text
Linux/WSL build:
  page8k api smoke: PASS
  existing page8K smoke: PASS
  page8K safety stress: PASS
  default HZ8 smoke: PASS

Windows build:
  page8k api smoke: PASS
  remote smoke: PASS
  remote stress: claims=80000, drained=80000, depth_max=2
  lifecycle smoke/stress: close=1000, adopt=1000, drained=8000
  residency smoke: decommit=16, recommit=16
  thread turnover: close=1000, adopt=999
```

The Windows suite still reports pre-existing compiler warnings in unrelated
HZ8/HZ10 sources; the F1 API smoke itself builds and passes. These checks do
not promote the R3 row or change the public `hz8-v2` default.

Promotion remains blocked until native-platform and application-like gates
are rerun. After this box, the next performance experiment may evaluate
`4097..8192` requests on the unchanged 8KiB page geometry.

The follow-up was isolated as `H8_MEDIUM_PAGE8K_RANGE4097_L1`. It remains an
opt-in evidence lane only: requests from 4097 through 8192 bytes use the
existing eight-slot, 64KiB page8K run, and every other size keeps its prior
route. The range lane has diagnostic-only eligible/served allocation fields;
no counter or atomic is added to the public speed lane. Its focused Windows
result was about 12.7% below HZ8 v2, so the range expansion is correctness GO
but speed-candidate NO-GO.
