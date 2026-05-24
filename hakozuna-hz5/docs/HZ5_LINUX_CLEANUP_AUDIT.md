# HZ5 Linux Cleanup Audit

This note records cleanup opportunities for the Linux HZ5 front-end work.
It is intentionally conservative: do not remove diagnostic lanes until their
measurements are preserved and the route matrix says they are obsolete.

## Current Module Shape

```text
smallfront/
  SmallFront-S1, ordinary malloc <=2KiB

midfront/
  MidFront-M1, ordinary malloc 2049..65536

largefront/
  LargeFront-L1/L2 candidates, ordinary malloc 65537..1MiB

ownerhub/
  cross-front drain experiments, R1/R2/R3

preload/
  LD_PRELOAD API adapter and ownership dispatch

linux/build_linux_hz5_standalone.sh
  build flag surface for standalone, preload, and candidate lanes
```

## Priority 1: Keep Lane Presets Small

The build script has many diagnostic flags. That is acceptable for research,
but paper-facing or broad comparison runs should use short presets.

Current useful preset:

```text
--linux-hz5-general-region-outbox
```

This expands to:

```text
SmallFront remote outbox cap8
MidFront rb16 + owner-fast
LargeFront owner-fast + region-map
```

Rule:

```text
Use presets for comparisons.
Use individual flags only for focused A/B work.
```

## Priority 2: Do Not Prematurely Share Hot Paths

SmallFront, MidFront, and LargeFront repeat similar patterns:

```text
owner token
local cache push/pop
remote publish list
remote drain budget
ownerhub_drain_some
state transition
map lookup
```

Do not collapse these into one generic allocator front-end yet. The object
shapes and fail-closed checks are different:

```text
SmallFront:
  page + slot-state array
  slot-boundary check
  object pointer is the free-list node

MidFront:
  one object per page/run span
  page-map descriptor lookup
  span state transition

LargeFront:
  retained large span
  page-map or region-map lookup
  exact base-pointer free contract
```

Safe commonization targets:

```text
owner token helpers
remote outbox/list helper patterns
build preset generation
documentation tables
```

Risky commonization targets:

```text
generic object state machine
generic descriptor lookup
generic free validation
generic remote entry payload
```

## Priority 3: Isolate Diagnostics

Keep these buildable, but do not promote them as defaults without new evidence:

```text
OwnerHub-R2/R3
LargeFront base-only
LargeFront takefirst
LargeFront rb16
LargeFront emptygate
MidFront takefirst
MidFront maskhitstop
MidFront globalrecycle
SmallFront emptygate
Local2P no-cookie/no-CAS and old A/B lanes
```

Specific caution:

```text
LargeFront base-only:
  useful root-cause diagnostic
  not a safety lane because interior pages no longer map to the descriptor

OwnerHub-R2/R3:
  useful evidence for cross-front pending work
  not broad defaults in the latest matrix
```

## Priority 4: Possible Deletion Later

Do not delete now. First preserve raw results and route-matrix decisions.

Future removal candidates:

```text
old Local2P diagnostic build aliases that are superseded by linkflags,
rssretain2048tls, and remotebatch

OwnerHub-R2/R3 if a later remote handoff layer replaces them and the raw
evidence remains in current_task.md

LargeFront base-only after region-map or a later range/radix map becomes the
accepted implementation and timeout evidence is preserved
```

## Priority 5: Next Code Cleanup Candidates

Low-risk:

```text
1. Keep `HZ5_LINUX_ROUTE_LANE_MATRIX.md` as the source of truth for lane names.
2. Add one broad comparison script preset for `hz5-general-region-outbox`.
3. Add small helper comments around diagnostic-only blocks in largefront and
   ownerhub.
```

Medium-risk:

```text
1. Factor OwnerHub front-mask/bit/counter fanout into a small table/helper.
2. Factor a typed remote outbox helper macro or header after SmallFront and
   MidFront both use the same owner/class outbox shape.
3. Add a typed range-map helper only after LargeFront region-map stabilizes.
```

High-risk:

```text
1. Replacing front-specific remote payloads with a generic RemoteEntry.
2. Merging Small/Mid/Large state transitions.
3. Removing diagnostic lanes before paper/repro docs are finalized.
```

## Current Recommendation

```text
Keep code modular by front-end for now.
Promote only build presets and documentation commonization.
Do not delete legacy diagnostics yet.
Next implementation cleanup should be additive and typed, not generic.
```

## Worker Audit Notes

Implemented immediately:

```text
preload_full:
  removed a vestigial stats registration helper; destructor-driven stats remain

preload_hybrid:
  named the exact 64K/a8192 claim constants and reused the size constant for
  realloc copy bounds
```

Deferred deliberately:

```text
ownerhub:
  front-mask, pending-bit, and counter fanout can be table-driven, but that
  should be a dedicated refactor with benchmark smoke because it touches
  cross-front remote-drain policy

Small/Mid/Large publish/drain skeleton:
  repeated shape is real, but front-specific validation and object geometry are
  still different enough that a generic RemoteEntry is not justified
```
