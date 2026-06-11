# HZ7 TinyRoute V2 Tasks

HZ7 v2 is the readable tiny allocator reference track. This document turns the
current goal into a concrete task list so the folder has one obvious place to
look before adding more code.

## Core Goal

```text
local-performance focused
remote-free safe
coarse-lock cross-thread free safety
route safety
low RSS
readable tiny allocator shape
```

## Current Design Decision

The current HZ7 v2 line is aligned with the latest design review:

```text
main lane:
  local small/medium performance
  low RSS
  route safety
  coarse-lock cross-thread free safety

accepted MT direction:
  SlowPathOutsideLock-L1 / LockScopeTrim-L1
  keep the global lock contract
  move OS allocation/release outside the lock only after route state is safe

remote lane:
  safety/control only
  not a throughput claim
```

This means HZ7 v2 should not grow owner-aware remote handoff, inboxes, TLS
ownership, remote batching, or lock-free queues. Those ideas belong to an
HZ6-family remote allocator or a future HZ8-style track, not this tiny
reference allocator.

The recommended active order is:

```text
1. keep SlowPathOutsideLock-L1 as the accepted default
2. keep benchmark plumbing and baseline snapshots readable
3. continue small source/list/stat helper cleanups
4. keep remote-free smoke as evidence/control
5. do not pursue remote fast path in HZ7 v2
```

## Priority 1: Must-Have

- [x] Keep local small/medium performance viable
- [x] Keep cross-thread free safe under the coarse global lock
- [x] Preserve fail-closed route safety
- [x] Keep RSS low and predictable
- [x] Keep the code readable enough to explain as a tiny reference

## Priority 2: Implementation

- [x] Keep the direct API small and explicit
- [x] Keep the small/medium hot path separate from remote safety concerns
- [x] Implement `SlowPathOutsideLock-L1` before adding remote fast paths
- [x] Move OS allocation/release work out of the global lock where route safety allows
- [x] Keep remote as safety/evidence only, not a throughput claim
- [x] Keep route/class changes tiny unless they beat the archived baseline
- [x] Fail closed on invalid or foreign-looking input

## Priority 3: Documentation

- [x] Keep the v2 goal statement in sync with the code shape
- [x] Keep the README short and direct about what v2 is and is not
- [x] Keep no-go decisions visible so repeated dead ends stay archived
- [x] Keep v1 frozen as the cute tiny baseline

## Priority 4: Evaluation

- [x] Compare small / medium / mixed together before promoting a change
- [x] Track RSS alongside throughput
- [x] Use remote only as a safety/control check
- [x] Preserve the archived no-go baseline when a new idea does not earn a win

## Archived No-Gos

```text
medium empty-span cap experiments:
  cap=2 / cap=4 / cap=8 style tweaks did not improve the local-complete shape
  they pushed v2 away from the tiny baseline without earning a stable win

route_slot_index cleanup alone:
  hygiene-only unless paired with a stronger route hot-path change

route hot cache:
  archived no-go for now; did not hold a stable repeat-5 win

SpanClassLookupTrim-L1:
  archived no-go; improved medium but regressed mixed enough to lose the win
```

## Accepted Tiny-Step

The accepted tiny step is `SlowPathOutsideLock-L1`, also called
`LockScopeTrim-L1`. Keep the coarse global lock design, but stop holding that
lock across OS allocation and OS release when the route/state transition can be
completed first.

Status:

```text
implemented and accepted as the current HZ7 v2 default path
```

Implementation order:

- [x] Split malloc into a locked fast check, an outside-lock OS allocation, and a locked commit
- [x] Prepare span/direct region metadata outside the lock before the locked commit
- [x] Recheck retained/span availability after the outside-lock allocation before committing the new region
- [x] Release any unused preallocated region outside the lock
- [x] Split free so route lookup, state transition, retained decision, and route unregister happen under lock
- [x] Run only the final OS release outside the lock
- [x] Keep retained direct and empty-span policies unchanged

Acceptance:

- [x] `hz7_smoke` passes
- [x] `hz7_remote_smoke` passes
- [x] `hz7_mt_smoke` passes
- [x] `h7_route()` still reports foreign as `MISS`, active exact as `VALID`, and interior/retained/inactive as `INVALID`
- [x] `route_register_fail = 0`
- [x] random_mixed small/medium/mixed repeat-5 does not regress materially
- [x] single-thread local performance stays within 1 percent of the current baseline
- [x] RSS stays unchanged within noise

Measured on Windows random_mixed repeat-5 after `SlowPathOutsideLock-L1`:

```text
hz7-v2:
  small   77.237M ops/s, 4,576 KB peak
  medium  17.939M ops/s, 5,040 KB peak
  mixed   19.419M ops/s, 5,504 KB peak

source:
  out_win_random_mixed_hz7v2_prepare_outside_lock_repeat5/
  20260611_151952_paper_random_mixed_windows.md
```

No-go checks:

- [x] Do not move route unregister outside the lock
- [x] Do not free an OS region while its route entry is still active
- [x] Do not add owner tokens, inboxes, TLS ownership, remote queues, or production diagnostics
- [x] Do not keep the change if the code becomes hard to explain as a tiny reference

## Next Candidate Backlog

The next source cleanup step is `EmptySpanListHelper-L1`. It is not an
allocation policy change. It makes retained empty-span pop/push/count
transitions explicit helpers so future span/free-list work has a single small
place to inspect.

```text
EmptySpanListHelper-L1:
  [x] add a helper for popping one retained empty span
  [x] add a helper for pushing one empty span when under cap
  [x] keep partial/empty span list behavior unchanged
  [x] keep empty_count accounting unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next optional cleanup after that is `SpanCommitStatsHelper-L1`. It should
only happen if the empty-span helper lands cleanly.

```text
SpanCommitStatsHelper-L1:
  [x] share span_count / reserved_bytes commit transitions
  [x] share span release accounting transitions
  [x] keep retained empty-span behavior unchanged
  [x] keep route safety unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `DirectReservedStatsHelper-L1`. It is not an
allocation policy change. It mirrors the span commit stats cleanup for direct
regions by giving direct `reserved_bytes` transitions one tiny owner while
leaving retained direct regions reserved and route-INVALID.

```text
DirectReservedStatsHelper-L1:
  [x] share direct reserved_bytes commit transitions
  [x] share direct reserved_bytes release transitions
  [x] keep retained direct regions reserved
  [x] keep retained direct route INVALID semantics unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `MallocPreallocPrepareHelper-L1`. It is not an
allocation policy change. It keeps the accepted `SlowPathOutsideLock-L1`
sequence, but gives the outside-lock region preparation step a single helper so
`h7_malloc()` reads as fast check, preallocate, locked commit, release unused.

```text
MallocPreallocPrepareHelper-L1:
  [x] add a helper for outside-lock malloc region preparation
  [x] keep small span preparation unchanged
  [x] keep direct region preparation unchanged
  [x] keep the locked commit and unused release sequence unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `MallocLockedCommitHelper-L1`. It is not an
allocation policy change. It pairs with the prealloc preparation helper by
giving the locked recheck/commit section one owner.

```text
MallocLockedCommitHelper-L1:
  [x] add a helper for locked malloc recheck and prepared-region commit
  [x] keep retained/span recheck before commit
  [x] keep successful commit clearing the pending release
  [x] keep unused preallocated region release outside the lock
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `FreeLockedDispatchHelper-L1`. It is not an
allocation policy change. It mirrors the malloc flow cleanup by giving the
locked free route lookup and small/direct dispatch one helper while keeping OS
release outside the lock.

```text
FreeLockedDispatchHelper-L1:
  [x] add a helper for locked free route lookup and dispatch
  [x] keep route lookup and state transitions under the lock
  [x] keep OS release outside the lock
  [x] keep invalid/foreign frees ignored
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `StatsSnapshotHelper-L1`. It is not an
allocation policy change. It gives the locked stats snapshot a tiny helper so
the public `h7_stats()` wrapper matches the other API wrappers.

```text
StatsSnapshotHelper-L1:
  [x] add a helper for copying stats under the lock
  [x] keep public H7Stats layout unchanged
  [x] keep stats accounting unchanged
  [x] keep allocator policy and speed path unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `SpanSlotFreeListHelper-L1`. It is not an
allocation policy change. It gives span slot pointer calculation and free-list
pop/push operations tiny helpers while keeping bitmap and active stats
transitions unchanged.

```text
SpanSlotFreeListHelper-L1:
  [x] add a helper for computing one span slot pointer
  [x] add helpers for popping and pushing one free slot index
  [x] keep bitmap set/clear behavior unchanged
  [x] keep used_count / active_bytes behavior unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `RegionRouteValidationHelper-L1`. It is not a
route policy change. It gives the magic/cookie/kind/active checks used by route
lookup tiny helpers so `MISS / VALID / INVALID` meaning remains explicit.

```text
RegionRouteValidationHelper-L1:
  [x] add a helper for matching a region header to a route entry
  [x] add a helper for checking active region state
  [x] keep retained/inactive regions route INVALID
  [x] keep foreign pointers route MISS
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `RegionFlagStateHelper-L1`. It is not an
allocation policy change. It gives ACTIVE / RETAINED / released flag
transitions tiny helpers so direct and span release paths use the same region
state vocabulary.

```text
RegionFlagStateHelper-L1:
  [x] add helpers for active, retained, and released region flags
  [x] keep direct retained regions route INVALID
  [x] keep released regions unregistered before OS release
  [x] keep stats accounting unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `DirectReleaseTransitionHelper-L1`. It is not
an allocation policy change. It gives direct retained and OS-release transitions
the same explicit shape as the span release helper.

```text
DirectReleaseTransitionHelper-L1:
  [x] add a helper for moving a direct region to retained state
  [x] add a helper for detaching a direct region for OS release
  [x] keep route unregister before OS release
  [x] keep direct active/reserved stats accounting unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `PendingReleaseSetHelper-L1`. It is not an
allocation policy change. It completes the pending-release helper trio by
sharing the `ptr / size` assignment used by span release, direct release, and
outside-lock preallocation.

```text
PendingReleaseSetHelper-L1:
  [x] add a helper for setting pending release pointer and size
  [x] use it from span detach, direct detach, and preallocation
  [x] keep OS release outside the lock
  [x] keep pending release clear/now behavior unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next cleanup step is `StatsInvariantSmoke-L1`. It is not a policy change.
It keeps the allocator code unchanged and makes the public stats/route contract
explicit in a focused smoke test before the next performance experiment.

```text
StatsInvariantSmoke-L1:
  [x] add a stats-focused smoke test
  [x] assert active_bytes excludes retained direct regions after free
  [x] assert direct_count counts active direct regions only
  [x] assert retained 32K/64K direct routes remain INVALID, not MISS
  [x] assert retained direct reuse restores VALID routes and direct_count
  [x] avoid exact route_count == 0 checks because retained spans/direct routes
      intentionally remain registered
  [x] keep allocator policy and speed path unchanged
```

The next smoke parity step is `CrossPlatformMtSmoke-L1`. It is not a policy
change. It keeps the Windows MT smoke behavior and makes the same coarse-lock
multithread safety check run on Linux via pthreads.

```text
CrossPlatformMtSmoke-L1:
  [x] make hz7_mt_smoke compile on Windows and Linux
  [x] keep the same 4-thread allocation/free stress shape
  [x] wire hz7_mt_smoke into the Linux smoke script
  [x] keep allocator policy and speed path unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next API packaging step is `HeaderCppSmoke-L1`. It is not an allocator
policy change. It proves the public `hz7.h` header can be included from C++ and
linked against the C implementation without changing the tiny direct API.

```text
HeaderCppSmoke-L1:
  [x] add a C++ smoke that includes hz7.h
  [x] call h7_malloc / h7_route / h7_free / h7_stats from C++
  [x] wire the C++ smoke into Windows and Linux smoke scripts
  [x] keep allocator policy and speed path unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next maintenance step is `SmokeScriptModule-L1`. It is not an allocator
policy change. It keeps the smoke list unchanged while removing repetitive
Windows smoke build/run boilerplate so future checks can be added without
copying another large argument block.

```text
SmokeScriptModule-L1:
  [x] add a small Windows smoke build/run helper
  [x] keep all existing smoke binaries and output names unchanged
  [x] keep the Linux smoke script behavior unchanged unless a direct cleanup is
      needed
  [x] keep allocator policy and speed path unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next maintenance step is `LinuxSmokeScriptModule-L1`. It mirrors the
Windows smoke script cleanup by giving the Linux smoke runner small build/run
helpers while keeping the same smoke binaries, flags, and execution order.

```text
LinuxSmokeScriptModule-L1:
  [x] add small Linux smoke build/run helpers
  [x] keep all existing smoke binaries and output names unchanged
  [x] keep the C++ header smoke linked against a C-compiled hz7 object
  [x] keep allocator policy and speed path unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `RouteSlotClearHelper-L1`. It is not a route
policy change. It removes duplicated route-entry clearing logic from
`h7_route_unregister()` so the reserved-slot fast unregister path and fallback
scan path share one small helper.

```text
RouteSlotClearHelper-L1:
  [x] add a tiny helper for clearing one route table slot
  [x] keep route_count accounting unchanged
  [x] keep region->reserved reset behavior unchanged
  [x] keep foreign pointer MISS and retained/interior INVALID semantics
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `RegionHeaderInitHelper-L1`. It is not an
allocation policy change. It makes span and direct region preparation share the
same tiny header initializer so magic/cookie/kind/flags/size setup has a single
obvious owner.

```text
RegionHeaderInitHelper-L1:
  [x] add a tiny region-header initialization helper
  [x] use it from span and direct region preparation
  [x] keep region flags and route semantics unchanged
  [x] keep allocator policy and speed path unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `DirectActiveStatsHelper-L1`. It is not an
allocation policy change. It makes retained-direct reuse and newly committed
direct regions share the same active stats transition helper.

```text
DirectActiveStatsHelper-L1:
  [x] add tiny helpers for direct ACTIVE and inactive stats transitions
  [x] use them from retained reuse, direct commit, and direct free
  [x] keep retained direct route INVALID semantics unchanged
  [x] keep active_bytes/direct_count accounting unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `SpanActiveStatsHelper-L1`. It is not an
allocation policy change. It makes span slot allocation/free share tiny active
stats transition helpers, mirroring the direct-region stats cleanup.

```text
SpanActiveStatsHelper-L1:
  [x] add tiny helpers for span slot ACTIVE and inactive stats transitions
  [x] use them from span slot allocation and free
  [x] keep partial/empty span list behavior unchanged
  [x] keep active_bytes accounting unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The accepted cleanup step is `RouteValidationModule-L1`. It is not a policy
change. It keeps the current route lookup behavior, but shares the small/direct
user-pointer validation logic used by `h7_route()` and `h7_free()`.

```text
accepted:
  DirectRetainModule-L1
  [x] keep 32K and 64K retain caps unchanged
  [x] move direct retain bucket lookup/pop/push into small helpers
  [x] keep retained direct regions route INVALID, not MISS
  [x] keep RSS flat
  [x] require small / medium / mixed repeat check before treating it as accepted

measured:
  small   77.504M ops/s, 4,576 KB peak
  medium  17.938M ops/s, 5,040 KB peak
  mixed   19.078M ops/s, 5,504 KB peak

source:
  out_win_random_mixed_hz7v2_directretain_module_repeat5/
  20260611_152258_paper_random_mixed_windows.md

RouteLookupModule-L1:
  [x] keep route hash/probe policy unchanged
  [x] split exact-base lookup from direct-region range fallback
  [x] keep foreign pointer MISS
  [x] keep interior/retained/inactive pointer INVALID
  [x] pass Windows/Linux smoke before acceptance

measured:
  small   78.153M ops/s, 4,576 KB peak
  medium  17.876M ops/s, 5,036 KB peak
  mixed   19.361M ops/s, 5,496 KB peak

source:
  out_win_random_mixed_hz7v2_routelookup_module_repeat5/
  20260611_152559_paper_random_mixed_windows.md

RouteValidationModule-L1:
  [x] share small span slot validation between route and free
  [x] share direct user pointer validation between route and free
  [x] keep foreign pointer MISS
  [x] keep interior/retained/inactive pointer INVALID
  [x] avoid adding diagnostics or new route policy

measured:
  small   78.337M ops/s, 4,576 KB peak
  medium  18.199M ops/s, 5,036 KB peak
  mixed   19.491M ops/s, 5,504 KB peak

source:
  out_win_random_mixed_hz7v2_routevalidation_module_repeat5/
  20260611_152945_paper_random_mixed_windows.md

avoid:
  remote throughput policy
  owner-aware handoff
  TLS ownership
  broad diagnostic counters in production speed builds
```

## Remote Evidence

```text
Allowed:
  coarse-lock remote-free safety smoke
  route-capacity evidence
  fail-closed safety checks

Not allowed:
  remote throughput claims
  owner-aware remote free
  inbox / TLS ownership
  remote policy matrices
```
