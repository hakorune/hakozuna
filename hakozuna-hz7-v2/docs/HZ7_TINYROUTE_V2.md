# HZ7 TinyRoute V2

HZ7 TinyRoute V2 is the next-stage design note for a tiny allocator that stays
small while becoming more complete on local workloads.

This note applies to the separate `hakozuna-hz7-v2/` folder. The v1 tree stays
frozen as the cute tiny baseline.

## Folder Policy

```text
Code:
  keep HZ7 v2 in the separate hakozuna-hz7-v2 folder

Docs:
  keep v2 notes under hakozuna-hz7-v2/docs

Do not:
  merge v2 back into the v1 folder unless the shapes converge again
```

The reason is simple: HZ7 v1 is still one small allocator family, but v2 is
now the growth track. Keeping them separate lets v1 stay tiny and readable
while v2 can grow without blurring the baseline.

## HZ7 Tiny V2 Goal

```text
Target:
  a readable tiny allocator reference implementation

Design center:
  direct API
  route safety
  low RSS
  local small/medium performance
  remote-free safe
  coarse multithread safety
  teachable invariants and no-go history
  line growth as a soft guard, not a hard cap

Non-goals:
  remote allocator speed
  libc interpose
  owner inboxes
  TLS ownership
  HZ6-style profile matrix
```

Current task list:

- [HZ7_V2_TASKS.md](HZ7_V2_TASKS.md)

## Why Separate Folder

Keeping v2 separate keeps the baseline honest.

```text
v1:
  frozen cute tiny baseline

v2:
  growth track for a local-complete tiny allocator
```

The current HZ7 v1 implementation stays compact and readable on its own.
Splitting v2 out now lets us grow the local-complete path without blurring the
baseline or the line budget.

## Suggested Organization

```text
hakozuna-hz7/
  hz7.c
  hz7.h
  README.md
  docs/HZ7_TINYROUTE_PLAN.md
  docs/HZ7_TINYROUTE_PLAN_V2.md
  tests/
  win/
  linux/

hakozuna-hz7-v2/
  hz7.c
  hz7.h
  README.md
  docs/HZ7_TINYROUTE_PLAN.md
  docs/HZ7_TINYROUTE_PLAN_V2.md
  docs/HZ7_TINYROUTE_V2.md
  tests/
  win/
  linux/
```

## Split Trigger

Only consider a new code folder if one of these becomes true:

```text
1. v2 needs a different public API.
2. v2 needs a separate build/run matrix.
3. v2 needs a runtime layout that no longer reads as "tiny allocator".
4. v2 starts pulling in remote/owner/thread-local subsystems.
5. the main allocator core no longer fits comfortably in one file tree.
```

## Practical Recommendation

```text
Now:
  keep v1 frozen
  grow v2 in its own folder
  start with local small/medium reuse

Later:
  only reunify if the code shapes converge again
```

## Current HZ7 v1 Read

```text
HZ7 v1:
  tiny route-safe allocator
  direct API
  coarse MT smoke
  local random_mixed focus
  DirectRetain32/64 cap=16 default
  remote is evidence/control, not a performance claim
```

## v2 Read

```text
HZ7 v2:
  try to stay under 2000 lines
  keep local allocator scope
  improve local performance without importing HZ6 complexity
  keep route safety and low-RSS as first-class goals
  start by strengthening medium local reuse

Remote read:
  remote is evidence/control only
  no owner-aware remote free
  no inbox/TLS ownership
  if remote is revisited, it should stay in a smoke/preset lane rather than a
  performance lane
```

## Current Growth Step

```text
First pass:
  keep span initialization lightweight
  keep the route path exact-base and tiny

Read:
  keep the core easy to explain
  keep line growth as a soft guard, not a hard cap

  route cleanup needs to earn its keep in benchmark
  if a cleanup knob does not improve medium/mixed, keep v1 frozen and
  archive the knob as hygiene-only
```

## Archived No-Gos

```text
medium empty-span cap experiments:
  cap=2 / cap=4 / cap=8 style tweaks did not improve the local-complete shape
  they pushed v2 away from the tiny baseline without earning a stable win

Read:
  medium/local reuse should not be forced by a cache knob

route_slot_index cleanup alone:
  makes unregister safer and cleaner, but did not earn a stable random_mixed
  win on its own

Read:
  route_slot_index cleanup is hygiene-only unless a cache or other route hot
  path change also comes with it

route hot cache:
  exact-base one-slot route cache was tried on top of the slot-hint cleanup
  repeat-5 recheck did not hold a stable win across small / medium / mixed

Read:
  route hot cache is archived no-go for now; keep it out of the default HZ7 v2
  path unless a later design brings a stronger paired change

SpanClassLookupTrim-L1:
  branch-ladder size->class lookup was tried for 16B..16KiB
  it improved medium but regressed mixed enough that it did not hold a stable
  win as a v2 step

Read:
  SpanClassLookupTrim-L1 is archived no-go; keep the compact scan until a
  stronger local hot-path idea appears

Recheck after removal:
  small 45.266M ops/s, medium 7.304M ops/s, mixed 7.987M ops/s on a 3-run
  pass; the reverted baseline is the one to keep moving forward from
```

## Next Step

```text
next tiny-step candidate:
  choose a new tiny local hot-path change only if it can beat the archived
  scan baseline on small / medium / mixed together

Current read:
  route hot cache and SpanClassLookupTrim-L1 are both archived
  keep v1 frozen and stop layering tiny route/class changes unless a stronger
  paired improvement appears
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
