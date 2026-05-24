# HZ5 Linux Development Brief

This is the short entry point for the current HZ5 Linux allocator work. Keep
large measurement logs in `current_task.md` and route-specific design docs.

## Current Direction

HZ5 Linux is no longer only an exact `64K/a8192` sidecar. It is being rebuilt
as a general LD_PRELOAD allocator candidate:

```text
SmallFront-S1:
  ordinary malloc <= 2048

MidFront-M1:
  ordinary malloc 2049..65536

LargeFront-L1:
  ordinary malloc 65537..1048576

Local2P:
  exact 64K/a8192 appendix/special profile
```

Windows P43i/P45 remains separate and must not be weakened by Linux front-end
experiments.

## Current Best HZ5 Linux Rows

```text
Local exact 64K/a8192:
  hz5-local2p-linkflags

RSS-retained exact 64K/a8192:
  hz5-local2p-rssretain2048tls

Producer/consumer exact remote-free:
  hz5-local2p-remotebatch

General malloc local/mixed:
  hz5-smallfront-s1 + hz5-midfront-rb16 + hz5-largefront-l1

Remote-heavy large:
  hz5-largefront-inbox
```

Diagnostic-only LargeFront rows:

```text
hz5-largefront-rb16:
  remote batch cap16
  not a clear win

hz5-largefront-takefirst:
  direct remote drain return
  broad regression risk
```

## Latest LargeFront Finding

LargeFront-L1 fixed the `cross128` fallback hole for local traffic.

Focused repeat-3 medians with `HZ5_PRELOAD_STATS` disabled:

```text
cross128 r0:
  tcmalloc  80.22M
  HZ5 L1    63.45M
  HZ4       30.79M

large_only r0:
  tcmalloc 104.12M
  HZ4       94.63M
  HZ5 L1    81.93M
```

Owner-inbox improved remote-heavy large traffic:

```text
large_only r90:
  L1 global recycle  3.66M
  owner inbox        8.81M

cross128 r90:
  L1 global recycle  3.03M
  owner inbox        6.51M
```

Remote batch and take-first did not become broad winners.

## Current Weakness

`cross128 r90` remains behind HZ4/tcmalloc. LargeFront-only tuning is no longer
the whole bottleneck:

```text
likely cause:
  cross-size remote handoff cost across SmallFront, MidFront, and LargeFront

not enough:
  LargeFront class-drain tuning only
  MidFront allgate only
  LargeFront remote batch only
```

## OwnerHub-R1 Finding

After the Pro review, the next direction is OwnerHub-R1:

```text
shared owner-slot pending mask
per-front specialized inbox payloads
no generic RemoteEntry yet
dry-run observation only
```

Initial diagnostic runs used `HZ5_OWNERHUB_STATS=1` and are not raw performance
runs.

Short r90 observations:

```text
cross128:
  publish_small=84 publish_mid=1977 publish_large=3090
  miss_any_pending=3302
  miss_target_pending=537
  miss_other_pending=3258

main:
  publish_small=373 publish_mid=4270 publish_large=0
  miss_any_pending=2143
  miss_target_pending=480
  miss_other_pending=2090

large_only:
  publish_large=8233
  miss_any_pending=414
  miss_target_pending=414
  miss_other_pending=0
```

Interpretation:

```text
large_only is a same-front problem.
main/cross128 frequently have other-front pending work at alloc miss.
OwnerHub-R2 coordinated drain has a real target.
```

## Next Technical Question

Should HZ5 Linux remote-heavy general malloc continue with per-front-end owner
inboxes, or should it introduce a shared remote handoff layer?

Candidate designs:

```text
1. OwnerHub-R2 coordinated drain:
   shared owner pending mask
   per-front specialized inbox payloads
   drain bounded Small/Mid/Large work on alloc miss

2. Full shared remote handoff layer:
   one owner-slot inbox shared by SmallFront/MidFront/LargeFront
   entries carry front-end kind + class + pointer
   not recommended yet

3. Benchmark-shape SPSC transfer lane:
   optimize producer/consumer handoff directly
   likely diagnostic/paper appendix, not default allocator

4. Stop remote tuning for now:
   treat HZ5 as local/mixed candidate
   document remote-heavy as future work
```

## Where To Read Next

```text
Route and lane names:
  docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md

LargeFront design and measurements:
  docs/HZ5_LARGEFRONT_L1_DESIGN.md

Paper benchmark policy:
  docs/HZ5_PAPER_BENCH_SUITE.md

Implementation map:
  docs/source-map.md

Full chronological log:
  current_task.md
```
