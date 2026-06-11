# HZ7 TinyRoute V2 Tasks

HZ7 v2 is the readable tiny allocator reference track. This document turns the
current goal into a concrete task list so the folder has one obvious place to
look before adding more code.

## Active Task Board

Latest design review summary:

```text
Next identity:
  tiny local allocator reference
  remote-free safe
  not remote-throughput optimized

Current accepted default:
  SlowPathOutsideLock-L1
  DirectRetainCap-L2 cap 64

Next work:
  stabilize docs / lanes / baseline snapshots
  keep local small/medium/mixed plus RSS as the main scoreboard
  only add cleanup or measurement that preserves tiny readability
```

Task queue:

```text
done:
  SlowPathOutsideLock-L1
    moved OS allocation/release out of the global lock where route state is safe

  DirectRetainCap-L2
    promoted H7_DIRECT_RETAIN_CAP from 32 to 64
    recovered medium/mixed throughput without giving up low RSS

  BaselineSnapshot-L1
    refreshed the cross-allocator random_mixed small/medium/mixed snapshot
    confirmed HZ7 v2 cap64 is the lowest-RSS row, not the throughput winner

  DirectRetireHelper-L1
    kept direct free validation and retain/release dispatch explicit
    did not change direct retain policy or route semantics

  RoutePublicKindHelper-L1
    kept h7_route lookup separate from user-pointer VALID/INVALID interpretation
    did not change MISS / VALID / INVALID semantics

  CloseoutReview-L1
    added docs/HZ7_V2_CLOSEOUT_REVIEW.md
    captured current identity, code size, default lane, snapshot, and stop/go criteria

  StopOrContinuePrompt-L1
    added docs/HZ7_V2_STOP_OR_CONTINUE_PROMPT.md
    prepared the external review prompt for closeout vs one-more-experiment

active:
  HZ7 v2 lane closeout
    keep README current measurement on the cap64 default
    keep cap128/cap256 as controls only
    keep Windows/Linux smoke parity visible

next:
  StopOrContinueDecision-L1
    decide whether to stop HZ7 v2 here as the cap64 tiny reference
    use docs/HZ7_V2_STOP_OR_CONTINUE_PROMPT.md for external design review
    if continuing, use the baseline snapshot as the scoreboard before tuning

  OptionalCleanup-L1
    only accept additional tiny source/list cleanup if it preserves readability
    stop if the cleanup needs new policy, counters, owner state, or remote fast path

optional:
  SpanFreeListTrim-L1
    one tiny cleanup/perf probe only if it stays explainable
    stop immediately if it needs owner/TLS/remote policy

not now:
  owner-aware remote free
  owner inbox
  TLS ownership
  lock-free remote queue
  remote batching
  HZ6-style profile matrix
```

Acceptance for the current closeout:

```text
safety:
  Windows smoke passes
  Linux smoke passes
  route_register_fail = 0
  retained direct routes remain INVALID while retained

performance:
  random_mixed small/medium/mixed uses the cap64 default row
  medium/mixed stay materially above the old cap32 baseline
  RSS remains in the low-MiB HZ7 v2 shape

documentation:
  README says remote-safe, not remote-fast
  task doc labels remote-fast work as non-goal
  no-go controls stay archived instead of being re-tried silently
  baseline snapshot exists for the cap64 closeout row
```

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

## Latest Design Review Taskization

The current review confirms that HZ7 v2 should grow as a tiny local allocator,
not as a remote-throughput allocator. The next work should preserve the coarse
global lock contract while keeping slow OS work outside the lock whenever route
state has already been made safe.

```text
Accepted direction:
  LockScopeTrim-L1 / SlowPathOutsideLock-L1
  remote-free safe under the coarse global lock
  route-safe local small/medium direct API allocator

Keep:
  low RSS
  readable tiny allocator shape
  Windows/Linux smoke parity
  random_mixed small / medium / mixed baseline snapshots

Do not add in HZ7 v2:
  owner tokens
  owner inbox
  TLS ownership
  lock-free remote queue
  remote batching
  HZ6-style profile matrix
  production hot-path diagnostics
```

The line-count target is only a smell detector:

```text
1000-1500 lines:
  ideal tiny reference shape

1500-2000 lines:
  healthy HZ7 v2 if the code remains explainable

2000-2300 lines:
  review before adding more policy

2500+ lines:
  likely no longer HZ7 v2; consider a separate HZ6/HZ8-family track
```

Next task boundaries:

```text
Already accepted:
  SlowPathOutsideLock-L1 default path
  route/state transitions under lock
  final OS allocation/release outside lock

Allowed next:
  source/list/stat helper cleanup
  benchmark plumbing cleanup
  remote-free safety smoke maintenance
  one optional SpanFreeListTrim-L1 only if it stays tiny and measurable

Not next:
  remote fast path
  owner-aware handoff
  TLS cache with remote semantics
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

## Current Small Cleanup

`SpanFreeListTrim-L1` is the next allowed tiny cleanup. It is not a cache
policy change and does not change span retention, partial-list movement, or
RSS behavior. It only gives the free-list next-index cell stored inside each
free span slot one helper owner.

```text
SpanFreeListTrim-L1:
  [x] add helpers for reading and writing a free slot next index
  [x] use them from span preparation, pop, and push
  [x] keep bitmap set/clear behavior unchanged
  [x] keep partial/empty span list behavior unchanged
  [x] keep active_bytes and used_count accounting unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `PreparedRegionValidationHelper-L1`. It is not
a route policy change. It shares the small/direct prepared-region checks used
by the locked commit path after outside-lock OS allocation.

```text
PreparedRegionValidationHelper-L1:
  [x] add helpers for validating prepared span and direct regions
  [x] use them from small and direct commit-and-alloc paths
  [x] keep the existing magic/kind checks unchanged
  [x] keep route registration and stats transitions unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `DirectUserOffsetHelper-L1`. It is not a size
policy change. It gives the direct-region user pointer offset one helper so
direct pointer, region-size, and preparation paths use the same layout rule.

```text
DirectUserOffsetHelper-L1:
  [x] add a helper for the direct-region user offset
  [x] use it from direct user pointer, region sizing, and preparation
  [x] keep 16-byte user alignment unchanged
  [x] keep retained direct route semantics unchanged
  [x] require Windows and Linux smoke scripts to pass
```

The next source cleanup step is `SizeClassBoundaryHelper-L1`. It is not a size
class policy change. It names the small/span boundary so malloc prepare,
existing allocation, and prepared-region commit all use the same predicate.

```text
SizeClassBoundaryHelper-L1:
  [x] add a helper for the small/span size boundary
  [x] use it from malloc existing, preallocation, and commit paths
  [x] keep class table and H7_SPAN_CLASS_MAX unchanged
  [x] keep direct retained bucket behavior unchanged
  [x] require Windows and Linux smoke scripts to pass
```

## Cleanup Baseline Snapshot

After the current helper cleanup series, HZ7 v2 remains a low-RSS tiny direct
API allocator. The cleanup series did not add a new policy knob, so this
snapshot should be read as the current readable-code baseline rather than a new
performance promotion.

Windows `random_mixed` repeat-5, direct-API row `hz7-v2`:

```text
small   76.680M ops/s, 4,576 KB peak
medium  17.505M ops/s, 5,044 KB peak
mixed   19.040M ops/s, 5,500 KB peak
```

Source:

```text
out_win_random_mixed_hz7v2_cleanup_repeat5/
20260611_164902_paper_random_mixed_windows.md
```

Next decision:

```text
Do not keep splitting helpers unless the boundary remains obvious.
Next performance work should start from measured small/medium/mixed behavior,
not from remote-fast features.
```

## Size-Slice Diagnostic

`Hz7V2SizeSliceRunner-L1` adds an HZ7 v2-only diagnostic runner that reuses the
existing Windows `bench_random_mixed_compare` executable. It does not add a new
allocator policy. Its purpose is to split the medium profile into the
span-covered and direct-retained halves before changing direct-region behavior.

```text
Hz7V2SizeSliceRunner-L1:
  [x] add a Windows HZ7 v2-only size-slice runner
  [x] keep the runner outside allocator hot paths
  [x] measure small, span_4k_16k, direct_16k_32k, and mixed
  [x] write markdown and raw log artifacts
  [x] keep allocator code unchanged
```

Windows HZ7 v2 size-slice diagnostic, runs=3:

```text
small_16_2k     77.139M ops/s, 4,584 KB peak
span_4k_16k     27.894M ops/s, 5,164 KB peak
direct_16k_32k  16.280M ops/s, 4,760 KB peak
mixed_16_32k    19.101M ops/s, 5,500 KB peak
```

Source:

```text
out_win_hz7_v2_size_slices_cleanup/
20260611_165230_hz7_v2_size_slices_windows.md
```

Reading:

```text
medium weakness is mostly the direct-retained 16K+..32K slice.
span-covered 4K..16K is slower than small but not the main collapse.
next performance work should inspect direct retained reuse / route transitions,
not remote-fast machinery.
```

## Direct Retain Cap Promotion

`DirectRetainCap32-L1` is a small policy change. The size-slice diagnostic
showed that the medium weakness is concentrated in the direct-retained
`16K+..32K` slice, and increasing the direct retain cap from 16 to 32 removes
most of that collapse without changing the route contract.

```text
DirectRetainCap32-L1:
  [x] promote H7_DIRECT_RETAIN_CAP default from 16 to 32
  [x] keep only the existing 32K and 64K retained buckets
  [x] keep retained direct regions route INVALID, not MISS
  [x] keep remote-free safety unchanged
  [x] require Windows and Linux smoke scripts to pass
  [x] require random_mixed repeat-5 after promotion
```

Cap=32 size-slice probe, runs=3:

```text
small_16_2k     78.162M ops/s, 5,016 KB peak
span_4k_16k     28.227M ops/s, 5,168 KB peak
direct_16k_32k  43.031M ops/s, 4,752 KB peak
mixed_16_32k    30.127M ops/s, 5,508 KB peak
```

Source:

```text
out_win_hz7_v2_size_slices_cap32/
20260611_165545_hz7_v2_size_slices_windows.md
```

Default cap=32 random_mixed repeat-5:

```text
small   77.536M ops/s, 4,576 KB peak
medium  28.789M ops/s, 5,040 KB peak
mixed   29.852M ops/s, 5,512 KB peak
```

Source:

```text
out_win_random_mixed_hz7v2_cap32_default_repeat5/
20260611_165655_paper_random_mixed_windows.md
```

## Empty Span Cap Promotion

`EmptySpanCap4-L1` is a small span-cache policy change. After direct retain
cap=32, the remaining medium weakness moved to the span-covered `4K..16K`
slice. Raising the per-class empty span cap from 1 to 4 improves that slice
while preserving the coarse global lock and route safety contract.

```text
EmptySpanCap4-L1:
  [x] promote H7_EMPTY_SPAN_CAP default from 1 to 4
  [x] keep span class table unchanged
  [x] keep partial/empty span list semantics unchanged
  [x] keep retained empty spans route INVALID, not MISS
  [x] require Windows and Linux smoke scripts to pass
  [x] require random_mixed repeat-5 after promotion
```

Empty cap probe, runs=3:

```text
emptycap2:
  span_4k_16k  32.967M ops/s, 5,172 KB peak
  mixed_16_32k 34.273M ops/s, 5,584 KB peak

emptycap4:
  span_4k_16k  44.202M ops/s, 5,200 KB peak
  mixed_16_32k 40.606M ops/s, 5,620 KB peak
```

Source:

```text
out_win_hz7_v2_size_slices_emptycap2/
20260611_170015_hz7_v2_size_slices_windows.md

out_win_hz7_v2_size_slices_emptycap4/
20260611_170015_hz7_v2_size_slices_windows.md
```

Default emptycap4 random_mixed repeat-5:

```text
small   78.968M ops/s, 4,572 KB peak
medium  41.005M ops/s, 5,076 KB peak
mixed   41.477M ops/s, 5,612 KB peak
```

Source:

```text
out_win_random_mixed_hz7v2_emptycap4_default_repeat5/
20260611_170118_paper_random_mixed_windows.md
```

## Cross-Allocator Snapshot

After `DirectRetainCap32-L1` and `EmptySpanCap4-L1`, HZ7 v2 is no longer just a
tiny safety reference. It remains slower than HZ3/tcmalloc on this
single-thread random_mixed suite, but it is the lowest-RSS row in all three
profiles.

Windows `random_mixed` runs=3:

```text
small:
  hz3       155.273M ops/s,  7,184 KB peak
  hz4       120.775M ops/s, 12,564 KB peak
  hz7-v2     79.859M ops/s,  4,580 KB peak
  mimalloc  102.449M ops/s,  6,548 KB peak
  tcmalloc  123.558M ops/s,  9,508 KB peak

medium:
  hz3       155.948M ops/s,  6,440 KB peak
  hz4        84.061M ops/s,  9,812 KB peak
  hz7-v2     41.186M ops/s,  5,080 KB peak
  mimalloc   89.051M ops/s, 11,652 KB peak
  tcmalloc  154.142M ops/s, 10,280 KB peak

mixed:
  hz3       150.675M ops/s,  7,444 KB peak
  hz4        79.981M ops/s, 18,268 KB peak
  hz7-v2     42.376M ops/s,  5,612 KB peak
  mimalloc   89.200M ops/s, 11,896 KB peak
  tcmalloc  150.727M ops/s, 10,896 KB peak
```

Source:

```text
out_win_random_mixed_hz7v2_cross_compare/
20260611_170729_paper_random_mixed_windows.md
```

Reading:

```text
HZ7 v2 strength:
  lowest RSS across small / medium / mixed
  much stronger than the initial v2 cleanup baseline
  tiny source size still under 900 lines

HZ7 v2 weakness:
  single-thread throughput still below HZ3/tcmalloc
  medium/mixed speed is now useful but not top-tier

Next likely target:
  same-thread hot path cost, especially global lock + route lookup overhead
  do not chase remote fast path yet
```

## Hot Path Microbench

`Hz7V2HotPathMicrobench-L1` is diagnostic-only. It measures direct-API
`malloc/free` pairs and standalone `h7_route()` calls without adding allocator
counters or production hot-path instrumentation.

Command:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna-hz7-v2\win\run_win_hz7_v2_hotpath.ps1 -Runs 3 -Iters 5000000
```

Observed Windows run:

```text
malloc_free:
  small64    48.310M pairs/s
  span8k     47.536M pairs/s
  direct32k  56.162M pairs/s

route_valid:
  small64   120.945M ops/s
  span8k    120.671M ops/s
  direct32k 121.777M ops/s

route_invalid:
  small64   120.931M ops/s
  span8k    121.037M ops/s
  direct32k 118.798M ops/s
```

Source:

```text
out_win_hz7_v2_hotpath_check/
20260611_171114_hz7_v2_hotpath_windows.md
```

Reading:

```text
h7_route() alone is around 120M ops/s.
The direct-API malloc/free pair path is around 47M-56M pairs/s.

This means route lookup is not the only blocker.
The next HZ7 v2 optimization should inspect lock scope and slow-path work
inside the coarse global lock before adding remote-fast machinery.
```

## Next Task: LockScopeTrim / SlowPathOutsideLock

Consultation result:

```text
Remote fast path is not the next best target.
The next target is reducing unnecessary work under the global lock while
preserving HZ7 v2's simple route-safe, remote-free-safe contract.
```

Task plan:

```text
1. Keep HZ7 v2 remote-free safe, but not remote-throughput optimized.
2. Add no production hot-path counters.
3. Identify work currently performed under the global lock.
4. Move pure slow-path preparation outside the lock where correctness allows.
5. Keep route mutation, span free-list mutation, and direct retain mutation
   inside the lock.
6. Validate with smoke tests plus the hot-path diagnostic.
```

Candidate split:

```text
Inside lock:
  route table insert/remove/lookup used by allocation/free
  span free-list mutation
  direct retain bucket mutation
  global accounting visible through h7_stats

Outside lock if safe:
  requested-size classification
  direct allocation size rounding
  OS direct allocation before final route publish
  failed slow-path cleanup after route publish failure
```

Acceptance:

```text
safety:
  Windows and Linux smokes pass
  remote-free smoke stays clean
  route invalid / retained route invariants stay clean

performance:
  hot-path malloc_free improves or stays flat
  random_mixed small/medium/mixed does not regress
  RSS remains near current low-RSS baseline

design:
  no owner inbox
  no lock-free remote queue
  no TLS frontcache yet
  no production counter/atomic in the hot path
```

### LockScopeTrim-L1 observation

Tried direction:

```text
AllocPlan/ClassFast-L1:
  compute small/direct classification outside the lock
  pass class_id/size plan into malloc locked sections
  avoid repeated h7_class_for_size / h7_size_is_small checks under lock
```

Safety result:

```text
Windows smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok

Linux smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok
```

Performance result:

```text
hotpath:
  baseline small64 malloc_free    48.310M pairs/s
  L1 small64 malloc_free          46.523M pairs/s

  baseline span8k malloc_free     47.536M pairs/s
  L1 span8k malloc_free           46.366M pairs/s

  baseline direct32k malloc_free  56.162M pairs/s
  L1 direct32k malloc_free        53.953M pairs/s

random_mixed:
  baseline small   78.968M-79.859M ops/s depending snapshot
  L1 small         78.080M ops/s

  baseline medium  41.005M-41.186M ops/s
  L1 medium        40.080M ops/s

  baseline mixed   41.477M-42.376M ops/s
  L1 mixed         41.933M ops/s
```

Decision:

```text
AllocPlan/ClassFast-L1:
  no-go / do not promote

Reason:
  safe but not faster.
  HZ7 v2's remaining same-thread cost is not explained by lock-inside
  size-class recomputation alone.

Next:
  keep allocator body at the previous selected implementation
  look for a different hot-path target before changing remote-free design
```

### Component Hot Path Diagnostic

`Hz7V2ComponentHotPath-L1` extends the diagnostic-only hotpath bench. It does
not add allocator counters or change production code.

Added rows:

```text
malloc_batch:
  allocate many objects first, then free them after measurement

free_batch:
  preallocate many objects, then measure only the free phase

free_retained_loop:
  seed one retained object, then measure malloc/free steady reuse
```

Windows repeat-3 observation:

```text
steady pair path:
  malloc_free small64       48.938M pairs/s
  malloc_free span8k        47.915M pairs/s
  malloc_free direct32k     56.920M pairs/s

route only:
  route_valid small64      122.091M ops/s
  route_valid span8k       122.136M ops/s
  route_valid direct32k    123.175M ops/s

batch allocate:
  malloc_batch small64      43.462M ops/s
  malloc_batch span8k        0.708M ops/s
  malloc_batch direct32k      0.537M ops/s

batch free:
  free_batch small64        77.011M ops/s
  free_batch span8k          1.921M ops/s
  free_batch direct32k        0.840M ops/s

retained steady loop:
  free_retained_loop small64    49.038M pairs/s
  free_retained_loop span8k     48.349M pairs/s
  free_retained_loop direct32k  56.125M pairs/s
```

Source:

```text
out_win_hz7_v2_hotpath_component_l1/
20260611_172336_hz7_v2_hotpath_windows.md
```

Reading:

```text
Route lookup is not the main steady-state blocker.
Steady reuse is reasonably flat across small/span/direct.

The very slow rows are cold/source-pressure batch rows:
  many live span/direct allocations
  many route registrations
  later route unregister/release or retain-limit pressure

This means HZ7 v2 should not chase remote-fast yet, and it should not treat
random_mixed weakness as a simple route lookup issue.
```

Next candidate:

```text
MixedSizeSteady-L1 diagnostic:
  measure a steady random-size working set with bounded live objects
  separate same-size retained reuse from cross-size churn
  identify whether medium/mixed weakness is class switching, direct retain
  bucket mismatch, route churn, or benchmark RNG/workload overhead
```

### MixedSizeSteady-L1

Added diagnostic-only rows to the hotpath bench:

```text
mixed_steady small_ws400:
  16B..2KiB, live set 400

mixed_steady span_medium_ws400:
  4KiB..16KiB, live set 400

mixed_steady direct_medium_ws400:
  16KiB+1..32KiB, live set 400

mixed_steady medium_ws400:
  4KiB..32KiB, live set 400

mixed_steady mixed_ws400:
  16B..32KiB, live set 400
```

Windows repeat-3 observation:

```text
small_ws400          51.191M ops/s
span_medium_ws400   37.956M ops/s
direct_medium_ws400 54.547M ops/s
medium_ws400        24.280M ops/s
mixed_ws400         23.083M ops/s
```

Source:

```text
out_win_hz7_v2_hotpath_mixedsteady_slices_l1/
20260611_172642_hz7_v2_hotpath_windows.md
```

Reading:

```text
Direct-retained medium is not the weak row by itself.
Small steady and direct-medium steady are both around 50M+ ops/s.
Span-medium steady is lower but still far above cross-boundary medium/mixed.

The weak signal appears when the workload crosses the 16KiB span/direct
boundary with a bounded live set. This points at cross-class / cross-source
churn rather than a simple route lookup or direct retain problem.

The peak_kb column in this diagnostic is cumulative within one process and is
not used as RSS evidence for these mixed_steady rows.
```

Next implementation candidates:

```text
BoundaryChurn-L1 diagnostic:
  count how often steady workloads cross small/span/direct source families
  without adding production counters

MediumBoundaryPolicy-L1:
  test whether changing the 16KiB span/direct boundary improves cross-boundary
  steady mixed rows without hurting small/direct-only rows

DirectRetainClass-L1:
  test whether finer direct retain buckets help only after boundary churn is
  proven to be a direct-retain mismatch rather than source-family switching
```

### BoundaryChurn-L1

`BoundaryChurn-L1` adds diagnostic-only source-family switch attribution to
the `mixed_steady` rows. It does not add allocator counters or change HZ7 v2
production code.

Family definition:

```text
span family:
  size <= 16KiB

direct family:
  size > 16KiB
```

Windows repeat-3 observation:

```text
small_ws400:
  median 50.324M ops/s
  switch_rate 0.000000

span_medium_ws400:
  median 37.173M ops/s
  switch_rate 0.000000

direct_medium_ws400:
  median 53.592M ops/s
  switch_rate 0.000000

medium_ws400:
  median 24.176M ops/s
  switch_rate 0.489742

mixed_ws400:
  median 23.206M ops/s
  switch_rate 0.499983
```

Source:

```text
out_win_hz7_v2_hotpath_boundarychurn_l1/
20260611_172849_hz7_v2_hotpath_windows.md
```

Reading:

```text
Boundary churn is now the strongest explanation for the medium/mixed steady
drop.

Rows with switch_rate 0 remain much faster:
  small_ws400 around 50M
  direct_medium_ws400 around 54M
  span_medium_ws400 around 37M

Rows crossing the 16KiB span/direct boundary about half the time drop to
around 23M-24M.
```

Decision:

```text
Next best target:
  MediumBoundaryPolicy-L1

Why:
  The weakness is not direct-retain alone and not route lookup alone.
  It appears when a steady live set alternates between span and direct source
  families.

First experiment:
  raise the span/direct boundary for HZ7 v2 in an explicit lane
  compare:
    span/direct-only steady rows
    medium_ws400
    mixed_ws400
    random_mixed small/medium/mixed
    RSS

No-go:
  RSS loses HZ7 v2's low-memory identity
  small or direct-only rows regress materially
  medium/mixed do not improve
```

### MediumBoundaryPolicy-L1

Implemented as an explicit build lane, not a default change:

```text
H7_SPAN_CLASS_MAX default:
  16KiB

H7_SPAN_CLASS_MAX=32768:
  enables an additional 32KiB span class
  keeps default HZ7 v2 unchanged
```

Build hooks:

```text
hakozuna-hz7-v2/win/run_win_hz7_v2_hotpath.ps1:
  -SpanClassMax 32768

win/build_win_random_mixed_suite.ps1:
  -Hz7V2SpanClassMax 32768

win/run_win_random_mixed_paper.ps1:
  -SuiteDirName <custom suite dir>
```

Safety:

```text
Default Windows smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok

Default Linux smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok
```

Windows span32 observation:

```text
hotpath, H7_SPAN_CLASS_MAX=32768:
  small_ws400          49.930M ops/s
  span_medium_ws400   36.880M ops/s
  direct_medium_ws400 42.476M ops/s
  medium_ws400         2.540M ops/s
  mixed_ws400          2.484M ops/s

random_mixed, H7_SPAN_CLASS_MAX=32768:
  small   78.488M ops/s, 5,016 KB peak
  medium   4.928M ops/s, 5,708 KB peak
  mixed    5.864M ops/s, 6,172 KB peak
```

Sources:

```text
out_win_hz7_v2_hotpath_span32_l1/
20260611_173229_hz7_v2_hotpath_windows.md

out_win_random_mixed_hz7v2_span32_l1/
20260611_173229_paper_random_mixed_windows.md
```

Decision:

```text
MediumBoundaryPolicy-L1:
  no-go / do not promote

Reason:
  raising the span/direct boundary to 32KiB makes medium/mixed dramatically
  worse. Large span slots are too expensive for this tiny allocator design.

Keep:
  H7_SPAN_CLASS_MAX override support as an evidence/control lane.

Default:
  keep 16KiB span boundary.
```

Next:

```text
Do not solve boundary churn by expanding spans to 32KiB.

Next target should preserve the 16KiB boundary and instead reduce cross-family
steady churn:
  direct retain bucket / admission policy
  medium range split policy
  or benchmark/lane separation between span-medium and direct-medium profiles
```

### RandomToggle-L1 correction

`bench_random_mixed_compare` is a toggle workload:

```text
if slot is empty:
  allocate
else:
  free
```

It does not do a free+alloc replacement every operation, and it does not touch
the allocated payload. Earlier `mixed_steady` rows are therefore useful as a
boundary-replacement stress, but they are not the same as the paper
`random_mixed` row.

Added diagnostic rows:

```text
random_toggle fresh_*:
  run before the heavy batch rows so the process is not polluted by batch
  source pressure

random_toggle fresh_*_notouch:
  same toggle workload, but no payload touch on allocation
  closer to bench_random_mixed_compare
```

Windows repeat-3 observation:

```text
fresh_small_ws400:
  touch     86.072M ops/s
  notouch   85.729M ops/s

fresh_medium_ws400:
  touch     21.172M ops/s
  notouch   42.647M ops/s

fresh_mixed_ws400:
  touch     23.276M ops/s
  notouch   43.573M ops/s
```

Source:

```text
out_win_hz7_v2_hotpath_toggle_notouch_l1/
20260611_173807_hz7_v2_hotpath_windows.md
```

Corrected reading:

```text
The paper random_mixed medium/mixed row is closer to the notouch toggle row.
The 20M-class medium/mixed result was mostly payload touch/page cost mixed
with allocator cost.

HZ7 v2 still has a real medium/mixed gap:
  small toggle notouch   around 86M
  medium toggle notouch  around 43M
  mixed toggle notouch   around 44M

This gap is no longer explained by payload touch.
It is likely caused by direct-retain capacity/admission and source churn in
the medium range, not by 32KiB span expansion.
```

Next:

```text
DirectRetainCap-L2:
  measure cap 64 / 128 / 256 explicitly with notouch random_toggle and
  random_mixed medium/mixed.

Acceptance:
  medium/mixed improve materially
  small does not regress
  RSS stays close to the low-RSS HZ7 v2 baseline

No-go:
  speed improves only by retaining too much memory
  RSS loses the HZ7 v2 identity
```

### DirectRetainCap-L2

Tested `H7_DIRECT_RETAIN_CAP` 64 / 128 / 256 using explicit lanes.
`hakozuna-hz7-v2/win/run_win_hz7_v2_hotpath.ps1` now accepts
`-DirectRetainCap`.

Hotpath random_toggle notouch observation:

```text
cap 64:
  fresh_small_ws400_notouch   85.736M ops/s
  fresh_medium_ws400_notouch  56.777M ops/s
  fresh_mixed_ws400_notouch   55.290M ops/s

cap 128:
  fresh_small_ws400_notouch   84.679M ops/s
  fresh_medium_ws400_notouch  57.580M ops/s
  fresh_mixed_ws400_notouch   53.947M ops/s

cap 256:
  fresh_small_ws400_notouch   85.135M ops/s
  fresh_medium_ws400_notouch  57.284M ops/s
  fresh_mixed_ws400_notouch   53.907M ops/s
```

Windows random_mixed repeat-3 observation:

```text
cap 64:
  small   76.577M ops/s, 5,020 KB peak
  medium  53.224M ops/s, 5,144 KB peak
  mixed   51.563M ops/s, 5,672 KB peak

cap 128:
  small   77.146M ops/s, 5,020 KB peak
  medium  53.264M ops/s, 5,176 KB peak
  mixed   51.320M ops/s, 5,728 KB peak

cap 256:
  small   77.510M ops/s, 5,028 KB peak
  medium  53.208M ops/s, 5,188 KB peak
  mixed   51.498M ops/s, 5,736 KB peak
```

Default confirmation after promoting cap 64:

```text
small   78.281M ops/s, 5,016 KB peak
medium  53.657M ops/s, 5,144 KB peak
mixed   51.995M ops/s, 5,672 KB peak
```

Sources:

```text
out_win_hz7_v2_hotpath_retain64_l2/
20260611_174052_hz7_v2_hotpath_windows.md

out_win_hz7_v2_hotpath_retain128_l2/
20260611_174052_hz7_v2_hotpath_windows.md

out_win_hz7_v2_hotpath_retain256_l2/
20260611_174052_hz7_v2_hotpath_windows.md

out_win_random_mixed_hz7v2_retain64_l2/
20260611_174118_paper_random_mixed_windows.md

out_win_random_mixed_hz7v2_retain128_l2/
20260611_174118_paper_random_mixed_windows.md

out_win_random_mixed_hz7v2_retain256_l2/
20260611_174118_paper_random_mixed_windows.md

out_win_random_mixed_hz7v2_default_retain64_confirm/
20260611_174218_paper_random_mixed_windows.md
```

Safety:

```text
Default Windows smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok

Default Linux smokes:
  hz7_smoke ok
  hz7_remote_smoke ok
  hz7_mt_smoke ok
  hz7_stats_smoke ok
  hz7_cpp_smoke ok
```

Decision:

```text
H7_DIRECT_RETAIN_CAP default:
  promote from 32 to 64

Why:
  cap 64 recovers medium/mixed throughput while preserving HZ7 v2's low-RSS
  shape. cap 128/256 do not materially improve speed and retain slightly more
  memory.

Keep as controls:
  cap 128 / cap 256

Default:
  cap 64
```

### BaselineSnapshot-L1

After promoting `H7_DIRECT_RETAIN_CAP=64`, refreshed the Windows
`random_mixed` cross-allocator snapshot using the existing paper-aligned runner.

Command shape:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_random_mixed_suite.ps1 `
  -OnlyHz7V2 `
  -OutDirName out_win_random_mixed

powershell -ExecutionPolicy Bypass -File .\win\run_win_random_mixed_paper.ps1 `
  -Runs 3 `
  -Profiles small,medium,mixed `
  -Allocators hz3,hz4,hz7-v2,mimalloc,tcmalloc `
  -OutputDir .\docs\benchmarks\windows\hz7_v2_baseline_snapshot `
  -SuiteDirName out_win_random_mixed
```

Result:

```text
docs/benchmarks/windows/hz7_v2_baseline_snapshot/
20260611_174745_paper_random_mixed_windows.md
```

HZ7 v2 cap64 row:

```text
small:
  79.741M ops/s
  4,576 KB peak

medium:
  53.353M ops/s
  5,140 KB peak

mixed:
  52.911M ops/s
  5,664 KB peak
```

Decision:

```text
keep:
  HZ7 v2 cap64 default as the current closeout lane

strength:
  lowest-RSS row in small / medium / mixed among the selected allocators
  medium/mixed materially improved over the cap32/emptycap4 rows

limit:
  not a throughput winner against hz3/tcmalloc
  not a remote-throughput allocator

next:
  do not retune blindly
  use this snapshot as the scoreboard before any further OptionalCleanup-L1 work
```

### DirectRetireHelper-L1

This is an `OptionalCleanup-L1` source cleanup, not an allocation policy change.
It makes direct free easier to audit by splitting validation from the retained
vs OS-release decision.

Implementation:

```text
h7_big_is_active_user:
  validates direct region magic / kind / ACTIVE flag / exact user pointer

h7_big_retire_locked:
  pushes to the direct retain bucket when possible
  otherwise unregisters the route and schedules OS release outside the lock
```

Contract:

```text
unchanged:
  H7_DIRECT_RETAIN_CAP default remains 64
  retained direct regions remain route INVALID
  route unregister still happens under the lock before OS release
  final OS release still happens outside the lock
  no owner / inbox / TLS / remote-fast policy is added
```

Acceptance:

```text
required:
  Windows smoke passes
  Linux smoke passes

expected:
  no material random_mixed change because this is only readability cleanup
```

### RoutePublicKindHelper-L1

This is an `OptionalCleanup-L1` route readability cleanup, not a route policy
change. It keeps `h7_route_unlocked()` as a tiny lookup wrapper and moves the
region-kind-specific user pointer interpretation into one helper.

Implementation:

```text
h7_region_user_route_kind:
  returns MISS/INVALID directly for non-VALID route lookup results
  validates small span slot pointers with h7_small_slot_index
  validates direct user pointers with h7_big_is_user_ptr
  returns INVALID for unknown route region kinds
```

Contract:

```text
unchanged:
  foreign pointer -> MISS
  active exact pointer -> VALID
  interior/retained/inactive pointer -> INVALID
  h7_free still performs the locked dispatch path separately
  no route table policy change
```

Acceptance:

```text
required:
  Windows smoke passes
  Linux smoke passes
```
