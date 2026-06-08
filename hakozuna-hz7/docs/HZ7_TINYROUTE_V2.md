# HZ7 TinyRoute V2

HZ7 TinyRoute V2 is the next-stage design note for a tiny allocator that stays
small while becoming more complete on local workloads.

## Folder Policy

```text
Code:
  keep HZ7 v2 in the existing hakozuna-hz7 folder for now

Docs:
  add v2 notes under hakozuna-hz7/docs

Do not:
  create a parallel code folder yet
```

The reason is simple: HZ7 v1 is still one small allocator family, not a split
product line. A new folder only becomes useful if v2 grows enough that the code
shape no longer fits the current single-tree layout.

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

## Why Same Folder First

Keeping v2 in the same folder keeps the boundary honest.

```text
If the allocator still fits in one readable tree:
  keep it together

If v2 starts needing separate runtime shape, separate benchmarks, or separate
feature families:
  then split into a new folder
```

The current HZ7 v1 implementation is still compact enough that a folder split
would mostly add noise rather than clarity.

## Suggested Organization

```text
hakozuna-hz7/
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
  keep the same folder
  write docs first
  keep code in place

Later:
  split only if v2 proves it needs a different shape
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
```
