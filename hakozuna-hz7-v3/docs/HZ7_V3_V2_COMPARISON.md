# HZ7 V2 vs V3

This note keeps the split between the v2 closeout reference and the v3
performance-growth fork explicit.

## HZ7 v2

```text
identity:
  tiny reference allocator
  route-safe
  low RSS
  coarse-lock remote-free safe
  RemoteNatural-L1 bounded remote pressure evidence

goal:
  stay tiny, readable, and safe
  keep remote behavior as safety/control, not throughput
```

## HZ7 v3

```text
identity:
  performance-growth fork
  starts from v2 code
  keeps the same basic route safety and remote-free safety contract

goal:
  improve the local 4K..16K span path
  keep low RSS close to v2
  keep the experiment readable while it grows
```

## Main Differences

```text
v2:
  closeout reference
  lock-scope and direct retain decisions are already settled
  benchmark snapshots are primarily closeout evidence

v3:
  first-class experiment folder
  route/span/big module split in the allocator
  ops/rows/sequences split in the benchmark driver
  benchmark surface and comparison notes kept alongside the code
```

## What Stayed the Same

```text
route safety:
  preserved

remote-free safety:
  preserved

readability:
  still a requirement in both folders
```

## Why This Split Exists

V2 is the tiny reference that should remain stable. V3 is where the current
local span-path experiments can grow without blurring the v2 closeout story.
