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

direct-page mode:
  prepares the active page once and cycles by page id
  touch=1: about 566-674M ops/s
  touch=0: about 584-631M ops/s

opaque-handle mode:
  prepares an opaque page handle once and cycles by handle
  touch=1: about 643-771M ops/s
  touch=0: about 724-818M ops/s

tls-handle mode:
  caches the selected page handle in TLS by class
  R1 class sweep:
    touch=1: about 531-654M ops/s
    touch=0: about 693-731M ops/s
  class 64K, 5M-iteration repeat:
    touch=1: about 692-710M ops/s
    touch=0: about 711-721M ops/s

tlsroute mode:
  allocates through the TLS cached handle, then frees through route/free
  touch=1: about 244-255M ops/s
  touch=0: about 251-265M ops/s
  compared with route at about 182-207M touch=1 and 199-226M touch=0

tlslocal mode:
  allocates through the TLS cached handle, then frees through cached-page local
  free
  R1 class sweep:
    touch=1: about 283-311M ops/s
    touch=0: about 261-296M ops/s
  class 64K, 5M-iteration repeat:
    tlsroute touch=1: about 238-243M ops/s
    tlslocal touch=1: about 295-307M ops/s
    tls cycle touch=1: about 568-570M ops/s
```

Interpretation:

```text
global routeable SegmentEntry is safe enough to benchmark, but not yet at
TLS-only SegmentLocalCache fused speed.

The next optimization target is not an active-page lookup shortcut. The useful
substrate remains the fused known-slot body. Direct-page mode shows that a
routeable page body can still gain from keeping the page handle out of the
inner loop. Opaque-handle and TLS-handle modes strengthen that result:
  keep exact routeable ownership
  keep public free route separate
  avoid adding active-fast branches unless a later bench proves a stable win
  prefer entry designs that cache the selected page handle at the local call
  site
  treat TLS cached page handle as the next behavior integration shape
  handle-cached allocation alone is not enough; the next residual is free route
  cached-page local free helps, but still pays pointer-to-slot decode and state
  validation; known-slot local free remains the upper-bound body
```
