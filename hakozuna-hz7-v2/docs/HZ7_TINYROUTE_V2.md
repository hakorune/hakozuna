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
  a 2000-line-class tiny allocator

Design center:
  direct API
  route safety
  low RSS
  local small/medium performance
  coarse multithread safety

Non-goals:
  remote allocator speed
  libc interpose
  owner inboxes
  TLS ownership
  HZ6-style profile matrix
```

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
```

## Current Growth Step

```text
First pass:
  keep span initialization lightweight
  do not over-retain empty spans for medium classes

Read:
  medium/local reuse knobs need to earn their keep in benchmark
  if a knob slows medium/mixed, keep the v1 baseline instead
```
