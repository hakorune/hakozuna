# HZ9 SegmentEntry L1

`SegmentEntry` is the first step after `SegmentLocalCache` route proofs.

## Purpose

```text
SegmentLocalCache:
  TLS-only proof scaffold
  excellent fused local body
  unsafe as public allocator substrate without global routing

SegmentEntry:
  global routeable segment pages
  exact pointer VALID / interior INVALID boundary
  local fused-body target preserved for later behavior wiring
```

## Current Scope

```text
implemented:
  debug allocation/free API
  global page scan route
  exact pointer validity
  interior pointer invalid
  double free invalid
  per-class active page cursor

not implemented:
  public h8_malloc/h8_free entry
  remote pending protocol
  owner exit integration
  RSS/cache policy
  default promotion
```

## Smoke Gate

```bash
make -C hakozuna-hz9 smoke-hz9segmententry
```

The smoke checks all medium classes:

```text
alloc exact pointer:
  route VALID

interior pointer:
  route INVALID
  free rejected with owned=true

exact free:
  route becomes INVALID

double free:
  rejected with owned=true

reuse:
  next alloc returns the freed slot
```

## Next Step

```text
1. Add a focused local bench for SegmentEntry fused alloc/free.
2. Compare against SegmentLocalCache pairfused and public route modes.
3. Only then consider H9_LOCAL_ENTRY_SPLIT_L1 integration.
```

Do not wire public entry before route ownership and owner-drain semantics are
explicit.

## Initial Bench Read

```bash
make -C hakozuna-hz9 bench-hz9segmententry
```

R1 class sweep:

```text
route free, touch=1:
  about 161-181M ops/s

fused body, touch=1:
  about 239-275M ops/s

route free, touch=0:
  about 197-211M ops/s

fused body, touch=0:
  about 349-374M ops/s
```

After moving bench mode selection out of the inner loop, the fused body is
confirmed as the useful local proof:

```text
fused body:
  touch=1: about 522-628M ops/s
  touch=0: about 553-614M ops/s

active-fast mode:
  does not beat fused consistently
  class sweep showed about 352-617M ops/s with a 24K touch=1 outlier
```

Interpretation:

```text
global routeable SegmentEntry is safe enough to benchmark, but not yet at
TLS-only SegmentLocalCache fused speed.

The next optimization target is not an active-page lookup shortcut. The useful
substrate remains the fused known-slot body:
  keep exact routeable ownership
  keep public free route separate
  avoid adding active-fast branches unless a later bench proves a stable win
```
