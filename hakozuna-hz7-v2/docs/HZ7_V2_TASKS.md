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

## Priority 1: Must-Have

- [ ] Keep local small/medium performance viable
- [ ] Keep cross-thread free safe under the coarse global lock
- [ ] Preserve fail-closed route safety
- [ ] Keep RSS low and predictable
- [ ] Keep the code readable enough to explain as a tiny reference

## Priority 2: Implementation

- [ ] Keep the direct API small and explicit
- [ ] Keep the small/medium hot path separate from remote safety concerns
- [x] Implement `SlowPathOutsideLock-L1` before adding remote fast paths
- [x] Move OS allocation/release work out of the global lock where route safety allows
- [ ] Keep remote as safety/evidence only, not a throughput claim
- [ ] Keep route/class changes tiny unless they beat the archived baseline
- [ ] Fail closed on invalid or foreign-looking input

## Priority 3: Documentation

- [ ] Keep the v2 goal statement in sync with the code shape
- [ ] Keep the README short and direct about what v2 is and is not
- [ ] Keep no-go decisions visible so repeated dead ends stay archived
- [ ] Keep v1 frozen as the cute tiny baseline

## Priority 4: Evaluation

- [ ] Compare small / medium / mixed together before promoting a change
- [ ] Track RSS alongside throughput
- [ ] Use remote only as a safety/control check
- [ ] Preserve the archived no-go baseline when a new idea does not earn a win

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

## Next Tiny-Step Rule

The next active tiny step is `SlowPathOutsideLock-L1`, also called
`LockScopeTrim-L1`. Keep the coarse global lock design, but stop holding that
lock across OS allocation and OS release when the route/state transition can be
completed first.

Status:

```text
implemented and accepted as the current HZ7 v2 default path
```

Implementation order:

- [x] Split malloc into a locked fast check, an outside-lock OS allocation, and a locked commit
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
  small   76.641M ops/s, 4,572 KB peak
  medium  17.230M ops/s, 5,040 KB peak
  mixed   18.738M ops/s, 5,500 KB peak

source:
  out_win_random_mixed_hz7v2_slowpath_repeat5/
  20260611_151350_paper_random_mixed_windows.md
```

No-go:

- [ ] Do not move route unregister outside the lock
- [ ] Do not free an OS region while its route entry is still active
- [ ] Do not add owner tokens, inboxes, TLS ownership, remote queues, or production diagnostics
- [ ] Do not keep the change if the code becomes hard to explain as a tiny reference

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
