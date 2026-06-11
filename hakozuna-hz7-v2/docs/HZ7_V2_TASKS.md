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

Only add a new tiny local hot-path change if it can beat the archived scan
baseline on small / medium / mixed together. If it cannot, keep v1 frozen and
leave the knob archived.

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
