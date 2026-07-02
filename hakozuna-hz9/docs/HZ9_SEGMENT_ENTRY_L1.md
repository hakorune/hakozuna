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

## Source Layout

```text
h8_hz9_segment_entry.c:
  core routeable page scaffold
  exact pointer route/free
  fused and checked page bodies

h8_hz9_segment_entry_cache.c:
  TLS cache / ledger debug bodies
  one-entry cache state experiments
  ledger-body attribution

h8_hz9_segment_entry_internal.h:
  private shared page state
  internal helper declarations only
```

The split is behavior-preserving. Its purpose is to keep the active
SegmentEntry source files below the 800-line rule before adding the next
fused-body experiment.

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

handlecheckedtouch mode:
  prepares an opaque page handle once, then runs the checked-touch body without
  per-iteration class/TLS lookup
  handlebody is the same checked-touch body called through the internal inline
  helper from the bench TU; it tests whether the public debug wrapper boundary
  is the next bottleneck.
  class 64K, 5M-iteration R3:
    handlecheckedtouch touch=1: about 523-570M ops/s
    tlscheckedtouch touch=1: about 415-417M ops/s
    tlsepochbody touch=1: about 350-361M ops/s
  R1 class sweep, 3M iterations:
    handlecheckedtouch touch=1: about 494-564M ops/s
    tlscheckedtouch touch=1: about 410-416M ops/s
  focused R3 sweep:
    command: RUNS=3 ITERS=3000000
             scripts/run_hz9_segment_entry_handle_probe.sh
    handlecheckedtouch: about 521-572M ops/s
    tlscheckedtouch: about 394-422M ops/s
    tlsepochbody: about 349-360M ops/s
    tlsroute64body: about 284-290M ops/s
  direct body check, class 64K, 10M-iteration R3:
    handlebody: about 548-562M ops/s
    handlecheckedtouch: about 562-575M ops/s
  reading: removing the public debug wrapper boundary does not improve the
  acquired-handle body. The remaining large gap is the handle/TLS acquisition
  shape, not a wrapper-call artifact.
  TLS acquisition split, class 64K, 5M-iteration R3:
    handlebody: about 514-547M ops/s
    tlsbody: about 506-522M ops/s
    tlsbodychecked: about 451-467M ops/s
    tlscheckedtouch: about 375-383M ops/s
  reading: a raw TLS handle load is close to acquired-handle speed. The
  material loss starts when the hot body also performs page/class/free checks,
  and the public debug wrapper/fallback shape adds more loss. HZ9 local reuse
  should receive a prevalidated handle or epoch token and keep route/class
  checks at acquisition/retirement boundaries.
  generation guard check, class 64K, 5M-iteration R3:
    handlebody: about 650-655M ops/s
    handleguardbody: about 520-565M ops/s
    tlsbody: about 598-610M ops/s
    tlsguardbody: about 467-497M ops/s
    tlsbodychecked: about 503-528M ops/s
  reading: even a compact generation guard is too expensive for the local reuse
  body. The generation/token check belongs at handle acquisition and retirement;
  the hot loop should run on an already-validated page handle.

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
  after removing duplicate slot decode in route/free, class 64K 5M repeat:
    route touch=1: about 253-259M ops/s
    tlsroute touch=1: about 248-267M ops/s

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
  after route/free duplicate-decode cleanup, class 64K 5M repeat:
    tlslocal touch=1: about 271-278M ops/s

tlsknown mode:
  allocation returns the slot id, then local free uses the known slot
  R1 class sweep:
    touch=1: about 288-352M ops/s
    touch=0: about 299-350M ops/s
  class 64K, 5M-iteration repeat:
    tlslocal touch=1: about 273-283M ops/s
    tlsknown touch=1: about 307-345M ops/s
    tls cycle touch=1: about 474-566M ops/s

tlschecked mode:
  fuses the TLS cached-page cycle while retaining simple post-allocation state
  validation in the body; this mode intentionally does not payload-touch after
  the fused free
  R1 class sweep:
    touch=1: about 562-590M ops/s
    touch=0: about 573-620M ops/s
  class 64K, 5M-iteration repeat:
    tlsknown touch=1: about 315-336M ops/s
    tlschecked no-payload-touch: about 520-614M ops/s
    tls cycle touch=1: about 524-529M ops/s

tlscheckedtouch mode:
  fuses the TLS cached-page cycle, checks state, and payload-touches inside the
  local body before returning the slot to the free mask
  R1 class sweep:
    touch=1: about 436-471M ops/s
    touch=0: about 430-497M ops/s
  class 64K, 5M-iteration repeat:
    tlscheckedtouch touch=1: about 399-464M ops/s
    tls cycle touch=1: about 609-613M ops/s

tlsepochbody mode:
  adds an active-page/epoch-style pointer check before the same checked-touch
  body
  class 64K, 5M-iteration R3:
    tlscheckedtouch touch=1: about 435-443M ops/s
    tlsepochbody touch=1: about 318-359M ops/s
    tlsroute64body touch=1: about 289-294M ops/s

tlsroutebody mode:
  fuses the TLS cached-page cycle, payload-touches, then performs exact
  global route lookup while the slot is still allocated before returning it
  to the local free mask
  class 64K, 5M-iteration R3:
    tlscheckedtouch touch=1: about 436-444M ops/s
    tlsroutebody touch=1: about 257-261M ops/s
    tlsroute touch=1: about 249-251M ops/s
    route touch=1: about 216-236M ops/s

tlsrouteevery / tlsroute64body modes:
  keep the fused page body but sample exact global route instead of checking on
  every local reuse
  class 64K, 5M-iteration R3:
    tlscheckedtouch touch=1: about 435-445M ops/s
    tlsrouteevery period=4: about 266-271M ops/s
    tlsrouteevery period=16: about 286-288M ops/s
    tlsrouteevery period=64: about 284-300M ops/s
    tlsrouteevery period=0: about 318-347M ops/s
    tlsroute64body constant-period: about 286-316M ops/s

tlscache mode:
  models a one-slot TLS object cache with `LOCAL_CACHE`-like state:
    cached slot is not route-VALID
    double free through route/free is rejected
    alloc can pop the cached object, but free pushes through exact pointer route
  R1 class sweep:
    touch=1: about 163-229M ops/s
    tlscheckedtouch comparison: about 411-493M ops/s
  class 64K, 5M-iteration repeat:
    tlscache touch=1: about 224-234M ops/s
    tlsroute touch=1: about 237-255M ops/s
    tlscheckedtouch touch=1: about 486-495M ops/s

tlsledger mode:
  models a same-thread allocation ledger: alloc records page/slot, and free
  pushes back to the local cache without exact pointer route
  R1 class sweep:
    touch=1: about 300-330M ops/s
    tlscache comparison: about 214-226M ops/s
    tlscheckedtouch comparison: about 443-472M ops/s
  class 64K, 5M-iteration repeat:
    tlsledger touch=1: about 298-328M ops/s
    tlscache touch=1: about 198-211M ops/s
    tlscheckedtouch touch=1: about 423-430M ops/s

tlsledgerbody mode:
  fuses the one-entry ledger pop/push into one debug body to test whether
  helper/API splitting is the primary cost
  R1 class sweep:
    touch=1: about 253-345M ops/s
    tlsledger comparison: about 301-331M ops/s
    tlscheckedtouch comparison: about 376-442M ops/s
  class 64K, 5M-iteration repeat:
    tlsledgerbody touch=1: about 300-312M ops/s
    tlsledger touch=1: about 305-329M ops/s
    tlscheckedtouch touch=1: about 400-431M ops/s
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
  known-slot free helps, but split alloc/free debug API still leaves a large
  gap to fused tls cycle
  tlschecked shows that simple state validation is not the blocker when it is
  fused into the local entry body; the blocker is the split alloc/free API shape
  tlscheckedtouch is the fairer payload-touch body and shows the remaining body
  cost once the touch happens before the fused free
  removing duplicate slot decode from route/free cleans up the boundary, but it
  does not close the fused-body gap
  tlsroutebody shows that even a fused helper cannot keep speed if exact global
  route lookup is in the local hot body; route must be a boundary operation, not
  a per-local-reuse operation
  route sampling improves over checking every iteration, but even a constant
  64-cycle sampling body remains far below tlscheckedtouch; HZ9 should avoid
  per-allocation route sampling in the local hot body and instead attach route
  authority at coarser ownership/epoch boundaries
  tlsepochbody shows that even cheap active-page/epoch checks are too expensive
  if they are paid on every local reuse; validate the local segment at
  acquisition/retirement boundaries and keep the local body fused
  handlecheckedtouch restores most of the fused body by passing an acquired
  segment handle directly; this is the best next behavior shape:
    acquisition validates route/owner/epoch
    local reuse receives a stable segment handle
    release/retire reattaches public routing authority
  run_hz9_segment_entry_handle_probe.sh is the reproducible gate for this
  decision; keep future handle-bound behavior candidates compared against it
  tlscache keeps fail-closed cached-slot semantics, but tying every local free
  back through exact pointer route collapses to route-free speed; a HZ9 cache
  cannot be just a TLS pop plus public route push
  tlsledger proves that removing route-push helps, but a one-entry ledger still
  pays enough bookkeeping that it does not recover the fused local body
  tlsledgerbody shows helper splitting is not the main blocker; the ledger/cache
  state shape itself is too heavy for the local hot body
  the next behavior design should preserve a fused local body and then reattach
  public free routing at the boundary, not insert another route shortcut inside
  the hot body
```
