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
